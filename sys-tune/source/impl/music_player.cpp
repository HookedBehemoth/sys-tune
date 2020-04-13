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

#define MPG_TRY(res_expr)                                 \
    ({                                                    \
        const auto res = (res_expr);                      \
        if (AMS_UNLIKELY(res != MPG123_OK)) {             \
            if (res == MPG123_DONE) {                     \
                return ResultMpgDone();                   \
            } else {                                      \
                mpg123_desc = mpg123_plain_strerror(res); \
                return ResultMpgFailure();                \
            }                                             \
        }                                                 \
    })

namespace ams::tune::impl {

    namespace {

        std::vector<std::string> g_playlist;
        std::vector<std::string> g_shuffle_playlist;
        std::string empty_string;
        std::string &g_current = empty_string;
        u32 g_queue_position;
        os::Mutex g_mutex;

        RepeatMode g_repeat = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status = PlayerStatus::Playing;

        AudioOutState audout_state = AudioOutState_Stopped;

        bool should_pause = false;
        bool should_run = true;
        const char *mpg123_desc;

        double g_tpf;
        off_t g_total_frame_count;

        mpg123_handle *music_handle = nullptr;

        Result AppendBuffer(AudioOutBuffer *buffer) {
            /* Read decoded audio. */
            size_t data_size;
            MPG_TRY(mpg123_read(music_handle, (u8 *)buffer->buffer, buffer->buffer_size, &data_size));
            buffer->data_size = data_size;
            armDCacheFlush(buffer->buffer, data_size);

            /* Append the decoded audio buffer. */
            return audoutAppendAudioOutBuffer(buffer);
        };

        Result PauseImpl() {
            R_SUCCEED_IF(audout_state == AudioOutState_Stopped);
            /* Wait for all buffers to finish playing. */
            u32 count;
            while (R_SUCCEEDED(audoutGetAudioOutBufferCount(&count)) && count > 0) {
                LOG("There are: %d buffers remaining\n", count);
                AudioOutBuffer *released;
                R_TRY(audoutWaitPlayFinish(&released, &count, UINT64_MAX));
            }
            /* Stop audio playback. */
            R_TRY(audoutStopAudioOut());
            audout_state = AudioOutState_Stopped;
            return ResultSuccess();
        }

        Result PlayImpl() {
            R_SUCCEED_IF(audout_state == AudioOutState_Started);
            R_TRY(audoutStartAudioOut());
            audout_state = AudioOutState_Started;
            return ResultSuccess();
        }

        Result SilentPauseImpl() {
            float prev_volume;
            R_TRY(GetVolume(&prev_volume));
            R_TRY(SetVolume(0));
            R_TRY(PlayImpl());
            R_TRY(PauseImpl());
            R_TRY(SetVolume(prev_volume));
            return ResultSuccess();
        }

        Result PlayTrack(const std::string &path) {
            R_TRY(SilentPauseImpl());
            ON_SCOPE_EXIT {
                SilentPauseImpl();
            };

            /* Open file. */
            MPG_TRY(mpg123_open(music_handle, path.c_str()));
            ON_SCOPE_EXIT { mpg123_close(music_handle); };

            /* Fix format to prevent variation during playback. */
            long rate;
            int channels, encoding;
            MPG_TRY(mpg123_getformat(music_handle, &rate, &channels, &encoding));
            MPG_TRY(mpg123_format_none(music_handle));
            MPG_TRY(mpg123_format(music_handle, rate, channels, encoding));

            /* Get length of frame in seconds. */
            double tpf = mpg123_tpf(music_handle);
            R_UNLESS(tpf >= 0, ResultMpgFailure());

            /* Get length of track in frames. */
            off_t frame_count = mpg123_framelength(music_handle);
            R_UNLESS(frame_count != MPG123_ERR, ResultMpgFailure());

            LOG("predicted length: %lf seconds\n", tpf * frame_count);

            g_total_frame_count = frame_count;
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

            R_UNLESS(buffer_count > 0, ResultBlockSizeTooBig());

            /* IAudioOut supports up to 32 buffers at the same time. */
            if (buffer_count > 0x20)
                buffer_count = 0x20;

            ScopedOutBuffer buffers[buffer_count];
            for (auto &buffer : buffers) {
                R_UNLESS(buffer.init(buffer_size), MAKERESULT(Module_Libnx, LibnxError_OutOfMemory));
                LOG("Buffer address: 0x%p\n", &buffer.buffer);
            }

            while (should_run && g_status == PlayerStatus::Playing) {
                if (should_pause) {
                    R_TRY(PauseImpl());
                } else {
                    R_TRY(PlayImpl());
                }

                /* Check if all buffers are still queued. */
                u32 count;
                R_TRY(audoutGetAudioOutBufferCount(&count));
                if (count != buffer_count) {
                    /* Append buffers that aren't queued. */
                    for (auto &buffer : buffers) {
                        bool appended;
                        if (R_SUCCEEDED(audoutContainsAudioOutBuffer(&buffer.buffer, &appended)) && !appended) {
                            AppendBuffer(&buffer.buffer);
                        }
                        LOG("buffer: 0x%p: appended: %s\n", &buffer.buffer, appended ? "true" : "false");
                    }
                }

                /* Wait buffers to finish playing and append those again. */
                AudioOutBuffer *released = nullptr;
                u32 release_count;
                if (R_SUCCEEDED(audoutWaitPlayFinish(&released, &release_count, 100'000'000))) {
                    if (release_count == 1) {
                        /* Append released audio buffer. */
                        R_TRY_CATCH(AppendBuffer(released)) {
                            R_CATCH(ResultMpgDone) {
                                if (g_repeat != RepeatMode::One)
                                    Next();

                                break;
                            }
                        }
                        R_END_TRY_CATCH;
                    }
                }
            }

            PauseImpl();

            return ResultSuccess();
        }

    }

    Result Initialize() {
        should_run = true;
        g_status = PlayerStatus::FetchNext;

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
    }

    void AudioThreadFunc(void *) {
        /* Run as long as we aren't stopped and no error has been encountered. */
        while (should_run) {
            g_current = empty_string;
            {
                std::scoped_lock lk(g_mutex);

                auto &queue = (g_shuffle == ShuffleMode::On) ? g_shuffle_playlist : g_playlist;
                size_t queue_size = queue.size();
                if (queue_size == 0) {
                    g_current = empty_string;
                } else if (g_queue_position >= queue_size) {
                    g_queue_position = queue_size - 1;
                    continue;
                } else  {
                    g_current = queue[g_queue_position];
                }
            }

            /* Sleep if queue is selected. */
            if (g_current.empty()) {
                svcSleepThread(100'000'000ul);
                continue;
            }

            LOG("Index: %d, track: %s\n", g_queue_position, g_current.c_str());

            g_status = PlayerStatus::Playing;
            /* Only play if playing and we have a track queued. */
            Result rc = PlayTrack(g_current);

            /* Log error. */
            if (R_FAILED(rc)) {
                /* Remove track if something went wrong. */
                Remove(g_queue_position);
                FILE *file = fopen("sdmc:/music.log", "a");
                if (AMS_LIKELY(file)) {
                    if (rc.GetValue() == ResultMpgFailure().GetValue()) {
                        if (!mpg123_desc)
                            mpg123_desc = "UNKNOWN";
                        fprintf(file, "mpg error: %s\n", mpg123_desc);
                    } else {
                        fprintf(file, "other error: 0x%x, 2%03X-%04X\n", rc.GetValue(), rc.GetModule(), rc.GetDescription());
                        u32 count;
                        u64 sample_count;
                        if (R_SUCCEEDED(audoutGetAudioOutBufferCount(&count)))
                            fprintf(file, "buffer count: %d\n", count);
                        if (R_SUCCEEDED(audoutGetAudioOutPlayedSampleCount(&sample_count)))
                            fprintf(file, "played sample count: %ld\n", sample_count);

                        SilentPauseImpl();
                    }
                    fclose(file);
                }
            }
        }

        mpg123_delete(music_handle);
        mpg123_exit();
    }

    void PscThreadFunc(void *ptr) {
        PscPmModule *module = static_cast<PscPmModule *>(ptr);

        /* Don't react for the first second after boot. */
        while (should_run) {
            if (R_FAILED(eventWait(&module->event, 10'000'000))) {
                continue;
            }

            PscPmState state;
            u32 flags;
            if (R_SUCCEEDED(pscPmModuleGetRequest(module, &state, &flags))) {
                switch (state) {
                    case PscPmState_ReadySleep:
                        should_pause = true;
                        while (audout_state == AudioOutState_Started)
                            ;
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
            svcSleepThread(10'000'000);
        }
    }

    Result GetStatus(AudioOutState *out) {
        return audoutGetAudioOutState(out);
    }

    Result Play() {
        should_pause = false;
        return ResultSuccess();
    }

    Result Pause() {
        should_pause = true;
        return ResultSuccess();
    }

    void Next() {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position < g_playlist.size() - 1) {
                g_queue_position++;
            } else {
                g_queue_position = 0;
                if (g_repeat == RepeatMode::Off)
                    should_pause = true;
            }
        }
        g_status = PlayerStatus::FetchNext;
    }

    void Prev() {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position > 0) {
                g_queue_position--;
            } else {
                g_queue_position = g_playlist.size() - 1;
            }
        }
        g_status = PlayerStatus::FetchNext;
    }

    Result GetVolume(float *volume) {
        if (hosversionBefore(6, 0, 0)) {
            double base, real, rva;
            if (mpg123_getvolume(music_handle, &base, &real, &rva) != MPG123_OK)
                return ResultMpgFailure();
            *volume = real * 2;
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
            return ResultSuccess();
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
        std::scoped_lock lk(g_mutex);

        if (g_playlist.size() > 0 && g_shuffle != mode) {
            auto &dst = (mode == ShuffleMode::On) ? g_shuffle_playlist : g_playlist;

            auto it = std::find(dst.cbegin(), dst.cend(), g_current);
            g_queue_position = it - dst.cbegin();
        }

        g_shuffle = mode;
    }

    void GetCurrentPlaylistSize(u32 *size) {
        std::scoped_lock lk(g_mutex);

        *size = g_playlist.size();
    }

    void GetCurrentPlaylist(u32 *size, char *buffer, size_t buffer_size) {
        std::scoped_lock lk(g_mutex);

        u32 tmp = 0;
        size_t remaining = buffer_size / FS_MAX_PATH;
        /* Traverse playlist and write to buffer. */
        for (auto it = g_playlist.cbegin(); it != g_playlist.cend() && remaining; ++it) {
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
            R_UNLESS(!g_current.empty(), ResultNotPlaying());
            R_UNLESS(buffer_size >= g_current.size(), ResultInvalidArgument());
            std::strcpy(buffer, g_current.c_str());
        }

        double progress_frame_count = mpg123_tellframe(music_handle);
        R_UNLESS(progress_frame_count != MPG123_ERR, ResultMpgFailure());

        out->tpf = g_tpf;
        out->total_frame_count = g_total_frame_count;
        out->progress_frame_count = progress_frame_count;

        return ResultSuccess();
    }

    void ClearQueue() {
        {
            std::scoped_lock lk(g_mutex);

            g_playlist.clear();
            g_shuffle_playlist.clear();
        }
        g_status = PlayerStatus::FetchNext;
    }

    void MoveQueueItem(u32 src, u32 dst) {
        std::scoped_lock lk(g_mutex);

        size_t queue_size = g_playlist.size();

        if (src >= queue_size) {
            src = queue_size - 1;
        }
        if (dst >= queue_size) {
            dst = queue_size - 1;
        }

        auto source = g_playlist.cbegin() + src;
        auto dest = g_playlist.cbegin() + dst;

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

    void Select(u32 index) {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position == index)
                return;

            size_t queue_size = g_playlist.size();

            if (index >= queue_size) {
                index = queue_size - 1;
            }
            g_queue_position = index;
        }
        g_status = PlayerStatus::FetchNext;
    }

    Result Enqueue(const char *buffer, size_t buffer_length, EnqueueType type) {
        /* Maps a mp3 file? */
        R_UNLESS(strcasecmp(buffer + buffer_length - 4, ".mp3") == 0, ResultInvalidPath());

        /* Ensure file exists and can be opened. */
        FILE *f = fopen(buffer, "rb");
        R_UNLESS(f, ResultFileNotFound());
        fclose(f);

        std::scoped_lock lk(g_mutex);

        if (type == EnqueueType::Next) {
            g_playlist.emplace(g_playlist.cbegin(), buffer, buffer_length);
        } else {
            g_playlist.emplace_back(buffer, buffer_length);
        }
        size_t shuffle_playlist_size = g_shuffle_playlist.size();
        size_t shuffle_index = (shuffle_playlist_size > 1) ? (randomGet64() % shuffle_playlist_size) : 0;
        g_shuffle_playlist.emplace(g_shuffle_playlist.cbegin() + shuffle_index, buffer, buffer_length);

        return ResultSuccess();
    }

    Result Remove(u32 index) {
        std::scoped_lock lk(g_mutex);

        /* Ensure we don't operate out of bounds. */
        R_UNLESS(!g_playlist.empty(), ResultQueueEmpty());
        R_UNLESS(index < g_playlist.size(), ResultOutOfRange());

        /* Get iterator for index position. */
        auto track = g_playlist.cbegin() + index;

        auto shuffle_it = std::find(g_shuffle_playlist.cbegin(), g_shuffle_playlist.cend(), *track);

        if (g_shuffle == ShuffleMode::On && shuffle_it != g_shuffle_playlist.cend())
            index = shuffle_it - g_shuffle_playlist.cbegin();

        /* Fetch a new track if we deleted the current song. */
        bool fetch_new = g_queue_position == index;

        /* Remove entry. */
        g_playlist.erase(track);
        if (shuffle_it != g_shuffle_playlist.cend())
            g_shuffle_playlist.erase(shuffle_it);

        /* Lower current position if needed. */
        if (g_queue_position > index) {
            g_queue_position--;
        }

        if (fetch_new)
            g_status = PlayerStatus::FetchNext;

        return ResultSuccess();
    }

}
