#pragma once

#include "../../ipc/tune.h"

#include <functional>

namespace tune {

    enum class PlayerStatus : u8 {
        Playing,
        FetchNext,
    };

    enum class ShuffleMode : u8 {
        Off,
        On,
    };

    enum class RepeatMode : u8 {
        Off,
        One,
        All,
    };

    enum class EnqueueType : u8 {
        Front,
        Back,
    };

    struct CurrentStats : TuneCurrentStats {};

}
