#pragma once

#include "../types.hpp"

namespace ams::music {

    void Initialize();
    void Exit();

    void ThreadFunc(void *);
    void EventThreadFunc(void *);

    Result GetStatusImpl(PlayerStatus *out);
    Result SetStatusImpl(PlayerStatus status);
    Result GetVolumeImpl(double *out);
    Result SetVolumeImpl(double volume);
    Result GetLoopImpl(LoopStatus *out);
    Result SetLoopImpl(LoopStatus loop);

    Result GetCurrentImpl(char *out_path, size_t out_path_length, CurrentTune *out);
    Result CountTunesImpl(u32 *out);
    Result ListTunesImpl(char *out_path, size_t out_path_length, u32 *out);

    Result AddToQueueImpl(const char *path, size_t path_length);
    Result ClearImpl();
    Result ShuffleImpl();

}
