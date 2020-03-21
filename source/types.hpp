#pragma once

#include <stratosphere.hpp>

enum class PlayerStatus : u8 {
    Stopped,
    Playing,
    Paused,
    Next,
    Exit,
};
