#include "music_player.hpp"

#include "../music_result.hpp"

#include <atomic>
#include <malloc.h>
#include <mpg123.h>
#include <queue>
#include <regex>
#include <string>

#define MPG_TRY(res_expr)                              \
    ({                                                 \
        const auto res = (res_expr);                   \
        if (res != MPG123_OK) {                        \
            *mpg123_desc = mpg123_plain_strerror(res); \
            return ResultMpgFailure();                 \
        }                                              \
    })

namespace ams::music {

    namespace {

        std::string g_current;
        std::queue<std::string> g_queue;
        std::atomic<PlayerStatus> g_status;
        os::Mutex g_mutex;

        mpg123_handle *music_handle = nullptr;

        ams::Result ThreadFuncImpl(const char **mpg123_desc, const char *path) {
            /* Music init */
            R_TRY(audoutStartAudioOut());
            ON_SCOPE_EXIT { audoutStopAudioOut(); };

            MPG_TRY(mpg123_init());
            ON_SCOPE_EXIT { mpg123_exit(); };

            int err;
            music_handle = mpg123_new(nullptr, &err);
            MPG_TRY(err);

            /* Set parameters. */
            MPG_TRY(mpg123_param(music_handle, MPG123_FORCE_RATE, audoutGetSampleRate(), 0));
            MPG_TRY(mpg123_param(music_handle, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO, 0));
            MPG_TRY(mpg123_volume(music_handle, 0.5));

            /* Open file. */
            MPG_TRY(mpg123_open(music_handle, path));
            ON_SCOPE_EXIT { mpg123_close(music_handle); };

            long rate;
            int channels, encoding;
            MPG_TRY(mpg123_getformat(music_handle, &rate, &channels, &encoding));
            MPG_TRY(mpg123_format_none(music_handle));
            MPG_TRY(mpg123_format(music_handle, rate, channels, encoding));

            off_t length_samp = mpg123_length(music_handle);
            R_UNLESS(length_samp != MPG123_ERR, ResultMpgFailure());

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

            size_t done;

            /* Read initial buffer. */
            MPG_TRY(mpg123_read(music_handle, (u8 *)audio_buffer[0].buffer, buffer_size, &done));
            audio_buffer[0].data_size = done;
            /* Applend buffer. */
            R_TRY(audoutAppendAudioOutBuffer(&audio_buffer[0]));

            size_t index = 1;
            while (true) {
                /* Read decoded audio. */
                MPG_TRY(mpg123_read(music_handle, (u8 *)audio_buffer[index].buffer, buffer_size, &done));
                audio_buffer[index].data_size = done;

                /* Check if not supposed to be playing. */
                while (g_status != PlayerStatus::Playing) {
                    /* Return if not paused. Exit or Stop requested. */
                    if (g_status != PlayerStatus::Paused) {
                        return ResultSuccess();
                    }
                    /* Sleep if paused. */
                    svcSleepThread(100'000'000ul);
                }
                /* Append the decoded audio buffer. */
                R_TRY(audoutAppendAudioOutBuffer(&audio_buffer[index]));

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
        g_status = PlayerStatus::Stopped;
    }

    void Exit() {
        g_status = PlayerStatus::Exit;
    }

    void ThreadFunc(void *) {
        bool has_next = false;
        char absolute_path[FS_MAX_PATH];

        FILE *file = fopen("sdmc:/music.log", "a");
        if (file) {
            fputs("Thread start\n", file);
            fclose(file);
        }

        /* Run as long as we aren't stopped and no error has been encountered. */
        while (g_status != PlayerStatus::Exit) {
            if (!has_next) {
                /* Obtain path to next file and pop it of the queue. */
                std::scoped_lock lk(g_mutex);
                has_next = !g_queue.empty();
                if (has_next) {
                    g_current = g_queue.front();
                    g_queue.pop();
                    std::snprintf(absolute_path, FS_MAX_PATH, "sdmc:%s", g_current.c_str());

                    file = fopen("sdmc:/music.log", "a");
                    if (file) {
                        fprintf(file, "next song: %s\n", absolute_path);
                        fclose(file);
                    }
                }
            }
            /* Only play if not stopped and we have a song queued. */
            if (g_status == PlayerStatus::Playing && has_next) {
                const char *mpg_desc = nullptr;
                Result rc = ThreadFuncImpl(&mpg_desc, absolute_path);
                /* Log error. */
                if (R_FAILED(rc)) {
                    file = fopen("sdmc:/music.log", "a");
                    if (!file)
                        continue;

                    if (rc.GetValue() == ResultMpgFailure().GetValue()) {
                        if (!mpg_desc)
                            mpg_desc = "UNKNOWN";
                        fprintf(file, "mpg error: %s\n", mpg_desc);
                    } else {
                        fprintf(file, "other error: 0x%x, 2%03X-%04X\n", rc.GetValue(), rc.GetModule(), rc.GetDescription());
                    }
                    fclose(file);
                }
                has_next = false;
                absolute_path[0] = '\0';
            } else {
                /* Nothing queued. Let's sleep. */
                svcSleepThread(1'000'000'000ul);
            }
        }

        file = fopen("sdmc:/music.log", "a");
        if (file) {
            fputs("Thread stop\n", file);
            fclose(file);
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

    Result GetQueueCountImpl(u32 *out) {
        *out = g_queue.size();
        return ResultSuccess();
    }

    Result AddToQueueImpl(const char *path, size_t path_length) {
        /* Maps a mp3 file? */
        std::regex matcher("^(/.*.mp3)$");
        R_UNLESS(std::regex_match(path, matcher), ResultInvalidPath());

        std::scoped_lock lk(g_mutex);

        /* Add song to queue. */
        g_queue.push(path);

        return ResultSuccess();
    }

    Result GetNextImpl(char *out_path, size_t out_path_length) {
        std::scoped_lock lk(g_mutex);

        /* Make sure queue isn't empty. */
        R_UNLESS(!g_queue.empty(), ResultQueueEmpty());

        const auto &next = g_queue.front();

        /* Path length sufficient? */
        R_UNLESS(out_path_length >= next.length(), ResultInvalidArgument());
        std::strcpy(out_path, next.c_str());

        return ResultSuccess();
    }

    Result GetLastImpl(char *out_path, size_t out_path_length) {
        std::scoped_lock lk(g_mutex);

        /* Make sure queue isn't empty. */
        R_UNLESS(!g_queue.empty(), ResultQueueEmpty());

        const auto &last = g_queue.back();

        /* Path length sufficient? */
        R_UNLESS(out_path_length >= last.length(), ResultInvalidArgument());
        std::strcpy(out_path, last.c_str());

        return ResultSuccess();
    }

    Result ClearImpl() {
        std::scoped_lock lk(g_mutex);

        while (!g_queue.empty())
            g_queue.pop();

        return ResultSuccess();
    }

}
