#include "music_player.hpp"

#include "../tune_result.hpp"
#include "sdmc.hpp"
#include "source.hpp"

#include <algorithm>
#include <cstring>
#include <nxExt.h>
#include <string>
#include <vector>

Source *g_source = nullptr;

namespace tune::impl {

    namespace {

        std::vector<std::string> g_playlist;
        std::vector<std::string> g_shuffle_playlist;
        std::string g_current = "";
        u32 g_queue_position;
        LockableMutex g_mutex;

        RepeatMode g_repeat   = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status = PlayerStatus::FetchNext;

        AudioDriver g_drv;
        constexpr const int MinSampleCount  = 256;
        constexpr const int MaxChannelCount = 8;
        constexpr const int BufferCount     = 2;
        constexpr const int AudioSampleSize = MinSampleCount * MaxChannelCount * sizeof(s16);
        constexpr const int AudioPoolSize   = AudioSampleSize * BufferCount;
        alignas(0x1000) u8 AudioMemoryPool[AudioPoolSize];

        static_assert((sizeof(AudioMemoryPool) % 0x2000) == 0, "Audio Memory pool needs to be page aligned!");

        bool should_pause = false;
        bool should_run   = true;

        Result PlayTrack(const std::string &path) {
            /* Open file and allocate */
            auto source = std::unique_ptr<Source>(OpenFile(path.c_str()));
            R_UNLESS(source != nullptr, tune::FileOpenFailure);
            R_UNLESS(source->IsOpen(), tune::FileOpenFailure);

            int channel_count = source->GetChannelCount();
            int sample_rate   = source->GetSampleRate();

            R_UNLESS(audrvVoiceInit(&g_drv, 0, channel_count, PcmFormat_Int16, sample_rate), tune::VoiceInitFailure);

            audrvVoiceSetDestinationMix(&g_drv, 0, AUDREN_FINAL_MIX_ID);

            if (channel_count == 1) {
                audrvVoiceSetMixFactor(&g_drv, 0, 1.0f, 0, 0);
                audrvVoiceSetMixFactor(&g_drv, 0, 1.0f, 0, 1);
            } else {
                audrvVoiceSetMixFactor(&g_drv, 0, 1.0f, 0, 0);
                audrvVoiceSetMixFactor(&g_drv, 0, 0.0f, 0, 1);
                audrvVoiceSetMixFactor(&g_drv, 0, 0.0f, 1, 0);
                audrvVoiceSetMixFactor(&g_drv, 0, 1.0f, 1, 1);
            }

            audrvVoiceStart(&g_drv, 0);

            const s32 sample_count                  = AudioSampleSize / channel_count / sizeof(s16);
            AudioDriverWaveBuf buffers[BufferCount] = {};

            for (int i = 0; i < BufferCount; i++) {
                buffers[i].data_pcm16          = reinterpret_cast<s16 *>(&AudioMemoryPool);
                buffers[i].size                = AudioSampleSize;
                buffers[i].start_sample_offset = i * sample_count;
                buffers[i].end_sample_offset   = buffers[i].start_sample_offset + sample_count;
            }

            g_source = source.get();

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
                    s16 *data = reinterpret_cast<s16 *>(&AudioMemoryPool) + refillBuf->start_sample_offset * 2;

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

            audrvVoiceDrop(&g_drv, 0);
            g_source = nullptr;

            return 0;
        }

    }

    Result Initialize() {
        /* Create audio driver. */
        Result rc = audrvCreate(&g_drv, &audren_cfg, 2);
        if (R_SUCCEEDED(rc)) {
            /* Register memory pool. */
            int mpid = audrvMemPoolAdd(&g_drv, AudioMemoryPool, AudioPoolSize);
            audrvMemPoolAttach(&g_drv, mpid);

            /* Attach default sink. */
            u8 sink_channels[] = {0, 1};
            audrvDeviceSinkAdd(&g_drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

            rc = audrvUpdate(&g_drv);
            if (R_SUCCEEDED(rc)) {
                rc = audrenStartAudioRenderer();
                if (R_SUCCEEDED(rc))
                    return 0;
            } else {
                /* Cleanup on failure */
                audrvClose(&g_drv);
            }
        }

        return rc;
    }

    void Exit() {
        should_run = false;
    }

    void TuneThreadFunc(void *) {
        /* Run as long as we aren't stopped and no error has been encountered. */
        while (should_run) {
            g_current = "";
            {
                std::scoped_lock lk(g_mutex);

                auto &queue       = (g_shuffle == ShuffleMode::On) ? g_shuffle_playlist : g_playlist;
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

            /* Sleep if queue is empty. */
            if (g_current.empty()) {
                svcSleepThread(100'000'000ul);
                continue;
            }

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
            }
        }

        audrvClose(&g_drv);
    }

    void PscmThreadFunc(void *ptr) {
        PscPmModule *module = static_cast<PscPmModule *>(ptr);

        while (should_run) {
            Result rc = eventWait(&module->event, 10'000'000);
            if (R_VALUE(rc) == KERNELRESULT(TimedOut))
                continue;
            if (R_VALUE(rc) == KERNELRESULT(Cancelled))
                break;

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
                    should_pause     = true;
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
        g_status     = PlayerStatus::FetchNext;
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
        g_status     = PlayerStatus::FetchNext;
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

    u32 GetPlaylistSize() {
        std::scoped_lock lk(g_mutex);

        return g_playlist.size();
    }

    Result GetPlaylistItem(u32 index, char *buffer, size_t buffer_size) {
        std::scoped_lock lk(g_mutex);

        if (index >= g_playlist.size())
            return tune::OutOfRange;

        strncpy(buffer, g_playlist[index].c_str(), buffer_size);

        return 0;
    }

    Result GetCurrentQueueItem(CurrentStats *out, char *buffer, size_t buffer_size) {
        R_UNLESS(g_source != nullptr, tune::NotPlaying);
        R_UNLESS(g_source->IsOpen(), tune::NotPlaying);

        {
            std::scoped_lock lk(g_mutex);
            R_UNLESS(!g_current.empty(), tune::NotPlaying);
            R_UNLESS(buffer_size >= g_current.size(), tune::InvalidArgument);
            std::strcpy(buffer, g_current.c_str());
        }

        auto [current, total] = g_source->Tell();
        int sample_rate       = g_source->GetSampleRate();

        out->sample_rate   = sample_rate;
        out->current_frame = current;
        out->total_frames  = total;

        return 0;
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
        auto dest   = g_playlist.cbegin() + dst;

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
        g_status     = PlayerStatus::FetchNext;
        should_pause = false;
    }

    void Seek(u32 position) {
        if (g_source != nullptr && g_source->IsOpen())
            g_source->Seek(position);
    }

    Result Enqueue(const char *buffer, size_t buffer_length, EnqueueType type) {
        /* Ensure file exists. */
        if (!sdmc::FileExists(buffer))
            return tune::InvalidPath;

        std::scoped_lock lk(g_mutex);

        if (type == EnqueueType::Front) {
            g_playlist.emplace(g_playlist.cbegin(), buffer, buffer_length);
            g_queue_position++;
        } else {
            g_playlist.emplace_back(buffer, buffer_length);
        }
        size_t shuffle_playlist_size = g_shuffle_playlist.size();
        size_t shuffle_index         = (shuffle_playlist_size > 1) ? (randomGet64() % shuffle_playlist_size) : 0;
        g_shuffle_playlist.emplace(g_shuffle_playlist.cbegin() + shuffle_index, buffer, buffer_length);

        return 0;
    }

    Result Remove(u32 index) {
        std::scoped_lock lk(g_mutex);

        /* Ensure we don't operate out of bounds. */
        R_UNLESS(!g_playlist.empty(), tune::QueueEmpty);
        R_UNLESS(index < g_playlist.size(), tune::OutOfRange);

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

        return 0;
    }

}
