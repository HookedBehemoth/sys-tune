#pragma once

#include "../types.hpp"

namespace ams::tune::impl {

    Result Initialize();
    void Exit();

    void AudioThreadFunc(void *);
    void PscThreadFunc(void *ptr);
    void GpioThreadFunc(void *ptr);

    Result GetStatus(AudioOutState *out);
    Result Play();
    Result Pause();
    void Next();
    void Prev();

    Result GetVolume(float *volume);
    Result SetVolume(float volume);

    void GetRepeatMode(RepeatMode *mode);
    void SetRepeatMode(RepeatMode mode);
    void GetShuffleMode(ShuffleMode *mode);
    void SetShuffleMode(ShuffleMode mode);

    void GetCurrentPlaylistSize(u32 *size);
    void GetCurrentPlaylist(u32 *size, char* buffer, size_t buffer_size);
    Result GetCurrentQueueItem(CurrentStats *out, char* buffer, size_t buffer_size);
    void ClearQueue();
    void MoveQueueItem(u32 src, u32 dst);
    void Select(u32 index);

    Result Enqueue(const char* buffer, size_t buffer_length, EnqueueType type);
    Result Remove(u32 index);

}
