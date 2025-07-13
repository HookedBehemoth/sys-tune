#pragma once

#include <switch.h>

namespace sdmc {

    Result Open();
    void Close();

    Result OpenFile(FsFile *file, const char* path, int open_mode = FsOpenMode_Read);
    Result OpenDir(FsDir *dir, const char *path, int open_mode);

    Result GetType(const char* path, FsDirEntryType* type);
    bool FileExists(const char* path);

    Result CreateFolder(const char* path);

}
