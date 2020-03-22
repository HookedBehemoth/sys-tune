#pragma once

#include "../types.hpp"

namespace ams::music {

    void Initialize();
    void Exit();

    void ThreadFunc(void *);

    Result GetStatusImpl(PlayerStatus *out);
    Result SetStatusImpl(PlayerStatus status);
    Result GetQueueCountImpl(u32 *out);
    Result GetCurrentImpl(char *out_path, size_t out_path_length);
    Result GetListImpl(char *out_path, size_t out_path_length, u32 *out);
    Result GetCurrentLengthImpl(double *out);
    Result GetCurrentProgressImpl(double *out);
    Result GetVolumeImpl(double *out);
    Result SetVolumeImpl(double volume);

    Result AddToQueueImpl(const char *path, size_t path_length);
    Result ClearImpl();

}
