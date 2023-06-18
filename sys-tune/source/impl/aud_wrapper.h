#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

#define AUD_WRAPPER_MAX_DELAY (1000000000ULL)

/// Initialize audout:a until [11.0.0], otherwise aud:a.
Result audWrapperInitialize(void);
/// Exit audout:a until [11.0.0], otherwise aud:a.
void audWrapperExit(void);

Result audWrapperRequestSuspend(u64 pid, u64 delay);
Result audWrapperRequestResume(u64 pid, u64 delay);
Result audWrapperGetProcessMasterVolume(u64 pid, float* volume_out);
Result audWrapperSetProcessMasterVolume(u64 pid, u64 delay, float volume);
Result audWrapperGetProcessRecordVolume(u64 pid, float* volume_out);
Result audWrapperSetProcessRecordVolume(u64 pid, u64 delay, float volume);

#ifdef __cplusplus
}
#endif
