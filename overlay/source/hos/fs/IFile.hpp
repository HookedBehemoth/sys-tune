#pragma once
#include "../util/defines.hpp"

#include <switch.h>

class IFile {
    NON_COPYABLE(IFile);

  private:
    ::FsFile m_file;
    bool open;

  public:
    IFile();
    IFile(FsFile &&file);
    IFile(IFile &&file);
    IFile &operator=(IFile &&file);
    ~IFile();

  public:
    bool IsOpen();
    void ForceClose();

  private:
    void Close();

  public:
    /* Stock */
    Result Read(u64 *bytes_read, void *buffer, s64 offset, u64 read_size, u32 option = FsReadOption_None);
    Result Write(const void *buffer, s64 offset, u64 write_size, u32 option = FsWriteOption_None);
    Result Flush();
    Result SetSize(s64 sz);
    Result GetSize(s64 *out);

    /* [4.0.0+] */
    Result OperateRange(FsOperationId operation_id, s64 offset, s64 length, FsRangeInfo *out);

    /* Extension */
    Result CopyTo(u8 *buffer, u64 buffer_size, IFile *file);
    Result CopyTo(IFile *file);
};
