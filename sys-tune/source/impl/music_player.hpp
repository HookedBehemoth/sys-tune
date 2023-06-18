#pragma once

#include "../tune_types.hpp"
#include <string>
#include <vector>

namespace tune::impl {
    using PlaylistID = u32;

    struct PlaylistEntry {
        std::string path;
        PlaylistID id;
    };

    Result Initialize(std::vector<PlaylistEntry>* playlist, std::vector<PlaylistID>* shuffle, PlaylistEntry* current);
    void Exit();

    void TuneThreadFunc(void *);
    void PscmThreadFunc(void *ptr);
    void GpioThreadFunc(void *ptr);
    void PmdmntThreadFunc(void *ptr);

    bool GetStatus();
    void Play();
    void Pause();
    void Next();
    void Prev();

    float GetVolume();
    void SetVolume(float volume);
    float GetTitleVolume();
    void SetTitleVolume(float volume);
    float GetDefaultTitleVolume();
    void SetDefaultTitleVolume(float volume);

    void TitlePlay();
    void TitlePause();
    void DefaultTitlePlay();
    void DefaultTitlePause();

    RepeatMode GetRepeatMode();
    void SetRepeatMode(RepeatMode mode);
    ShuffleMode GetShuffleMode();
    void SetShuffleMode(ShuffleMode mode);

    u32 GetPlaylistSize();
    u32 GetPlaylistItem(u32 index, char* buffer, size_t buffer_size);
    Result GetCurrentQueueItem(CurrentStats *out, char* buffer, size_t buffer_size);
    void ClearQueue();
    void MoveQueueItem(u32 src, u32 dst);
    void Select(u32 index);
    void Seek(u32 position);

    Result Enqueue(const char* buffer, size_t buffer_length, EnqueueType type);
    Result Remove(u32 index);

}
