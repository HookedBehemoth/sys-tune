#pragma once

#include <stratosphere.hpp>

namespace ams::tune {

    R_DEFINE_NAMESPACE_RESULT_MODULE(420);

    R_DEFINE_ERROR_RESULT(InvalidArgument, 1);
    R_DEFINE_ERROR_RESULT(InvalidPath, 2);
    R_DEFINE_ERROR_RESULT(FileNotFound, 3);
    R_DEFINE_ERROR_RESULT(QueueEmpty, 10);
    R_DEFINE_ERROR_RESULT(NotPlaying, 11);
    R_DEFINE_ERROR_RESULT(OutOfRange, 12);
    R_DEFINE_ERROR_RESULT(MpgFailure, 20);
    R_DEFINE_ERROR_RESULT(MpgDone, 21);
    R_DEFINE_ERROR_RESULT(BlockSizeTooBig, 22);

}

namespace ams::kern {

    R_DEFINE_NAMESPACE_RESULT_MODULE(1);

    R_DEFINE_ERROR_RESULT(WaitTimeout, 117);

}

#ifndef RELEASE
#define LOG(fmt, ...) std::printf(fmt, __VA_ARGS__);
#else
#define LOG(fmt, ...)
#endif