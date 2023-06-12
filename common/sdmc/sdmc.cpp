#include "sdmc.hpp"

#include <cstring>

namespace sdmc {

    namespace {

        FsFileSystem sdmc;
        char path_buffer[FS_MAX_PATH];

    }

    Result Open() {
        return fsOpenSdCardFileSystem(&sdmc);
    }

    void Close() {
        fsFsClose(&sdmc);
    }

    Result OpenFile(FsFile *file, const char *path, int open_mode) {
        std::strcpy(path_buffer, path);
        return fsFsOpenFile(&sdmc, path_buffer, open_mode, file);
    }

    bool FileExists(const char* path) {
        std::strcpy(path_buffer, path);
        FsTimeStampRaw ts;
        return R_SUCCEEDED(fsFsGetFileTimeStampRaw(&sdmc, path_buffer, &ts));
    }

    Result CreateFolder(const char* path) {
        std::strcpy(path_buffer, path);
        return fsFsCreateDirectory(&sdmc, path_buffer);
    }

}
