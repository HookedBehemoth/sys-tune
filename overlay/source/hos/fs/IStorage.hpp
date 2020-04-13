#pragma once
#include "../util/defines.hpp"

#include <switch.h>

namespace fs {

    class IStorage {
        NON_COPYABLE(IStorage);

      private:
        ::FsStorage m_storage;
        bool open;

      public:
        IStorage();
        IStorage(FsStorage &&storage);
        IStorage(IStorage &&storage);
        IStorage &operator=(IStorage &&storage);
        ~IStorage();

      public:
        bool IsOpen();
        void ForceClose();

      private:
        void Close();

      public:
        Result Read(s64 offset, void *buffer, u64 read_size);
        Result Write(s64 offset, const void *buffer, u64 write_size);
        Result Flush();
        Result SetSize(s64 size);
        Result GetSize(s64 *out);

        /* [4.0.0+] */
        Result OperateRange(FsOperationId operation_id, s64 offset, s64 len, FsRangeInfo *out);

      public:
        Result OpenBisStorage(FsBisPartitionId partitionId);
    };

}
