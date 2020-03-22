#pragma once
#include "defines.hpp"

#include <switch.h>

namespace os {

    class Mutex {
        NON_COPYABLE(Mutex);
        NON_MOVEABLE(Mutex);

      private:
        ::Mutex m;

      private:
        constexpr ::Mutex *GetMutex() {
            return &this->m;
        }

      public:
        constexpr Mutex()
            : m() {}

        void lock() {
            mutexLock(GetMutex());
        }

        void unlock() {
            mutexUnlock(GetMutex());
        }

        bool try_lock() {
            return mutexTryLock(GetMutex());
        }
    };

}
