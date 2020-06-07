#pragma once

#include "../types.hpp"

/* Default audio config. */
constexpr const AudioRendererConfig audren_cfg = {
    .output_rate = AudioRendererOutputRate_48kHz,
    .num_voices = 4,
    .num_effects = 0,
    .num_sinks = 1,
    .num_mix_objs = 1,
    .num_mix_buffers = 2,
};

namespace tune::impl {

    Result Initialize();
    void Exit();

    void AudioThreadFunc(void *);
    void PscThreadFunc(void *ptr);
    void GpioThreadFunc(void *ptr);

    bool GetStatus();
    void Play();
    void Pause();
    void Next();
    void Prev();

    float GetVolume();
    void SetVolume(float volume);

    RepeatMode GetRepeatMode();
    void SetRepeatMode(RepeatMode mode);
    ShuffleMode GetShuffleMode();
    void SetShuffleMode(ShuffleMode mode);

    u32 GetCurrentPlaylistSize();
    u32 GetCurrentPlaylist(char* buffer, size_t buffer_size);
    Result GetCurrentQueueItem(CurrentStats *out, char* buffer, size_t buffer_size);
    void ClearQueue();
    void MoveQueueItem(u32 src, u32 dst);
    void Select(u32 index);
    void Seek(u32 position);

    Result Enqueue(const char* buffer, size_t buffer_length, EnqueueType type);
    Result Remove(u32 index);

}
