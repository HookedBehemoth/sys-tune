#pragma once
#include <functional>
#include <utility>
#include "defines.hpp"

class ScopeGuard {
    NON_COPYABLE(ScopeGuard);
    NON_MOVEABLE(ScopeGuard);

  private:
    std::function<void()> f;

  public:
    ALWAYS_INLINE ScopeGuard(std::function<void()> f) : f(std::move(f)) {}
    ALWAYS_INLINE ~ScopeGuard() {
        if (f) {
            f();
        }
        f = nullptr;
    }
    ALWAYS_INLINE void dismiss() { f = nullptr; }
    ALWAYS_INLINE void invoke() { this->~ScopeGuard(); }
};