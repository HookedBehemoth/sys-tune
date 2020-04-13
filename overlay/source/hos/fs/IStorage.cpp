#include "IStorage.hpp"

#include "../util/result.hpp"

#include <utility>

namespace fs {

    IStorage::IStorage()
        : m_storage(), open(false) {}

    IStorage::IStorage(FsStorage &&storage)
        : m_storage(storage), open(true) {
        storage = {};
    }

    IStorage::IStorage(IStorage &&storage)
        : m_storage(storage.m_storage), open(storage.open) {
        storage.m_storage = {};
        storage.open = false;
    }

    IStorage &IStorage::operator=(IStorage &&storage) {
        std::swap(this->m_storage, storage.m_storage);
        std::swap(this->open, storage.open);
        return *this;
    }

    IStorage::~IStorage() {
        this->Close();
    }

    bool IStorage::IsOpen() {
        return this->open;
    }

    void IStorage::ForceClose() {
        this->Close();
    }

    void IStorage::Close() {
        if (this->open) {
            fsStorageClose(&this->m_storage);
            this->open = false;
        }
    }

    Result IStorage::Read(s64 offset, void *buffer, u64 read_size) {
        return fsStorageRead(&this->m_storage, offset, buffer, read_size);
    }

    Result IStorage::Write(s64 offset, const void *buffer, u64 write_size) {
        return fsStorageWrite(&this->m_storage, offset, buffer, write_size);
    }

    Result IStorage::Flush() {
        return fsStorageFlush(&this->m_storage);
    }

    Result IStorage::SetSize(s64 size) {
        return fsStorageSetSize(&this->m_storage, size);
    }

    Result IStorage::GetSize(s64 *out) {
        return fsStorageGetSize(&this->m_storage, out);
    }

    Result IStorage::OperateRange(FsOperationId operation_id, s64 offset, s64 length, FsRangeInfo *out) {
        return fsStorageOperateRange(&this->m_storage, operation_id, offset, length, out);
    }

    Result IStorage::OpenBisStorage(FsBisPartitionId partitionId) {
        this->Close();
        R_TRY(fsOpenBisStorage(&this->m_storage, partitionId));
        this->open = true;
        return ResultSuccess();
    }

}
