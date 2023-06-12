#pragma once

#include <switch.h>

namespace config {

auto get_title_default() -> bool;
void set_title_default(bool value);

auto get_title(u64 tid, bool load_default) -> bool;
void set_title(u64 tid, bool value);

auto get_shuffle() -> bool;
void set_shuffle(bool value);

auto get_repeat() -> int;
void set_repeat(int value);

auto get_volume() -> int;
void set_volume(int value);

} // namespace config
