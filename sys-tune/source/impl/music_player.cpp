#include "music_player.hpp"

#include "../music_result.hpp"

#include <list>
#include <string>

#include "source.hpp"

Source *g_source = nullptr;

namespace ams::tune::impl {

    namespace {

        std::vector<std::string> g_playlist;
        std::vector<std::string> g_shuffle_playlist;
        std::string g_current = "";
        u32 g_queue_position;
        os::Mutex g_mutex(false);

        RepeatMode g_repeat = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status = PlayerStatus::FetchNext;

        AudioDriver g_drv;
        u8 *audMemPool = nullptr;
        constexpr const int MinSampleCount = 256;
        constexpr const int MaxChannelCount = 8;
        constexpr const int BufferCount = 2;
        constexpr const int AudioSampleSize = MinSampleCount * MaxChannelCount * sizeof(s16);

        bool should_pause = false;
        bool should_run = true;

        Result PlayTrack(const std::string &path) {
            /* Open file and allocate */
            Source *source = OpenFile(path.c_str());
            R_UNLESS(source != nullptr, ResultFileOpenFailure());

            ON_SCOPE_EXIT { delete source; };
            R_UNLESS(source->IsOpen(), ResultFileOpenFailure());

            R_UNLESS(audrvVoiceInit(&g_drv, 0, source->GetChannelCount(), PcmFormat_Int16, source->GetSampleRate()), ResultAudrvVoiceInitFailure());
            ON_SCOPE_EXIT { audrvVoiceDrop(&g_drv, 0); };

            audrvVoiceSetDestinationMix(&g_drv, 0, AUDREN_FINAL_MIX_ID);

            int channel_count = source->GetChannelCount();

            for (int src = 0; src < channel_count; src++) {
                audrvVoiceSetMixFactor(&g_drv, 0, 1.0f, src, src % 2);
            }
            audrvVoiceStart(&g_drv, 0);

            const s32 sample_count = AudioSampleSize / channel_count / sizeof(s16);
            AudioDriverWaveBuf buffers[2] = {};

            for (int i = 0; i < 2; i++) {
                buffers[i].data_pcm16 = (s16 *)audMemPool;
                buffers[i].size = AudioSampleSize;
                buffers[i].start_sample_offset = i * sample_count;
                buffers[i].end_sample_offset = buffers[i].start_sample_offset + sample_count;
            }

            g_source = source;
            ON_SCOPE_EXIT { g_source = nullptr; };

            while (should_run && g_status == PlayerStatus::Playing) {
                if (should_pause) {
                    svcSleepThread(17'000'000);
                    continue;
                }

                AudioDriverWaveBuf *refillBuf = nullptr;
                for (auto &buffer : buffers) {
                    if (buffer.state == AudioDriverWaveBufState_Free || buffer.state == AudioDriverWaveBufState_Done) {
                        refillBuf = &buffer;
                        break;
                    }
                }

                if (refillBuf) {
                    s16 *data = (s16 *)audMemPool + refillBuf->start_sample_offset * 2;

                    int nSamples = source->Decode(sample_count, data);

                    if (nSamples == 0 && source->Done()) {
                        if (g_repeat != RepeatMode::One)
                            Next();
                        break;
                    }

                    armDCacheFlush(data, nSamples * 2 * sizeof(u16));
                    refillBuf->end_sample_offset = refillBuf->start_sample_offset + nSamples;

                    audrvVoiceAddWaveBuf(&g_drv, 0, refillBuf);
                    audrvVoiceStart(&g_drv, 0);
                }

                audrvUpdate(&g_drv);
                audrenWaitFrame();
            }

            return ResultSuccess();
        }

    }

    Result Initialize() {
        /* Create audio driver. */
        R_TRY(audrvCreate(&g_drv, &audren_cfg, 2));
        auto audrv_guard = SCOPE_GUARD { audrvClose(&g_drv); };

        /* Align memory pool size. */
        const int poolSize = (AudioSampleSize * BufferCount + (AUDREN_MEMPOOL_ALIGNMENT - 1)) & ~(AUDREN_MEMPOOL_ALIGNMENT - 1);

        /* Allocate memory aligned. */
        audMemPool = new (std::align_val_t(AUDREN_MEMPOOL_ALIGNMENT), std::nothrow) u8[poolSize];
        if (audMemPool == nullptr)
            return ResultOutOfMemory();
        auto buffer_guard = SCOPE_GUARD { delete[] audMemPool; };

        /* Register memory pool. */
        int mpid = audrvMemPoolAdd(&g_drv, audMemPool, poolSize);
        audrvMemPoolAttach(&g_drv, mpid);

        /* Attach default sink. */
        constexpr const u8 sink_channels[] = {0, 1};
        audrvDeviceSinkAdd(&g_drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);
        R_TRY(audrvUpdate(&g_drv));

        R_TRY(audrenStartAudioRenderer());

        /* Dismiss our safety guards as everything succeeded. */
        buffer_guard.Cancel();
        audrv_guard.Cancel();

        return ResultSuccess();
    }

    void Exit() {
        should_run = false;
    }

    void AudioThreadFunc(void *) {
        /* Run as long as we aren't stopped and no error has been encountered. */
        while (should_run) {
            g_current = "";
            {
                std::scoped_lock lk(g_mutex);

                auto &queue = (g_shuffle == ShuffleMode::On) ? g_shuffle_playlist : g_playlist;
                size_t queue_size = queue.size();
                if (queue_size == 0) {
                    g_current = "";
                } else if (g_queue_position >= queue_size) {
                    g_queue_position = queue_size - 1;
                    continue;
                } else {
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
                bool shuffle = g_shuffle == ShuffleMode::On;
                if (shuffle)
                    SetShuffleMode(ShuffleMode::Off);
                Remove(g_queue_position);
                if (shuffle)
                    SetShuffleMode(ShuffleMode::On);

                LOG("error: 0x%x, 2%03d-%04d\n", rc.GetValue(), rc.GetModule(), rc.GetDescription());
            }
        }

        delete[] audMemPool;
        audrvClose(&g_drv);
    }

    void PscThreadFunc(void *ptr) {
        PscPmModule *module = static_cast<PscPmModule *>(ptr);

        while (should_run) {
            R_TRY_CATCH(eventWait(&module->event, 10'000'000)) {
                R_CATCH(kern::ResultWaitTimeout) {
                    continue;
                }
                R_CATCH(kern::ResultOperationCanceled) {
                    break;
                }
            }
            R_END_TRY_CATCH_WITH_ASSERT;

            PscPmState state;
            u32 flags;
            R_ABORT_UNLESS(pscPmModuleGetRequest(module, &state, &flags));
            switch (state) {
                case PscPmState_ReadySleep:
                    should_pause = true;
                    break;
                default:
                    break;
            }
            pscPmModuleAcknowledge(module, state);
        }
    }

    void GpioThreadFunc(void *ptr) {
        GpioPadSession *session = static_cast<GpioPadSession *>(ptr);

        bool pre_unplug_pause = false;

        /* [0] Low == plugged in; [1] High == not plugged in. */
        GpioValue old_value = GpioValue_High;

        while (should_run) {
            /* Fetch current gpio value. */
            GpioValue value;
            if (R_SUCCEEDED(gpioPadGetValue(session, &value))) {
                if (old_value == GpioValue_Low && value == GpioValue_High) {
                    pre_unplug_pause = should_pause;
                    should_pause = true;
                } else if (old_value == GpioValue_High && value == GpioValue_Low) {
                    if (!pre_unplug_pause)
                        should_pause = false;
                }
                old_value = value;
            }
            svcSleepThread(10'000'000);
        }
    }

    bool GetStatus() {
        return !should_pause;
    }

    void Play() {
        should_pause = false;
    }

    void Pause() {
        should_pause = true;
    }

    void Next() {
        bool pause = false;
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position < g_playlist.size() - 1) {
                g_queue_position++;
            } else {
                g_queue_position = 0;
                if (g_repeat == RepeatMode::Off)
                    pause = true;
            }
        }
        g_status = PlayerStatus::FetchNext;
        should_pause = pause;
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
        should_pause = false;
    }

    float GetVolume() {
        return g_drv.in_mixes[0].volume;
    }

    void SetVolume(float volume) {
        g_drv.in_mixes[0].volume = volume;
    }

    RepeatMode GetRepeatMode() {
        return g_repeat;
    }

    void SetRepeatMode(RepeatMode mode) {
        g_repeat = mode;
    }

    ShuffleMode GetShuffleMode() {
        return g_shuffle;
    }

    void SetShuffleMode(ShuffleMode mode) {
        std::scoped_lock lk(g_mutex);

        if (g_playlist.size() > 0 && g_shuffle != mode) {
            auto &dst = (mode == ShuffleMode::On) ? g_shuffle_playlist : g_playlist;

            auto it = std::find(dst.cbegin(), dst.cend(), g_current);
            if (it != dst.cend())
                g_queue_position = it - dst.cbegin();
        }

        g_shuffle = mode;
    }

    u32 GetCurrentPlaylistSize() {
        std::scoped_lock lk(g_mutex);

        return g_playlist.size();
    }

    u32 GetCurrentPlaylist(char *buffer, size_t buffer_size) {
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
        return tmp;
    }

    Result GetCurrentQueueItem(CurrentStats *out, char *buffer, size_t buffer_size) {
        R_UNLESS(g_source != nullptr, ResultNotPlaying());
        R_UNLESS(g_source->IsOpen(), ResultNotPlaying());

        {
            std::scoped_lock lk(g_mutex);
            R_UNLESS(!g_current.empty(), ResultNotPlaying());
            R_UNLESS(buffer_size >= g_current.size(), ResultInvalidArgument());
            std::strcpy(buffer, g_current.c_str());
        }

        auto [current, total] = g_source->Tell();
        int sample_rate = g_source->GetSampleRate();

        out->sample_rate = sample_rate;
        out->current_frame = current;
        out->total_frames = total;

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

            /* Check if we are out of bounds. */
            size_t queue_size = g_playlist.size();
            if (index >= queue_size) {
                index = queue_size - 1;
            }

            /* Get absolute position in current playlist. Independent of shufflemode. */
            u32 pos = UINT32_MAX;
            if (g_shuffle == ShuffleMode::On) {
                auto it = std::find(g_shuffle_playlist.cbegin(), g_shuffle_playlist.cend(), g_playlist[index]);
                if (it != g_shuffle_playlist.cend()) {
                    pos = std::distance(it, g_shuffle_playlist.cbegin());
                } else {
                    return;
                }
            } else {
                pos = index;
            }

            /* Return if that track is already selected. */
            if (g_queue_position == pos)
                return;

            g_queue_position = pos;
        }
        g_status = PlayerStatus::FetchNext;
        should_pause = false;
    }

    void Seek(u32 position) {
        if (g_source != nullptr && g_source->IsOpen())
            g_source->Seek(position);
    }

    Result Enqueue(const char *buffer, size_t buffer_length, EnqueueType type) {
        /* Ensure file exists. */
        static ams::fs::FileTimeStampRaw timestamp;
        R_TRY(fs::GetFileTimeStampRaw(&timestamp, buffer));

        std::scoped_lock lk(g_mutex);

        if (type == EnqueueType::Next) {
            g_playlist.emplace(g_playlist.cbegin(), buffer, buffer_length);
            g_queue_position++;
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

        /* Remove entry. */
        g_playlist.erase(track);

        if (shuffle_it != g_shuffle_playlist.cend()) {
            g_shuffle_playlist.erase(shuffle_it);

            if (g_shuffle == ShuffleMode::On)
                index = std::distance(shuffle_it, g_shuffle_playlist.cbegin());
        }

        /* Fetch a new track if we deleted the current song. */
        bool fetch_new = g_queue_position == index;

        /* Lower current position if needed. */
        if (g_queue_position > index) {
            g_queue_position--;
        }

        if (fetch_new)
            g_status = PlayerStatus::FetchNext;

        return ResultSuccess();
    }

}
