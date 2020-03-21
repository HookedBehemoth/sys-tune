#pragma once

#include <switch.h>

typedef enum {
    MusicPlayerStatus_Stopped,
    MusicPlayerStatus_Playing,
    MusicPlayerStatus_Paused,
} MusicPlayerStatus;

Result musicInitialize();

void musicExit();

Result musicGetStatus(MusicPlayerStatus *out);
Result musicSetStatus(MusicPlayerStatus status);
Result musicGetQueueCount(u32 *out);
Result musicAddToQueue(const char *path);
Result musicGetNext(char *out_path, size_t out_path_length);
Result musicGetLast(char *out_path, size_t out_path_length);
Result musicClear();
