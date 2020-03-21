#include "music_player.hpp"

#include "../music_result.hpp"
#include "../scoped_file.hpp"

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
        constexpr const s64 ReadSize = 0x10000;
        u8 buffer[ReadSize];

        ams::Result ThreadFuncImpl(const char **mpg123_desc, const char *path) {
            MPG_TRY(mpg123_init());

            /* Allocate music parameters. */
            int err;
            auto *pars = mpg123_new_pars(&err);
            MPG_TRY(err);
            ON_SCOPE_EXIT { mpg123_delete_pars(pars); };

            /* Set parameters. */
            MPG_TRY(mpg123_par(pars, MPG123_FORCE_RATE, audoutGetSampleRate(), 0));
            MPG_TRY(mpg123_par(pars, MPG123_ADD_FLAGS, MPG123_FORCE_STEREO, 0));

            /* Allocate music handle. */
            music_handle = mpg123_parnew(pars, nullptr, &err);
            MPG_TRY(err);
            ON_SCOPE_EXIT { mpg123_delete(music_handle); };

            /* Open mp3 feed. */
            MPG_TRY(mpg123_open_feed(music_handle));

            fs::FileHandle mp3_file;
            R_TRY(fs::OpenFile(&mp3_file, path, fs::OpenMode_Read));

            s64 file_size;
            R_TRY(fs::GetFileSize(&file_size, mp3_file));

            R_TRY(audoutStartAudioOut());
            ON_SCOPE_EXIT { audoutStopAudioOut(); };

            constexpr const u32 buffer_size = 0x10000;
            u8 *audio_memory = (u8 *)memalign(0x1000, buffer_size);
            R_UNLESS(audio_memory, MAKERESULT(Module_Libnx, LibnxError_OutOfMemory));

            AudioOutBuffer audio_buffer{
                .buffer = audio_memory,
            };

            s64 pos = 0;
            while (pos < file_size) {
                size_t read_bytes;
                R_TRY(fs::ReadFile(&read_bytes, mp3_file, pos, buffer, ReadSize));

                size_t done;
                int err = mpg123_decode(music_handle, buffer, read_bytes, audio_memory, buffer_size, &done);
                if (err == MPG123_NEED_MORE)
                    continue;
                audio_buffer.buffer_size = done;
                audio_buffer.data_size = done;

                /* Check if not supposed to be playing. */
                while (g_status != PlayerStatus::Playing) {
                    /* Return if not paused. Exit or Stop requested. */
                    if (g_status != PlayerStatus::Paused) {
                        return ResultSuccess();
                    }
                    /* Sleep if paused. */
                    svcSleepThread(100'000'000ul);
                }

                AudioOutBuffer *released;
                u32 released_count;
                R_TRY(audoutWaitPlayFinish(&released, &released_count, 1'000'000'000ul));
                /* Append the decoded audio buffer. */
                R_TRY(audoutAppendAudioOutBuffer(&audio_buffer));

                pos += read_bytes;
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

        {
            ScopedFile log("sdmc:/music.log");
            log.WriteString("Thread start\n");
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

                    {
                        ScopedFile log("sdmc:/music.log");
                        log.WriteFormat("next song: %s\n", absolute_path);
                    }
                }
            }
            /* Only play if not stopped and we have a song queued. */
            if (g_status == PlayerStatus::Playing && has_next) {
                const char *mpg_desc = nullptr;
                Result rc = ThreadFuncImpl(&mpg_desc, absolute_path);
                /* Log error. */
                if (R_FAILED(rc)) {
                    ScopedFile log("sdmc:/music.log");
                    if (rc.GetValue() == ResultMpgFailure().GetValue()) {
                        if (!mpg_desc) {
                            mpg_desc = "UNKNOWN";
                        }
                        log.WriteFormat("mpg error: %s\n", mpg_desc);
                    } else {
                        log.WriteFormat("other error: 0x%x, 2%03X-%04X\n", rc.GetValue(), rc.GetModule(), rc.GetDescription());
                    }
                    g_status = PlayerStatus::Exit;
                    break;
                } else {
                    has_next = false;
                    absolute_path[0] = '\0';
                }
            } else {
                /* Nothing queued. Let's sleep. */
                svcSleepThread(1'000'000'000ul);

                {
                    ScopedFile log("sdmc:/music.log");
                    log.WriteString("Thread sleep\n");
                }
            }
        }

        {
            ScopedFile log("sdmc:/music.log");
            log.WriteString("Thread stop\n");
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
