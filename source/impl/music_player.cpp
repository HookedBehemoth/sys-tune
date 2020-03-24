#include "music_player.hpp"

#include "../music_result.hpp"

#include <atomic>
#include <list>
#include <malloc.h>
#include <mpg123.h>
#include <numeric>
#include <random>
#include <regex>
#include <string>

#define MPG_TRY(res_expr)                              \
    ({                                                 \
        const auto res = (res_expr);                   \
        if (AMS_UNLIKELY(res != MPG123_OK)) {          \
            *mpg123_desc = mpg123_plain_strerror(res); \
            return ResultMpgFailure();                 \
        }                                              \
    })

namespace ams::music {

    namespace {

        std::mt19937 urng;
        std::string g_current;
        std::list<std::string> g_queue;
        std::atomic<PlayerStatus> g_status;
        std::atomic<LoopStatus> g_loop = LoopStatus::List;
        std::atomic<double> g_tpf = 0;
        std::atomic<double> g_total_frame_count = 0;
        std::atomic<double> g_progress_frame_count = 0;
        std::atomic<double> g_volume = 0.4;
        os::Mutex g_queue_mutex;

        mpg123_handle *music_handle = nullptr;

        ams::Result ThreadFuncImpl(const char **mpg123_desc, const char *path) {
            /* Start audio out init */
            R_TRY(audoutStartAudioOut());
            ON_SCOPE_EXIT { audoutStopAudioOut(); };

            MPG_TRY(mpg123_init());
            ON_SCOPE_EXIT { mpg123_exit(); };

            int err;
            music_handle = mpg123_new(nullptr, &err);
            MPG_TRY(err);
            ON_SCOPE_EXIT { mpg123_delete(music_handle); };

            /* Set parameters. */
            MPG_TRY(mpg123_param(music_handle, MPG123_FORCE_RATE, audoutGetSampleRate(), 0));
            MPG_TRY(mpg123_param(music_handle, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO, 0));

            /* Open file. */
            MPG_TRY(mpg123_open(music_handle, path));
            ON_SCOPE_EXIT { mpg123_close(music_handle); };

            long rate;
            int channels, encoding;
            MPG_TRY(mpg123_getformat(music_handle, &rate, &channels, &encoding));
            MPG_TRY(mpg123_format_none(music_handle));
            MPG_TRY(mpg123_format(music_handle, rate, channels, encoding));

            /* Get length of track in frames. */
            off_t frame_count = mpg123_framelength(music_handle);
            R_UNLESS(frame_count != MPG123_ERR, ResultMpgFailure());
            g_total_frame_count = frame_count;

            /* Get length of frame in seconds. */
            double tpf = mpg123_tpf(music_handle);
            R_UNLESS(tpf >= 0, ResultMpgFailure());
            g_tpf = tpf;

            /* Reset frame information on exit. */
            ON_SCOPE_EXIT { g_tpf = 0; };
            ON_SCOPE_EXIT { g_total_frame_count = 0; };
            ON_SCOPE_EXIT { g_progress_frame_count = 0; };

            size_t data_size = mpg123_outblock(music_handle);
            size_t buffer_size = (data_size + 0xfff) & ~0xfff; // Align to 0x1000 bytes

            u8 *a_buffer = (u8 *)memalign(0x1000, buffer_size);
            R_UNLESS(a_buffer, MAKERESULT(Module_Libnx, LibnxError_OutOfMemory));
            ON_SCOPE_EXIT { free(a_buffer); };

            u8 *b_buffer = (u8 *)memalign(0x1000, buffer_size);
            R_UNLESS(b_buffer, MAKERESULT(Module_Libnx, LibnxError_OutOfMemory));
            ON_SCOPE_EXIT { free(b_buffer); };

            AudioOutBuffer audio_buffer[2] = {
                {
                    .next = nullptr,
                    .buffer = a_buffer,
                    .buffer_size = buffer_size,
                    .data_offset = 0,
                },
                {
                    .next = nullptr,
                    .buffer = b_buffer,
                    .buffer_size = buffer_size,
                    .data_offset = 0,
                },
            };

            /* Set volume before the first few reads. */
            MPG_TRY(mpg123_volume(music_handle, g_volume / 2));

            /* Audio get's crackly if you stop audout without waiting for remaining buffers. */
            ON_SCOPE_EXIT {
                bool wait;
                AudioOutBuffer *released;
                u32 released_count;
                /* For each queued buffer wait for one to finish. */
                for (auto &buffer : audio_buffer) {
                    audoutContainsAudioOutBuffer(&buffer, &wait);
                    if (wait)
                        audoutWaitPlayFinish(&released, &released_count, 1'000'000'000ul);
                }
            };

            size_t done;

            /* Read initial buffer. */
            MPG_TRY(mpg123_read(music_handle, (u8 *)audio_buffer[0].buffer, buffer_size, &done));
            audio_buffer[0].data_size = done;
            /* Append buffer. */
            R_TRY(audoutAppendAudioOutBuffer(&audio_buffer[0]));

            size_t index = 1;
            while (true) {
                /* Read decoded audio. */
                int res = mpg123_read(music_handle, (u8 *)audio_buffer[index].buffer, buffer_size, &done);
                if (AMS_UNLIKELY(res == MPG123_DONE)) {
                    break;
                } else if (AMS_UNLIKELY(res != MPG123_OK)) {
                    *mpg123_desc = mpg123_plain_strerror(res);
                    return ResultMpgFailure();
                }
                audio_buffer[index].data_size = done;

                /* Check if not supposed to be playing. */
                if (AMS_UNLIKELY(g_status != PlayerStatus::Playing)) {
                    /* Return if not paused. Exit or Stop requested. */
                    if (g_status != PlayerStatus::Paused) {
                        break;
                    }
                    R_TRY(audoutStopAudioOut());
                    while (g_status == PlayerStatus::Paused) {
                        /* Sleep if paused. */
                        svcSleepThread(100'000'000ul);
                    }
                    R_TRY(audoutStartAudioOut());
                }
                /* Append the decoded audio buffer. */
                R_TRY(audoutAppendAudioOutBuffer(&audio_buffer[index]));

                /* Check progress in track. */
                off_t frame = mpg123_tellframe(music_handle);
                R_UNLESS(frame >= 0, ResultMpgFailure());
                g_progress_frame_count = frame;

                /* Set volume. */
                MPG_TRY(mpg123_volume(music_handle, g_volume / 2));

                /* Wait for the last buffer to stop playing. */
                AudioOutBuffer *released;
                u32 released_count;
                R_TRY(audoutWaitPlayFinish(&released, &released_count, 1'000'000'000ul));

                index = (index + 1) % 2;
            }

            return ResultSuccess();
        }
    }

    void Initialize() {
        u64 seed[2];
        envGetRandomSeed(seed);
        urng = std::mt19937(seed[0]);
        g_status = PlayerStatus::Stopped;
    }

    void Exit() {
        g_status = PlayerStatus::Exit;
    }

    void ThreadFunc(void *) {
        bool has_next = false;
        char absolute_path[FS_MAX_PATH];

        /* Run as long as we aren't stopped and no error has been encountered. */
        while (g_status != PlayerStatus::Exit) {
            /* Deque current track. */
            if (g_status == PlayerStatus::Next) {
                g_status = PlayerStatus::Playing;
                has_next = false;
            }
            if (!has_next) {
                /* Obtain path to next track. */
                std::scoped_lock lk(g_queue_mutex);
                has_next = !g_queue.empty();
                if (has_next) {
                    g_current = g_queue.front();
                    std::snprintf(absolute_path, FS_MAX_PATH, "sdmc:%s", g_current.c_str());
                }
            }
            /* Only play if playing and we have a track queued. */
            if (g_status == PlayerStatus::Playing && has_next) {
                const char *mpg_desc = nullptr;

                do {
                    Result rc = ThreadFuncImpl(&mpg_desc, absolute_path);

                    /* Log error. */
                    if (R_FAILED(rc)) {
                        FILE *file = fopen("sdmc:/music.log", "a");
                        if (AMS_LIKELY(file)) {
                            if (rc.GetValue() == ResultMpgFailure().GetValue()) {
                                if (!mpg_desc)
                                    mpg_desc = "UNKNOWN";
                                fprintf(file, "mpg error: %s\n", mpg_desc);
                            } else {
                                fprintf(file, "other error: 0x%x, 2%03X-%04X\n", rc.GetValue(), rc.GetModule(), rc.GetDescription());
                            }
                            fclose(file);
                        }
                    }
                } while (g_loop == LoopStatus::Single && g_status == PlayerStatus::Playing);

                has_next = false;
                absolute_path[0] = '\0';

                /* Finally pop track from queue. */
                std::scoped_lock lk(g_queue_mutex);
                if (!g_queue.empty()) {
                    auto front = g_queue.front();
                    g_queue.pop_front();
                    if (g_loop == LoopStatus::List)
                        g_queue.push_back(front);
                }

            } else {
                /* Nothing queued. Let's sleep. */
                svcSleepThread(1'000'000'000ul);
            }
        }
    }

    void EventThreadFunc(void *) {
        /* Aquire power button event handle. */
        Event power_button_event = {};
        Result rc = hidsysAcquireSleepButtonEventHandle(&power_button_event);
        if (R_FAILED(rc))
            return;
        eventClear(&power_button_event);
        /* Cleanup event handle. */
        ON_SCOPE_EXIT { eventClose(&power_button_event); };

        GpioPadSession headphone_detect_session = {};
        rc = gpioOpenSession(&headphone_detect_session, GpioPadName(0x15));
        if (R_FAILED(rc))
            return;
        /* Close gpio session. */
        ON_SCOPE_EXIT { gpioPadClose(&headphone_detect_session); };

        /* [0] Low == plugged in; [1] High == not plugged in. */
        GpioValue headphone_value = GpioValue_High;

        /* During runtime listen to the event. */
        while (g_status != PlayerStatus::Exit) {
            if (R_SUCCEEDED(eventWait(&power_button_event, 20'000'000)))
                g_status = PlayerStatus::Paused;

            GpioValue tmp_value;
            if (R_SUCCEEDED(gpioPadGetValue(&headphone_detect_session, &tmp_value))) {
                if (headphone_value == GpioValue_Low && tmp_value == GpioValue_High) {
                    g_status = PlayerStatus::Paused;
                }
                headphone_value = tmp_value;
            }

            /* Sleep */
            svcSleepThread(20'000'000);
        }
    }

    Result GetStatusImpl(PlayerStatus *out) {
        *out = g_status;
        return ResultSuccess();
    }

    Result SetStatusImpl(PlayerStatus status) {
        g_status = status;
        return ResultSuccess();
    }

    Result GetVolumeImpl(double *out) {
        *out = g_volume;

        return ResultSuccess();
    }

    Result SetVolumeImpl(double volume) {
        /* Volume in range? */
        if (volume < 0) {
            volume = 0;
        } else if (volume > 1) {
            volume = 1;
        }

        g_volume = volume;

        return ResultSuccess();
    }

    Result GetLoopImpl(LoopStatus *out) {
        *out = g_loop;

        return ResultSuccess();
    }

    Result SetLoopImpl(LoopStatus loop) {
        g_loop = loop;

        return ResultSuccess();
    }

    Result GetCurrentImpl(char *out_path, size_t out_path_length, CurrentTune *out) {
        R_UNLESS(out_path_length >= FS_MAX_PATH, ResultInvalidArgument());

        out->volume = g_volume;
        out->tpf = g_tpf;
        out->total_frame_count = g_total_frame_count;
        out->progress_frame_count = g_progress_frame_count;

        std::scoped_lock lk(g_queue_mutex);
        R_UNLESS(!g_queue.empty(), ResultQueueEmpty());
        const char *ptr = g_queue.front().c_str();
        R_UNLESS(ptr, ResultNotPlaying());
        std::strcpy(out_path, ptr);

        return ResultSuccess();
    }

    Result CountTunesImpl(u32 *out) {
        *out = g_queue.size();
        return ResultSuccess();
    }

    Result ListTunesImpl(char *out_path, size_t out_path_length, u32 *out) {
        std::scoped_lock lk(g_queue_mutex);

        u32 tmp = 0;
        /* Traverse queue and write to buffer. */
        size_t remaining = out_path_length / FS_MAX_PATH;
        for (auto it = g_queue.cbegin(); it != g_queue.cend() && remaining; ++it) {
            std::strncpy(out_path, it->c_str(), FS_MAX_PATH);
            out_path += FS_MAX_PATH;
            remaining--;
            tmp++;
        }
        *out = tmp;

        return ResultSuccess();
    }

    Result AddToQueueImpl(const char *path, size_t path_length) {
        /* At root. */
        R_UNLESS(path[0] == '/', ResultInvalidPath());
        /* Maps a mp3 file? */
        R_UNLESS(strcasecmp(path + path_length - 4, ".mp3") == 0, ResultInvalidPath());

        std::scoped_lock lk(g_queue_mutex);

        /* Add song to queue. */
        g_queue.push_back(path);

        return ResultSuccess();
    }

    Result ClearImpl() {
        std::scoped_lock lk(g_queue_mutex);

        while (!g_queue.empty())
            g_queue.pop_front();

        return ResultSuccess();
    }

    Result ShuffleImpl() {
        R_UNLESS(g_status == PlayerStatus::Stopped, ResultInvalidStatus());

        std::scoped_lock lk(g_queue_mutex);

        /* Reference list in random accessable vector. */
        std::vector<std::reference_wrapper<const std::string>> shuffle_vect(g_queue.begin(), g_queue.end());
        /* Shuffle reference vector. */
        std::shuffle(shuffle_vect.begin(), shuffle_vect.end(), urng);

        /* Create list in shuffled order. */
        std::list<std::string> shuffled;
        for (auto &ref : shuffle_vect)
            shuffled.push_back(std::move(ref.get()));

        /* Swap out unshuffled list. */
        g_queue.swap(shuffled);

        return ResultSuccess();
    }

}
