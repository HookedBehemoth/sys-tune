#pragma once
#include "../util/defines.hpp"

#include <switch.h>

namespace fs {

    class IDirectory {
        NON_COPYABLE(IDirectory);

      private:
        ::FsDir m_dir;
        bool open;

      public:
        IDirectory();
        IDirectory(FsDir &&dir);
        IDirectory(IDirectory &&dir);
        IDirectory &operator=(IDirectory &&dir);
        ~IDirectory();

      public:
        bool IsOpen();
        void ForceClose();

      private:
        void Close();

      public:
        Result Read(s64 *total_entries, FsDirectoryEntry *entries, size_t max_entries);
        Result GetEntryCount(s64 *count);
    };

}
