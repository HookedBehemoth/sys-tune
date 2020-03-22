#pragma once
#include <cstddef>

#define NON_COPYABLE(cls)      \
    cls(const cls &) = delete; \
    cls &operator=(const cls &) = delete

#define NON_MOVEABLE(cls) \
    cls(cls &&) = delete; \
    cls &operator=(cls &&) = delete

#define ALWAYS_INLINE inline __attribute__((always_inline))

constexpr ALWAYS_INLINE size_t operator""_KB(unsigned long long n) {
    return static_cast<size_t>(n) * size_t(1024);
}

constexpr ALWAYS_INLINE size_t operator""_MB(unsigned long long n) {
    return operator""_KB(n)*size_t(1024);
}

constexpr ALWAYS_INLINE size_t operator""_GB(unsigned long long n) {
    return operator""_MB(n)*size_t(1024);
}
