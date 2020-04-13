#include "IFile.hpp"

#include "../util/result.hpp"

#include <utility>

namespace fs {

    IFile::IFile()
        : m_file(), open(false) {}

    IFile::IFile(FsFile &&file)
        : m_file(file), open(true) {
        file = {};
    }

    IFile::IFile(IFile &&file)
        : m_file(file.m_file), open(file.open) {
        file.m_file = {};
        file.open = false;
    }

    IFile &IFile::operator=(IFile &&file) {
        std::swap(this->m_file, file.m_file);
        std::swap(this->open, file.open);
        return *this;
    }

    IFile::~IFile() {
        this->Close();
    }

    bool IFile::IsOpen() {
        return this->open;
    }

    void IFile::ForceClose() {
        this->Close();
    }

    void IFile::Close() {
        if (this->open) {
            fsFileClose(&this->m_file);
            this->open = false;
        }
    }

    Result IFile::Read(u64 *bytes_read, void *buffer, s64 offset, u64 read_size, u32 option) {
        return fsFileRead(&this->m_file, offset, buffer, read_size, option, bytes_read);
    }

    Result IFile::Write(const void *buffer, s64 offset, u64 write_size, u32 option) {
        return fsFileWrite(&this->m_file, offset, buffer, write_size, option);
    }

    Result IFile::Flush() {
        return fsFileFlush(&this->m_file);
    }

    Result IFile::SetSize(s64 sz) {
        return fsFileSetSize(&this->m_file, sz);
    }

    Result IFile::GetSize(s64 *out) {
        return fsFileGetSize(&this->m_file, out);
    }

    Result IFile::OperateRange(FsOperationId operation_id, s64 offset, s64 length, FsRangeInfo *out) {
        return fsFileOperateRange(&this->m_file, operation_id, offset, length, out);
    }

    Result IFile::CopyTo(u8 *buffer, u64 buffer_size, IFile *file) {
        s64 file_size;

        R_TRY(this->GetSize(&file_size));
        R_TRY(file->SetSize(file_size));

        s64 pos = 0;
        while (pos < file_size) {
            u64 read;
            R_TRY(this->Read(&read, buffer, pos, buffer_size));
            R_TRY(file->Write(buffer, pos, read));
            pos += read;
        }

        return ResultSuccess();
    }

    Result IFile::CopyTo(IFile *file) {
        const u64 buffer_size = 0x100000;
        u8 *buffer = new u8[buffer_size];

        if (!buffer)
            return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

        Result rc = this->CopyTo(buffer, buffer_size, file);

        delete[] buffer;
        return rc;
    }

}
