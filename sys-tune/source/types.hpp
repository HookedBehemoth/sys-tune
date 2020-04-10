#pragma once

#include "../../ipc/tune.h"

#include <stratosphere.hpp>

namespace ams::tune {

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
        Next,
        Last,
    };

    struct ScopedOutBuffer {
        NON_COPYABLE(ScopedOutBuffer);
        NON_MOVEABLE(ScopedOutBuffer);
        AudioOutBuffer buffer;
        ScopedOutBuffer() : buffer({}) {}
        bool init(size_t buffer_size) {
            buffer.buffer = aligned_alloc(0x1000, buffer_size);
            buffer.buffer_size = buffer_size;
            return buffer.buffer;
        }
        ~ScopedOutBuffer() {
            if (buffer.buffer)
                free(buffer.buffer);
        }
    };

    struct CurrentStats : TuneCurrentStats {};

}
