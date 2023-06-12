#pragma once

#include <switch.h>

namespace pm {

auto Initialize() -> Result;
void Exit();
auto PollCurrentTitle(u64& title_id_out) -> bool;

} // namespace pm
