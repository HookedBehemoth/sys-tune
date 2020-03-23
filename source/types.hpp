#pragma once

#include <stratosphere.hpp>
#include "../ipc/music.h"

enum class PlayerStatus : u8 {
    Stopped,
    Playing,
    Paused,
    Next,
    Exit,
};

enum class LoopStatus : u8 {
    Off,
    List,
    Single,
};

struct CurrentTune : MusicCurrentTune {};
