#pragma once

#include "../types.hpp"

namespace ams::music {

    void Initialize();
    void Exit();

    void ThreadFunc(void *);

    Result GetStatusImpl(PlayerStatus *out);
    Result SetStatusImpl(PlayerStatus status);
    Result GetQueueCountImpl(u32 *out);
    Result AddToQueueImpl(const char *path, size_t path_length);
    Result GetNextImpl(char *out_path, size_t out_path_length);
    Result GetLastImpl(char *out_path, size_t out_path_length);
    Result ClearImpl();

}
