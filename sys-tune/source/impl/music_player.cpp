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

#define MPG_TRY(res_expr)                             \
    ({                                                \
        const auto res = (res_expr);                  \
        if (AMS_UNLIKELY(res != MPG123_OK)) {         \
            mpg123_desc = mpg123_plain_strerror(res); \
            return ResultMpgFailure();                \
        }                                             \
    })

namespace ams::tune::impl {

    namespace {

        std::mt19937 urng;
        std::vector<std::string> g_playlist;
        std::vector<std::string> g_shuffle_list;
        int g_queue_position;
        os::Mutex g_mutex;

        RepeatMode g_repeat = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status;

        bool should_run;
        const char *mpg123_desc;

        double g_tpf;
        off_t g_total_frame_count;

        mpg123_handle *music_handle = nullptr;

        Result AppendBuffer(AudioOutBuffer *buffer) {
            /* Read decoded audio. */
            size_t data_size;
            MPG_TRY(mpg123_read(music_handle, (u8 *)buffer->buffer, buffer->buffer_size, &data_size));
            buffer->data_size = data_size;
            /* Append the decoded audio buffer. */
            return audoutAppendAudioOutBuffer(buffer);
        };

        Result WaitAndAppend() {
            /* Wait buffers to finish playing and append those again. */
            AudioOutBuffer *buffers;
            u32 count;
            R_TRY(audoutWaitPlayFinish(&buffers, &count, 10'000'000));

            for (u32 i = 0; i < count; i++) {
                R_TRY(AppendBuffer(buffers + 1));
            }
            return ResultSuccess();
        };

        ams::Result PlayTrack(const std::string &path) {
            /* Open file. */
            MPG_TRY(mpg123_open(music_handle, path.c_str()));
            ON_SCOPE_EXIT { mpg123_close(music_handle); };

            /* Fix format to prevent variation during playback. */
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
            ON_SCOPE_EXIT {
                g_tpf = 0;
                g_total_frame_count = 0;
            };

            size_t data_size = mpg123_outblock(music_handle);
            size_t buffer_size = (data_size + 0xfff) & ~0xfff; // Align to 0x1000 bytes

            constexpr const size_t reserved_mem = 0x8000;
            u16 buffer_count = reserved_mem / buffer_size;

            struct ScopedOutBuffer {
                AudioOutBuffer buffer;
                ScopedOutBuffer() : buffer({}) {}
                bool init(size_t buffer_size) {
                    buffer.buffer = memalign(0x1000, buffer_size);
                    buffer.buffer_size = buffer_size;
                    return buffer.buffer;
                }
                ~ScopedOutBuffer() {
                    if (buffer.buffer)
                        free(buffer.buffer);
                }
            };

            ScopedOutBuffer buffers[buffer_count];
            for (auto &buffer : buffers) {
                R_UNLESS(buffer.init(buffer_size), MAKERESULT(Module_Libnx, LibnxError_OutOfMemory));
                R_TRY(AppendBuffer(&buffer.buffer));
            }

            /* Audio get's crackly if you stop audout with pending buffers. */
            ON_SCOPE_EXIT {
                audoutFlushAudioOutBuffers(nullptr);
            };

            while (g_status == PlayerStatus::Playing) {
                R_TRY(WaitAndAppend());
            }

            return ResultSuccess();
        }

        std::vector<std::string> &GetCurrentQueue() {
            if (g_shuffle == ShuffleMode::On) {
                return g_shuffle_list;
            } else {
                return g_playlist;
            }
        }

        std::vector<std::string> &GetOtherQueue() {
            if (g_shuffle == ShuffleMode::On) {
                return g_playlist;
            } else {
                return g_shuffle_list;
            }
        }

    }

    Result Initialize() {
        u64 seed[2];
        envGetRandomSeed(seed);
        urng = std::mt19937(seed[0]);
        should_run = true;
        g_status = PlayerStatus::FetchNext;

        R_TRY(audoutStartAudioOut());
        MPG_TRY(mpg123_init());
        int err;
        music_handle = mpg123_new(nullptr, &err);
        MPG_TRY(err);

        /* Set parameters. */
        MPG_TRY(mpg123_param(music_handle, MPG123_FORCE_RATE, audoutGetSampleRate(), 0));
        MPG_TRY(mpg123_param(music_handle, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO, 0));
        MPG_TRY(mpg123_volume(music_handle, 0.4));

        return ResultSuccess();
    }

    void Exit() {
        should_run = false;

        mpg123_delete(music_handle);
        mpg123_exit();
        audoutStopAudioOut();
    }

    void ThreadFunc(void *) {
        /* Run as long as we aren't stopped and no error has been encountered. */
        while (should_run) {
            /* Sleep if no track was selected. */
            if (g_queue_position < 0)
                svcSleepThread(1'000'000'000ul);

            std::string next_track;

            {
                std::scoped_lock lk(g_mutex);

                auto &queue = GetCurrentQueue();
                size_t queue_size = queue.size();
                if (g_queue_position >= queue_size) {
                    g_queue_position = queue_size - 1;
                    continue;
                }

                next_track = queue[g_queue_position];
            }

            /* Only play if playing and we have a track queued. */
            const char *mpg_desc = nullptr;
            Result rc = PlayTrack(next_track);

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

            next_track = "";

            Next();
        }
    }

    void PscThreadFunc(void *ptr) {
        PscPmModule *module = static_cast<PscPmModule *>(ptr);

        AudioOutState pre_sleep_state = AudioOutState_Stopped;

        /* Don't react for the first second after boot. */
        svcSleepThread(1'000'000'000);
        while (should_run) {
            eventWait(&module->event, UINT64_MAX);

            PscPmState state;
            u32 flags;
            if (R_SUCCEEDED(pscPmModuleGetRequest(module, &state, &flags))) {
                switch (state) {
                    case PscPmState_ReadyAwaken:
                        if (pre_sleep_state == AudioOutState_Started)
                            Play();
                        break;
                    case PscPmState_ReadySleep:
                        if (R_SUCCEEDED(audoutGetAudioOutState(&pre_sleep_state)))
                            if (pre_sleep_state == AudioOutState_Started)
                                Pause();
                        break;
                    case PscPmState_ReadyShutdown:
                        audoutFlushAudioOutBuffers(nullptr);
                        audoutStopAudioOut();
                        break;
                    default:
                        break;
                }
                pscPmModuleAcknowledge(module, state);
            }
        }
    }

    void GpioThreadFunc(void *ptr) {
        GpioPadSession *session = static_cast<GpioPadSession *>(ptr);

        AudioOutState pre_unplug_state = AudioOutState_Stopped;

        /* [0] Low == plugged in; [1] High == not plugged in. */
        GpioValue old_value = GpioValue_High;

        /* Don't react for the first second after boot. */
        svcSleepThread(1'000'000'000);
        while (should_run) {
            /* Fetch current gpio value. */
            GpioValue value;
            if (R_SUCCEEDED(gpioPadGetValue(session, &value))) {
                if (old_value == GpioValue_Low && value == GpioValue_High) {
                    if (R_SUCCEEDED(audoutGetAudioOutState(&pre_unplug_state)))
                        if (pre_unplug_state == AudioOutState_Started)
                            Pause();
                } else if (old_value == GpioValue_High && value == GpioValue_Low) {
                    if (pre_unplug_state == AudioOutState_Started)
                        Play();
                }
                old_value = value;
            }
        }
    }

    Result ShuffleImpl() {
        std::shuffle(g_shuffle_list.begin(), g_shuffle_list.end(), urng);

        return ResultSuccess();
    }

    /* NEW */

    Result GetStatus(AudioOutState *out) {
        return audoutGetAudioOutState(out);
    }

    Result Play() {
        return audoutStartAudioOut();
    }

    Result Pause() {
        return audoutStopAudioOut();
    }

    void Next() {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position < GetCurrentQueue().size() - 1) {
                g_queue_position++;
            } else {
                g_queue_position = 0;
            }
        }
        g_status = PlayerStatus::FetchNext;
        audoutFlushAudioOutBuffers(nullptr);
    }

    void Prev() {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position > 0) {
                g_queue_position--;
            } else {
                g_queue_position = GetCurrentQueue().size() - 1;
            }
        }
        g_status = PlayerStatus::FetchNext;
        audoutFlushAudioOutBuffers(nullptr);
    }

    Result GetVolume(float *volume) {
        if (hosversionBefore(6, 0, 0)) {
            double base, real, rva;
            if (mpg123_getvolume(music_handle, &base, &real, &rva) != MPG123_OK)
                return ResultMpgFailure();
            *volume = (real * 2);
            return ResultSuccess();
        }

        return audoutGetAudioOutVolume(volume);
    }

    Result SetVolume(float volume) {
        /* Volume in range? */
        if (volume < 0) {
            volume = 0;
        } else if (volume > 2) {
            volume = 2;
        }

        if (hosversionBefore(6, 0, 0)) {
            if (mpg123_volume(music_handle, volume / 2) != MPG123_OK)
                return ResultMpgFailure();
        }
        return audoutSetAudioOutVolume(volume);
    }

    void GetRepeatMode(RepeatMode *mode) {
        *mode = g_repeat;
    }

    void SetRepeatMode(RepeatMode mode) {
        g_repeat = mode;
    }

    void GetShuffleMode(ShuffleMode *mode) {
        *mode = g_shuffle;
    }

    void SetShuffleMode(ShuffleMode mode) {
        /* TODO */
        g_shuffle = mode;
    }

    void GetCurrentPlaylistSize(u32 *size) {
        std::scoped_lock lk(g_mutex);

        *size = GetCurrentQueue().size();
    }

    void ListCurrentPlaylist(u32 *size, char *buffer, size_t buffer_size) {
        std::scoped_lock lk(g_mutex);

        u32 tmp = 0;
        size_t remaining = buffer_size / FS_MAX_PATH;
        auto &queue = GetCurrentQueue();
        /* Traverse queue and write to buffer. */
        for (auto it = queue.cbegin(); it != queue.cend() && remaining; ++it) {
            std::strncpy(buffer, it->c_str(), FS_MAX_PATH);
            buffer += FS_MAX_PATH;
            remaining--;
            tmp++;
        }
        *size = tmp;
    }

    Result GetCurrentQueueItem(CurrentStats *out, char *buffer, size_t buffer_size) {
        {
            std::scoped_lock lk(g_mutex);
            R_UNLESS(g_queue_position < 0, ResultNotPlaying());
            auto &queue = GetCurrentQueue();
            R_UNLESS(!queue.empty(), ResultQueueEmpty());
            auto &track = queue[g_queue_position];
            R_UNLESS(!track.empty(), ResultNotPlaying());
            R_UNLESS(buffer_size >= track.size(), ResultInvalidArgument());
            std::strcpy(buffer, track.c_str());
        }

        double progress_frame_count = mpg123_tellframe(music_handle);
        R_UNLESS(progress_frame_count > 0, ResultMpgFailure());

        out->tpf = g_tpf;
        out->total_frame_count = g_total_frame_count;
        out->progress_frame_count = progress_frame_count;

        return ResultSuccess();
    }

    void ClearQueue() {
        {
            std::scoped_lock lk(g_mutex);

            g_playlist.clear();
            g_shuffle_list.clear();

            g_queue_position = -1;
        }
        g_status = PlayerStatus::FetchNext;
        audoutFlushAudioOutBuffers(nullptr);
    }

    void MoveQueueItem(u32 src, u32 dst) {
        std::scoped_lock lk(g_mutex);

        auto &queue = GetCurrentQueue();
        size_t queue_size = queue.size();

        if (src >= queue_size) {
            src = queue_size - 1;
        }
        if (dst >= queue_size) {
            dst = queue_size - 1;
        }

        auto source = queue.cbegin() + src;
        auto dest = queue.cbegin() + dst;

        g_playlist.insert(dest, *source);
        g_playlist.erase(source);

        if (src < dst) {
            if (g_queue_position == src) {
                g_queue_position = dst;
            } else if (g_queue_position >= src && g_queue_position <= dst) {
                g_queue_position--;
            }
        } else if (dst < src) {
            if (g_queue_position == src) {
                g_queue_position = dst;
            } else if (g_queue_position >= dst && g_queue_position <= src) {
                g_queue_position++;
            }
        }
    }

    Result Enqueue(char *buffer, size_t buffer_length, EnqueueType type) {
        /* At root. */
        R_UNLESS(buffer[0] == '/', ResultInvalidPath());
        /* Maps a mp3 file? */
        R_UNLESS(strcasecmp(buffer + buffer_length - 4, ".mp3") == 0, ResultInvalidPath());

        std::scoped_lock lk(g_mutex);

        auto &current = GetCurrentQueue();
        auto &other = GetOtherQueue();

        if (type == EnqueueType::Next) {
            current.insert(current.cbegin() + 1, buffer);
            other.push_back(buffer);
        } else {
            current.push_back(buffer);
            other.push_back(buffer);
        }

        return ResultSuccess();
    }

    void Remove(u32 index) {
        std::scoped_lock lk(g_mutex);

        auto &queue = GetCurrentQueue();
        if (queue.empty() || index >= queue.size())
            return;
        auto track = queue.cbegin() + g_queue_position;
        queue.erase(track);

        auto &other_queue = GetOtherQueue();
        for (auto it = other_queue.cbegin(); it != other_queue.cend(); it++)
            if (*it == *track)
                other_queue.erase(it);
    }

}
