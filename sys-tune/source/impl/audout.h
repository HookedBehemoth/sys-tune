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

// Untested
Result audoutaRequestSuspendOld(u64 pid, u64 delay, Handle* handle_out); // [1.0.0] - [4.0.0]
Result audoutaRequestResumeOld(u64 pid, u64 delay, Handle* handle_out); // [1.0.0] - [4.0.0]

#ifdef __cplusplus
}
#endif
