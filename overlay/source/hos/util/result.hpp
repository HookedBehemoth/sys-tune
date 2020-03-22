#pragma once
#include "../util/defines.hpp"

#include <switch.h>

#define MAKE_RC(name, module, desc) \
    static constexpr ALWAYS_INLINE Result Result##name() { return MAKERESULT(module, desc); }

MAKE_RC(Success, 0, 0)

#define R_TRY(res_expr)                   \
    ({                                    \
        const auto temp_res = (res_expr); \
        if (R_FAILED(temp_res))           \
            return temp_res;              \
    })

#define R_ASSERT(res_expr)                \
    ({                                    \
        const auto temp_res = (res_expr); \
        if (R_FAILED(temp_res))           \
            fatalThrow(temp_res);         \
    })

#define R_UNLESS(bool_expr, res) \
    ({                           \
        if (!(bool_expr)) {      \
            return res;          \
        }                        \
    })
