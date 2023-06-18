/**
 * @file audout.h
 * @brief Removed in [11.0.0+], replaced with aud:a / aud:d (see aud.h)
 * @note Only one session may be open at once.
 * @author TotalJustice
 * @copyright libnx Authors
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

#define AUDOUT_MAX_DELAY (1000000000ULL)

/// Initialize audout:a. Removed in [11.0.0].
Result audoutaInitialize(void);
/// Exit audout:a.
void audoutaExit(void);
/// Gets the Service for audout:a.
Service* audoutaGetServiceSession(void);

/// Initialize audout:d. Removed in [11.0.0].
Result audoutdInitialize(void);
/// Exit audout:d.
void audoutdExit(void);
/// Gets the Service for audout:d.
Service* audoutdGetServiceSession(void);

// Untested
Result audoutaRequestSuspendOld(u64 pid, u64 delay, Handle* handle_out); // [1.0.0] - [4.0.0]
Result audoutaRequestResumeOld(u64 pid, u64 delay, Handle* handle_out); // [1.0.0] - [4.0.0]
// All tested (and works)
Result audoutaRequestSuspend(u64 pid, u64 delay); // [4.0.0]+
Result audoutaRequestResume(u64 pid, u64 delay); // [4.0.0]+
Result audoutaGetProcessMasterVolume(u64 pid, float* volume_out);
Result audoutaSetProcessMasterVolume(u64 pid, u64 delay, float volume);
Result audoutaGetProcessRecordVolume(u64 pid, float* volume_out);
Result audoutaSetProcessRecordVolume(u64 pid, u64 delay, float volume);

// Untested
Result audoutdRequestSuspendForDebug(u64 pid, u64 delay);
Result audoutdRequestResumeForDebug(u64 pid, u64 delay);

#ifdef __cplusplus
}
#endif
