#pragma once

#include <switch.h>

namespace tune {

    constexpr const u32 Module = 420;

    constexpr const Result InvalidArgument  = MAKERESULT(Module, 1);
    constexpr const Result InvalidPath      = MAKERESULT(Module, 2);
    constexpr const Result FileNotFound     = MAKERESULT(Module, 3);
    constexpr const Result QueueEmpty       = MAKERESULT(Module, 10);
    constexpr const Result NotPlaying       = MAKERESULT(Module, 11);
    constexpr const Result OutOfRange       = MAKERESULT(Module, 12);
    constexpr const Result FileOpenFailure  = MAKERESULT(Module, 20);
    constexpr const Result VoiceInitFailure = MAKERESULT(Module, 21);
    constexpr const Result OutOfMemory      = MAKERESULT(Module, 30);
    constexpr const Result Generic          = MAKERESULT(Module, 40);

}

#define R_UNLESS(bool_expr, res) \
    ({                           \
        if (!(bool_expr)) {      \
            return res;          \
        }                        \
    })

#define R_TRY(res_expr)                   \
    ({                                    \
        const auto temp_res = (res_expr); \
        if (R_FAILED(temp_res))           \
            return temp_res;              \
    })

#define R_ABORT_UNLESS(res_expr)   \
    ({                             \
        auto tmp_res = (res_expr); \
        if (R_FAILED(tmp_res))     \
            diagAbortWithResult(tmp_res);   \
    })
