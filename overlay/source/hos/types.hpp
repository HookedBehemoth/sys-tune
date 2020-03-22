#pragma once

#include "types/hfs0.hpp"
#include "types/pfs0.hpp"
#include "util/defines.hpp"

#include <switch.h>

static constexpr ALWAYS_INLINE bool VerifyMagic(const u8 a[4], const u8 b[4]) {
    return *reinterpret_cast<const u32 *>(a) == *reinterpret_cast<const u32 *>(b);
}
