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
        Next,
        Last,
    };

    struct CurrentStats : TuneCurrentStats {};

    class ScopeGuard {
        ScopeGuard(const ScopeGuard &) = delete;
        ScopeGuard &operator=(const ScopeGuard &) = delete;
        ScopeGuard(ScopeGuard &&)                 = delete;
        ScopeGuard &operator=(ScopeGuard &&) = delete;

      private:
        std::function<void()> f;

      public:
        ScopeGuard(std::function<void()> f) : f(std::move(f)) {}
        ~ScopeGuard() {
            if (f) {
                f();
            }
            f = nullptr;
        }
        void dismiss() { f = nullptr; }
        void invoke() { this->~ScopeGuard(); }
    };

}
