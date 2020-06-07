#pragma once

#include <switch.h>

namespace sdmc {

    Result Open();
    void Close();

    Result OpenFile(FsFile *file, const char* path, int open_mode = FsOpenMode_Read);
    bool FileExists(const char* path);

}
