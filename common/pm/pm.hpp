#pragma once

#include <switch.h>

namespace pm {

auto Initialize() -> Result;
void Exit();
void getCurrentPidTid(u64& pid_out, u64& tid_out);
auto PollCurrentPidTid(u64& pid_out, u64& tid_out) -> bool;

} // namespace pm
