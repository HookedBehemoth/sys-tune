#include "music_player.hpp"

#include "../tune_result.hpp"
#include "../tune_service.hpp"
#include "sdmc/sdmc.hpp"
#include "pm/pm.hpp"
#include "aud_wrapper.h"
#include "config/config.hpp"
#include "source.hpp"

#include <cstring>
#include <nxExt.h>

namespace tune::impl {

    namespace {
        enum class AudrenCloseState {
            None, // no change
            Open, // just opened audren
            Close, // just closed audren
        };

        constexpr float VOLUME_MAX = 1.f;

        std::vector<PlaylistEntry>* g_playlist;
        std::vector<PlaylistID>* g_shuffle_playlist;
        PlaylistEntry* g_current;
        u32 g_queue_position;
        LockableMutex g_mutex;

        RepeatMode g_repeat   = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status = PlayerStatus::FetchNext;
        Source *g_source = nullptr;

        float g_tune_volume = 1.f;
        float g_title_volume = 1.f;
        float g_default_title_volume = 1.f;
        bool g_use_title_volume = true;

        AudioDriver g_drv;
        constexpr const int MinSampleCount  = 256;
        constexpr const int MaxChannelCount = 8;
        constexpr const int BufferCount     = 2;
        constexpr const int AudioSampleSize = MinSampleCount * MaxChannelCount * sizeof(s16);
        constexpr const int AudioPoolSize   = AudioSampleSize * BufferCount;
        alignas(AUDREN_MEMPOOL_ALIGNMENT) u8 AudioMemoryPool[AudioPoolSize];

        static_assert((sizeof(AudioMemoryPool) % 0x2000) == 0, "Audio Memory pool needs to be page aligned!");

        bool g_should_pause = false;
        bool g_should_run   = true;
        bool g_close_audren = false;

        Result audioInit() {
            /* Default audio config. */
            const AudioRendererConfig audren_cfg = {
                .output_rate = AudioRendererOutputRate_48kHz,
                .num_voices = 2,
                .num_effects = 0,
                .num_sinks = 1,
                .num_mix_objs = 1,
                .num_mix_buffers = 2,
            };

            smInitialize();
            Result rc = audrenInitialize(&audren_cfg);
            smExit();

            if (R_SUCCEEDED(rc)) {
                /* Create audio driver. */
                rc = audrvCreate(&g_drv, &audren_cfg, 2);
                if (R_SUCCEEDED(rc)) {
                    /* Register memory pool. */
                    int mpid = audrvMemPoolAdd(&g_drv, AudioMemoryPool, AudioPoolSize);
                    audrvMemPoolAttach(&g_drv, mpid);

                    /* Attach default sink. */
                    u8 sink_channels[] = {0, 1};
                    audrvDeviceSinkAdd(&g_drv, AUDREN_DEFAULT_DEVICE_NAME, 2, sink_channels);

                    rc = audrvUpdate(&g_drv);
                    if (R_SUCCEEDED(rc)) {
                        return audrenStartAudioRenderer();
                    } else {
                        /* Cleanup on failure */
                        audrvClose(&g_drv);
                    }
                } else {
                    /* Cleanup on failure */
                    audrenExit();
                }
            }

            return rc;
        }

        // Only call this from audrv thread, as closing audrv
        // while accesing it will be very bad.
        AudrenCloseState PollAudrenCloseState() {
            static bool close_audren_previous = false;

            if (close_audren_previous != g_close_audren) {
                close_audren_previous = g_close_audren;
                if (g_close_audren) {
                    audrvClose(&g_drv);
                    audrenExit();
                    return AudrenCloseState::Close;
                } else {
                    audioInit();
                    SetVolume(g_tune_volume);
                    return AudrenCloseState::Open;
                }
            }

            return AudrenCloseState::None;
        }

        Result PlayTrack(const std::string &path) {
            /* Open file and allocate */
            auto source = OpenFile(path.c_str());
            R_UNLESS(source != nullptr, tune::FileOpenFailure);
            R_UNLESS(source->IsOpen(), tune::FileOpenFailure);

            const auto channel_count = source->GetChannelCount();
            const auto sample_rate   = source->GetSampleRate();

            const auto voice_init = [&]() -> Result {
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

                return 0;
            };

            if (auto rc = voice_init(); R_FAILED(rc)) {
                return rc;
            }

            const s32 sample_count                  = AudioSampleSize / channel_count / sizeof(s16);
            AudioDriverWaveBuf buffers[BufferCount] = {};

            for (int i = 0; i < BufferCount; i++) {
                buffers[i].data_pcm16          = reinterpret_cast<s16 *>(&AudioMemoryPool);
                buffers[i].size                = AudioSampleSize;
                buffers[i].start_sample_offset = i * sample_count;
                buffers[i].end_sample_offset   = buffers[i].start_sample_offset + sample_count;
            }

            g_source = source.get();

            while (g_should_run && g_status == PlayerStatus::Playing) {
                switch (PollAudrenCloseState()) {
                    case AudrenCloseState::None:
                        break;
                    case AudrenCloseState::Open:
                        if (auto rc = voice_init(); R_FAILED(rc)) {
                            g_source = nullptr;
                            return rc;
                        }
                        break;
                    case AudrenCloseState::Close:
                        for (auto &buffer : buffers) {
                            buffer.state = AudioDriverWaveBufState_Free;
                        }
                        break;
                }

                if (g_close_audren) {
                    svcSleepThread(100'000'000ul);
                    continue;
                }

                if (g_should_pause) {
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

    } // namespace

    Result Initialize(std::vector<PlaylistEntry>* playlist, std::vector<PlaylistID>* shuffle, PlaylistEntry* current) {
        g_playlist = playlist;
        g_shuffle_playlist = shuffle;
        g_current = current;

        // tldr, most fancy things made by N will fatal
        const u64 blacklist[] = {
            0x010077B00E046000, // spyro reignited trilogy
            0x0100AD9012510000, // pac man 99
            0x01006C100EC08000, // minecraft dugeons
            0x01000A10041EA000, // skyrim
            0x0100F9F00C696000, // crash team racing nitro fueled
            0x01001E9003502000, // labo 03
            0x0100165003504000, // labo 04
            0x0100F2300D4BA000, // darksiders genesis
            0x0100E1400BA96000, // darksiders warmastered edition
            0x010071800BA98000, // darksiders 2
            0x0100F8F014190000, // darksiders 3
            0x0100D870045B6000, // NES NSO
            0x01008D300C50C000, // SNES NSO
            0x0100C62011050000, // GB NSO
            0x010012F017576000, // GBA NSO
            0x0100C9A00ECE6000, // N64 NSO
        };

        // do this on startup because the user may not copy a config
        // file or delete it at somepoint
        for (auto tid : blacklist) {
            config::set_title_blacklist(tid, true);
        }

        if (auto rc = audioInit(); R_FAILED(rc)) {
            return rc;
        }

        /* Fetch values from config, sanitize the return value */
        if (auto c = config::get_repeat(); c <= 2 && c >= 0) {
            SetRepeatMode(static_cast<RepeatMode>(c));
        }

        SetShuffleMode(static_cast<ShuffleMode>(config::get_shuffle()));
        SetVolume(config::get_volume());
        SetDefaultTitleVolume(config::get_default_title_volume());

        return 0;

    }

    void Exit() {
        g_should_run = false;
    }

    void TuneThreadFunc(void *) {
        /* Run as long as we aren't stopped and no error has been encountered. */
        while (g_should_run) {
            // update g_close_audren, returned state isn't needed
            PollAudrenCloseState();

            if (g_close_audren) {
                svcSleepThread(100'000'000ul);
                continue;
            }

            g_current->path = "";
            {
                std::scoped_lock lk(g_mutex);

                const auto &queue = *g_playlist;
                const auto queue_size = queue.size();
                if (queue_size == 0) {
                    g_current->path = "";
                } else if (g_queue_position >= queue_size) {
                    g_queue_position = queue_size - 1;
                    continue;
                } else {
                    if (g_shuffle == ShuffleMode::On) {
                        const auto shuffle_id = (*g_shuffle_playlist)[g_queue_position];
                        for (u32 i = 0; i < g_playlist->size(); i++) {
                            if ((*g_playlist)[i].id == shuffle_id) {
                                *g_current = (*g_playlist)[i];
                                break;
                            }
                        }
                    } else {
                        *g_current = queue[g_queue_position];
                    }
                }
            }

            /* Sleep if queue is empty. */
            if (g_current->path.empty()) {
                svcSleepThread(100'000'000ul);
                continue;
            }

            g_status = PlayerStatus::Playing;
            /* Only play if playing and we have a track queued. */
            Result rc = PlayTrack(g_current->path);

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

        if (!g_close_audren) {
            audrvClose(&g_drv);
            // this needs to be closed asap if a blacklisted title is launched.
            // this is why we close this here rather than in __appExit
            audrenExit();
        }
    }

    void PscmThreadFunc(void *ptr) {
        PscPmModule *module = static_cast<PscPmModule *>(ptr);
        bool previous_state{};

        while (g_should_run) {
            Result rc = eventWait(&module->event, 10'000'000);
            if (R_VALUE(rc) == KERNELRESULT(TimedOut))
                continue;
            if (R_VALUE(rc) == KERNELRESULT(Cancelled))
                break;

            PscPmState state;
            u32 flags;
            R_ABORT_UNLESS(pscPmModuleGetRequest(module, &state, &flags));
            switch (state) {
                // NOTE: PscPmState_Awake event seems to get missed (rare) or
                // PscPmState_ReadySleep is sent multiple times.
                // todo: fade in and delay playback on wakeup slightly
                case PscPmState_ReadyAwaken:
                    g_should_pause = previous_state;
                    break;
                // pause on sleep
                case PscPmState_ReadySleep:
                    previous_state = g_should_pause;
                    g_should_pause = true;
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

        while (g_should_run) {
            /* Fetch current gpio value. */
            GpioValue value;
            if (R_SUCCEEDED(gpioPadGetValue(session, &value))) {
                if (old_value == GpioValue_Low && value == GpioValue_High) {
                    pre_unplug_pause = g_should_pause;
                    g_should_pause     = true;
                } else if (old_value == GpioValue_High && value == GpioValue_Low) {
                    if (!pre_unplug_pause)
                        g_should_pause = false;
                }
                old_value = value;
            }
            svcSleepThread(10'000'000);
        }
    }

    void PmdmntThreadFunc(void *ptr) {
        u64 previous_tid{};
        u64 current_tid{};
        bool previous_state{};

        while (g_should_run) {
            u64 pid{}, new_tid{};
            if (pm::PollCurrentPidTid(pid, new_tid)) {
                // check if title is blacklisted
                g_close_audren = config::get_title_blacklist(new_tid);

                g_title_volume = 1.f;

                if (config::has_title_volume(new_tid)) {
                    g_use_title_volume = true;
                    SetTitleVolume(std::clamp(config::get_title_volume(new_tid), 0.f, VOLUME_MAX));
                }

                if (new_tid == previous_tid) {
                    g_should_pause = previous_state;
                } else {
                    previous_state = g_should_pause;
                    // TODO: fade song in rather than abruptly playing to avoid jump scares
                    if (config::has_title_enabled(new_tid)) {
                        g_should_pause = !config::get_title_enabled(new_tid);
                    } else {
                        g_should_pause = !config::get_title_enabled_default();
                    }
                }

                previous_tid = current_tid;
                current_tid = new_tid;
            }

            // sadly, we can't simply apply auda when the title changes
            // as it seems to apply to quickly, before the title opens audio
            // services, so the changes don't apply.
            // best option is to repeatdly set the out :/
            if (pid) {
                const auto v = g_use_title_volume ? g_title_volume : g_default_title_volume;
                audWrapperSetProcessMasterVolume(pid, 0, v);
                // audWrapperSetProcessRecordVolume(pid, 0, v);
            }

            svcSleepThread(10'000'000);
        }
    }

    bool GetStatus() {
        return !g_should_pause;
    }

    void Play() {
        g_should_pause = false;
    }

    void Pause() {
        g_should_pause = true;
    }

    void Next() {
        bool pause = false;
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position < g_playlist->size() - 1) {
                g_queue_position++;
            } else {
                g_queue_position = 0;
                if (g_repeat == RepeatMode::Off)
                    pause = true;
            }
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_pause = pause;
    }

    void Prev() {
        {
            std::scoped_lock lk(g_mutex);

            if (g_queue_position > 0) {
                g_queue_position--;
            } else {
                g_queue_position = g_playlist->size() - 1;
            }
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_pause = false;
    }

    float GetVolume() {
        return g_drv.in_mixes[0].volume;
    }

    void SetVolume(float volume) {
        volume = std::clamp(volume, 0.f, VOLUME_MAX);
        g_tune_volume = g_drv.in_mixes[0].volume = volume;
        config::set_volume(volume);
    }

    float GetTitleVolume() {
        return g_title_volume;
    }

    void SetTitleVolume(float volume) {
        volume = std::clamp(volume, 0.f, VOLUME_MAX);
        g_title_volume = volume;
        g_use_title_volume = true;
    }

    float GetDefaultTitleVolume() {
        return g_default_title_volume;
    }

    void SetDefaultTitleVolume(float volume) {
        volume = std::clamp(volume, 0.f, VOLUME_MAX);
        g_default_title_volume = volume;
        config::set_default_title_volume(volume);
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

        // if (g_playlist->size() > 0 && g_shuffle != mode) {
        //     auto &dst = (mode == ShuffleMode::On) ? *g_shuffle_playlist : *g_playlist;

        //     auto it = std::find(dst.cbegin(), dst.cend(), *g_current);
        //     if (it != dst.cend())
        //         g_queue_position = std::distance(dst.cbegin(), it);
        // }

        g_shuffle = mode;
    }

    u32 GetPlaylistSize() {
        std::scoped_lock lk(g_mutex);

        return g_playlist->size();
    }

    Result GetPlaylistItem(u32 index, char *buffer, size_t buffer_size) {
        std::scoped_lock lk(g_mutex);

        if (index >= g_playlist->size())
            return tune::OutOfRange;

        std::strncpy(buffer, (*g_playlist)[index].path.c_str(), buffer_size);

        return 0;
    }

    Result GetCurrentQueueItem(CurrentStats *out, char *buffer, size_t buffer_size) {
        R_UNLESS(g_source != nullptr, tune::NotPlaying);
        R_UNLESS(g_source->IsOpen(), tune::NotPlaying);

        {
            std::scoped_lock lk(g_mutex);
            R_UNLESS(!g_current->path.empty(), tune::NotPlaying);
            R_UNLESS(buffer_size >= g_current->path.size(), tune::InvalidArgument);
            std::strcpy(buffer, g_current->path.c_str());
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

            g_playlist->clear();
            g_shuffle_playlist->clear();
        }
        g_status = PlayerStatus::FetchNext;
    }

    void MoveQueueItem(u32 src, u32 dst) {
        std::scoped_lock lk(g_mutex);

        const auto queue_size = g_playlist->size();

        if (src >= queue_size) {
            src = queue_size - 1;
        }
        if (dst >= queue_size) {
            dst = queue_size - 1;
        }

        auto source = g_playlist->cbegin() + src;
        auto dest   = g_playlist->cbegin() + dst;

        g_playlist->insert(dest, *source);
        g_playlist->erase(source);

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
            size_t queue_size = g_playlist->size();
            if (index >= queue_size) {
                index = queue_size - 1;
            }

            /* Get absolute position in current playlist. Independent of shufflemode. */
            u32 pos = index;

            if (g_shuffle == ShuffleMode::On) {
                const auto track = g_playlist->cbegin() + index;
                for (u32 i = 0; i < g_shuffle_playlist->size(); i++) {
                    if ((*g_shuffle_playlist)[i] == track->id) {
                        pos = i;
                        break;
                    }
                }
            }

            /* Return if that track is already selected. */
            if (g_queue_position == pos)
                return;

            g_queue_position = pos;
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_pause = false;
    }

    void Seek(u32 position) {
        if (g_source != nullptr && g_source->IsOpen())
            g_source->Seek(position);
    }

    Result Enqueue(const char *buffer, size_t buffer_length, EnqueueType type) {
        // NOTE: do not decrement this
        static PlaylistID playlist_id{};

        /* Ensure file exists. */
        if (!sdmc::FileExists(buffer))
            return tune::InvalidPath;

        std::scoped_lock lk(g_mutex);

        const PlaylistEntry new_entry{
            .path = {buffer, buffer_length},
            .id = playlist_id
        };

        // add new entry to playlist
        if (type == EnqueueType::Front) {
            g_playlist->emplace(g_playlist->cbegin(), new_entry);
            if (g_shuffle == ShuffleMode::Off) {
                g_queue_position++;
            }
        } else {
            g_playlist->emplace_back(new_entry);
        }

        // add new entry id to shuffle_playlist_list
        const auto shuffle_playlist_size = g_shuffle_playlist->size();
        const auto shuffle_index = (shuffle_playlist_size > 1) ? (randomGet64() % shuffle_playlist_size) : 0;
        g_shuffle_playlist->emplace(g_shuffle_playlist->cbegin() + shuffle_index, playlist_id);

        // increase playlist counter
        playlist_id++;

        return 0;
    }

    Result Remove(u32 index) {
        std::scoped_lock lk(g_mutex);

        /* Ensure we don't operate out of bounds. */
        R_UNLESS(!g_playlist->empty(), tune::QueueEmpty);
        R_UNLESS(index < g_playlist->size(), tune::OutOfRange);

        /* Get iterator for index position. */
        const auto track = g_playlist->cbegin() + index;

        for (u32 i = 0; i < g_shuffle_playlist->size(); i++) {
            if ((*g_shuffle_playlist)[i] == track->id) {
                const auto shuffle_it = g_shuffle_playlist->cbegin() + i;
                // we are playing from shuffle list so use that index instead
                if (g_shuffle == ShuffleMode::On) {
                    index = i;
                }
                // finally remove
                g_shuffle_playlist->erase(shuffle_it);
                break;
            }
        }

        /* Remove entry. */
        g_playlist->erase(track);

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
