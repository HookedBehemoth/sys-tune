#include "music_player.hpp"

#include "../tune_result.hpp"
#include "../tune_service.hpp"
#include "sdmc/sdmc.hpp"
#include "pm/pm.hpp"
#include "aud_wrapper.h"
#include "config/config.hpp"
#include "source.hpp"
#include "resamplers/SDL_audioEX.h"

#include <cstring>
#include <nxExt.h>

namespace tune::impl {

    namespace {
        constexpr float VOLUME_MAX = 1.f;
        constexpr auto PLAYLIST_ENTRY_MAX = 512; // 128k

        struct PlayListEntry2 {
        public:
            // in most cases, the path will not exceed 256 bytes,
            // so this is a reasonable max rather than 0x301.
            bool Add(const char* path) {
                if (!IsEmpty()) {
                    return false;
                }

                if (std::strlen(path) > sizeof(m_path)) {
                    return false;
                }

                std::strcpy(m_path, path);
                return true;
            }

            void Remove() {
                m_path[0] = '\0';
            }

            bool IsEmpty() const {
                return m_path[0] == '\0';
            }

        // private:
            char m_path[256]{};
        };

        struct PlayList {
            std::array<PlayListEntry2, PLAYLIST_ENTRY_MAX> m_entries{};

            bool Add(u32 index, const char* path) {
                if (index > m_entries.size()) {
                    return false;
                }

                return m_entries[index].Add(path);
            }

            void Remove(u32 index) {
                if (index > m_entries.size()) {
                    return;
                }

                return m_entries[index].Remove();
            }

            s32 FindNextFreeEntry() const {
                for (u32 i = 0; i < m_entries.size(); i++) {
                    if (m_entries[i].IsEmpty()) {
                        return i;
                    }
                }

                return -1;
            }

            const char* GetPath(u32 index) {
                return m_entries[index].m_path;
            }

            void Clear() {
                for (u32 i = 0; i < m_entries.size(); i++) {
                    m_entries[i].Remove();
                }
            }
        };

        PlayList g_playlist2;

        // todo: move below into playlist struct
        std::vector<PlaylistEntry> g_playlist;
        std::vector<PlaylistID> g_shuffle_playlist;
        PlaylistEntry g_current;
        u32 g_queue_position;

        LockableMutex g_mutex;

        RepeatMode g_repeat   = RepeatMode::All;
        ShuffleMode g_shuffle = ShuffleMode::Off;
        PlayerStatus g_status = PlayerStatus::FetchNext;
        Source *g_source = nullptr;

        float g_title_volume = 1.f;
        float g_default_title_volume = 1.f;
        bool g_use_title_volume = true;

        constexpr auto AUDIO_FREQ          = 48000;
        constexpr auto AUDIO_CHANNEL_COUNT = 2;
        constexpr auto AUDIO_BUFFER_COUNT  = 2;
        constexpr auto AUDIO_LATENCY_MS    = 50;
        constexpr auto AUDIO_BUFFER_SIZE   = AUDIO_FREQ / 1000 * AUDIO_LATENCY_MS * AUDIO_CHANNEL_COUNT;

        AudioOutBuffer g_audout_buffer[AUDIO_BUFFER_COUNT];
        alignas(0x1000) s16 AudioMemoryPool[AUDIO_BUFFER_COUNT][(AUDIO_BUFFER_SIZE + 0xFFF) & ~0xFFF];
        static_assert((sizeof(AudioMemoryPool[0]) % 0x2000) == 0, "Audio Memory pool needs to be page aligned!");

        bool g_should_pause      = false;
        bool g_should_run        = true;

        Result PlayTrack(const char* path) {
            /* Open file and allocate */
            auto source = OpenFile(path);
            R_UNLESS(source != nullptr, tune::FileOpenFailure);
            R_UNLESS(source->IsOpen(), tune::FileOpenFailure);
            R_UNLESS(source->SetupResampler(audoutGetChannelCount(), audoutGetSampleRate()), tune::VoiceInitFailure);

            AudioOutState state;
            R_TRY(audoutGetAudioOutState(&state));
            if (state == AudioOutState_Stopped) {
                R_TRY(audoutStartAudioOut());
            }

            g_source = source.get();

            while (g_should_run && g_status == PlayerStatus::Playing) {
                if (g_should_pause) {
                    svcSleepThread(17'000'000);
                    continue;
                }

                AudioOutBuffer* buffer = NULL;
                for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
                    bool has_buffer = false;
                    R_TRY(audoutContainsAudioOutBuffer(&g_audout_buffer[i], &has_buffer));
                    if (!has_buffer) {
                        buffer = &g_audout_buffer[i];
                        break;
                    }
                }

                if (!buffer) {
                    u32 released_count;
                    R_TRY(audoutWaitPlayFinish(&buffer, &released_count, UINT64_MAX));
                }

                bool error = false;
                if (buffer) {
                    const auto nSamples = source->Resample((u8*)buffer->buffer, AUDIO_BUFFER_SIZE * sizeof(s16));
                    if (nSamples <= 0) {
                        error = true;
                    } else {
                        buffer->data_size = nSamples;
                        R_TRY(audoutAppendAudioOutBuffer(buffer));
                    }
                }

                if (error || source->Done()) {
                    if (g_repeat != RepeatMode::One) {
                        Next();
                    }
                    break;
                }
            }

            g_source = nullptr;

            return 0;
        }

    }

    Result Initialize() {
        for (int i = 0; i < AUDIO_BUFFER_COUNT; i++) {
            g_audout_buffer[i].buffer = AudioMemoryPool[i];
            g_audout_buffer[i].buffer_size = sizeof(AudioMemoryPool[i]);
        }

        R_TRY(audoutInitialize());
        SetVolume(config::get_volume());

        g_playlist.reserve(PLAYLIST_ENTRY_MAX);
        g_shuffle_playlist.reserve(PLAYLIST_ENTRY_MAX);

        /* Fetch values from config, sanitize the return value */
        if (auto c = config::get_repeat(); c <= 2 && c >= 0) {
            SetRepeatMode(static_cast<RepeatMode>(c));
        }

        SetShuffleMode(static_cast<ShuffleMode>(config::get_shuffle()));
        SetDefaultTitleVolume(config::get_default_title_volume());

        return 0;

    }

    void Exit() {
        g_should_run = false;
    }

    void TuneThreadFunc(void *) {
        /* Run as long as we aren't stopped and no error has been encountered. */
        while (g_should_run) {
            g_current.Reset();
            {
                std::scoped_lock lk(g_mutex);

                const auto &queue = g_playlist;
                const auto queue_size = queue.size();
                if (queue_size == 0) {
                    g_current.Reset();
                } else if (g_queue_position >= queue_size) {
                    g_queue_position = queue_size - 1;
                    continue;
                } else {
                    if (g_shuffle == ShuffleMode::On) {
                        const auto shuffle_id = g_shuffle_playlist[g_queue_position];
                        for (u32 i = 0; i < g_playlist.size(); i++) {
                            if (g_playlist[i].id == shuffle_id) {
                                g_current = g_playlist[i];
                                break;
                            }
                        }
                    } else {
                        g_current = queue[g_queue_position];
                    }
                }
            }

            /* Sleep if queue is empty. */
            if (!g_current.IsValid()) {
                svcSleepThread(100'000'000ul);
                continue;
            }

            g_status = PlayerStatus::Playing;
            /* Only play if playing and we have a track queued. */
            Result rc = PlayTrack(g_playlist2.GetPath(g_current.id));

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

        audoutStopAudioOut();
        audoutExit();
    }

    void GpioThreadFunc(void *ptr) {
        GpioPadSession *session = static_cast<GpioPadSession *>(ptr);

        bool pre_unplug_pause = false;

        /* [0] Low == plugged in; [1] High == not plugged in. */
        GpioValue old_value = GpioValue_High;

        // TODO(TJ): pausing on headphone change should be a config option.
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
        while (g_should_run) {
            u64 pid{}, new_tid{};
            if (pm::PollCurrentPidTid(&pid, &new_tid)) {
                g_title_volume = 1.f;

                if (config::has_title_volume(new_tid)) {
                    g_use_title_volume = true;
                    SetTitleVolume(std::clamp(config::get_title_volume(new_tid), 0.f, VOLUME_MAX));
                }

                // TODO(TJ): fade song in rather than abruptly playing to avoid jump scares
                if (config::has_title_enabled(new_tid)) {
                    g_should_pause = !config::get_title_enabled(new_tid);
                } else {
                    g_should_pause = !config::get_title_enabled_default();
                }
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

            if (g_queue_position < g_playlist.size() - 1) {
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
                g_queue_position = g_playlist.size() - 1;
            }
        }
        g_status     = PlayerStatus::FetchNext;
        g_should_pause = false;
    }

    float GetVolume() {
        float volume = 1.F;
        audoutGetAudioOutVolume(&volume);
        return volume;
    }

    void SetVolume(float volume) {
        volume = std::clamp(volume, 0.f, VOLUME_MAX);
        audoutSetAudioOutVolume(volume);
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

        std::strncpy(buffer, g_playlist2.GetPath(index), buffer_size);

        return 0;
    }

    Result GetCurrentQueueItem(CurrentStats *out, char *buffer, size_t buffer_size) {
        R_UNLESS(g_source != nullptr, tune::NotPlaying);
        R_UNLESS(g_source->IsOpen(), tune::NotPlaying);

        {
            std::scoped_lock lk(g_mutex);
            R_UNLESS(g_current.IsValid(), tune::NotPlaying);
            // R_UNLESS(buffer_size >= g_current.path.size(), tune::InvalidArgument);
            std::strcpy(buffer, g_playlist2.GetPath(g_current.id));
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
            g_playlist2.Clear();
        }
        g_status = PlayerStatus::FetchNext;
    }

    void MoveQueueItem(u32 src, u32 dst) {
        std::scoped_lock lk(g_mutex);

        const auto queue_size = g_playlist.size();

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
            u32 pos = index;

            if (g_shuffle == ShuffleMode::On) {
                const auto track = g_playlist.cbegin() + index;
                for (u32 i = 0; i < g_shuffle_playlist.size(); i++) {
                    if (g_shuffle_playlist[i] == track->id) {
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
        /* Ensure file exists. */
        if (!sdmc::FileExists(buffer))
            return tune::InvalidPath;

        std::scoped_lock lk(g_mutex);

        const auto new_id = g_playlist2.FindNextFreeEntry();
        if (new_id < 0) {
            return tune::OutOfMemory;
        }

        if (!g_playlist2.Add(new_id, buffer)) {
            return tune::OutOfMemory;
        }

        const PlaylistEntry new_entry{
            .id = static_cast<PlaylistID>(new_id)
        };

        // add new entry to playlist
        if (type == EnqueueType::Front) {
            g_playlist.emplace(g_playlist.cbegin(), new_entry);
            if (g_shuffle == ShuffleMode::Off) {
                g_queue_position++;
            }
        } else {
            g_playlist.emplace_back(new_entry);
        }

        // add new entry id to shuffle_playlist_list
        const auto shuffle_playlist_size = g_shuffle_playlist.size();
        const auto shuffle_index = (shuffle_playlist_size > 1) ? (randomGet64() % shuffle_playlist_size) : 0;
        g_shuffle_playlist.emplace(g_shuffle_playlist.cbegin() + shuffle_index, new_id);

        return 0;
    }

    Result Remove(u32 index) {
        std::scoped_lock lk(g_mutex);

        /* Ensure we don't operate out of bounds. */
        R_UNLESS(!g_playlist.empty(), tune::QueueEmpty);
        R_UNLESS(index < g_playlist.size(), tune::OutOfRange);

        /* Get iterator for index position. */
        const auto track = g_playlist.cbegin() + index;
        g_playlist2.Remove(track->id);

        for (u32 i = 0; i < g_shuffle_playlist.size(); i++) {
            if (g_shuffle_playlist[i] == track->id) {
                const auto shuffle_it = g_shuffle_playlist.cbegin() + i;
                // we are playing from shuffle list so use that index instead
                if (g_shuffle == ShuffleMode::On) {
                    index = i;
                }
                // finally remove
                g_shuffle_playlist.erase(shuffle_it);
                break;
            }
        }

        /* Remove entry. */
        g_playlist.erase(track);

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
