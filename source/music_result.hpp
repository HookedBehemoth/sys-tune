#pragma once

#include <stratosphere.hpp>

namespace ams::music {

    R_DEFINE_NAMESPACE_RESULT_MODULE(420);

    R_DEFINE_ERROR_RESULT(InvalidArgument, 1);
    R_DEFINE_ERROR_RESULT(InvalidPath, 2);
    R_DEFINE_ERROR_RESULT(QueueEmpty, 10);
    R_DEFINE_ERROR_RESULT(MpgFailure, 20);

}
