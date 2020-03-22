#include "IContentStorage.hpp"

#include <utility>

IContentStorage::IContentStorage()
    : m_storage(), open(false) {}

IContentStorage::IContentStorage(NcmContentStorage &&storage)
    : m_storage(storage), open(true) {}

IContentStorage::IContentStorage(IContentStorage &&storage)
    : m_storage(storage.m_storage), open(storage.open) {
    storage.m_storage = {};
    storage.open = false;
}

IContentStorage &IContentStorage::operator=(IContentStorage &&storage) {
    std::swap(this->m_storage, storage.m_storage);
    std::swap(this->open, storage.open);
    return *this;
}

IContentStorage::~IContentStorage() {
    this->Close();
}

Result IContentStorage::Open(NcmStorageId storageId) {
    Result rc = ncmOpenContentStorage(&this->m_storage, storageId);
    this->open = R_SUCCEEDED(rc);
    return rc;
}

bool IContentStorage::IsOpen() {
    return this->open;
}

void IContentStorage::ForceClose() {
    this->Close();
}

void IContentStorage::Close() {
    if (this->open) {
        ncmContentStorageClose(&this->m_storage);
        this->open = false;
    }
}

Result IContentStorage::GeneratePlaceHolderId(NcmPlaceHolderId *out_id) {
    return ncmContentStorageGeneratePlaceHolderId(&this->m_storage, out_id);
}

Result IContentStorage::CreatePlaceHolder(const NcmContentId *content_id, const NcmPlaceHolderId *placeholder_id, s64 size) {
    return ncmContentStorageCreatePlaceHolder(&this->m_storage, content_id, placeholder_id, size);
}

Result IContentStorage::DeletePlaceHolder(const NcmPlaceHolderId *placeholder_id) {
    return ncmContentStorageDeletePlaceHolder(&this->m_storage, placeholder_id);
}

Result IContentStorage::HasPlaceHolder(bool *out, const NcmPlaceHolderId *placeholder_id) {
    return ncmContentStorageHasPlaceHolder(&this->m_storage, out, placeholder_id);
}

Result IContentStorage::WritePlaceHolder(const NcmPlaceHolderId *placeholder_id, u64 offset, const void *data, size_t data_size) {
    return ncmContentStorageWritePlaceHolder(&this->m_storage, placeholder_id, offset, data, data_size);
}

Result IContentStorage::Register(const NcmContentId *content_id, const NcmPlaceHolderId *placeholder_id) {
    return ncmContentStorageRegister(&this->m_storage, content_id, placeholder_id);
}

Result IContentStorage::Delete(const NcmContentId *content_id) {
    return ncmContentStorageDelete(&this->m_storage, content_id);
}

Result IContentStorage::Has(bool *out, const NcmContentId *content_id) {
    return ncmContentStorageHas(&this->m_storage, out, content_id);
}

Result IContentStorage::GetPath(char *out_path, size_t out_size, const NcmContentId *content_id) {
    return ncmContentStorageGetPath(&this->m_storage, out_path, out_size, content_id);
}

Result IContentStorage::GetPlaceHolderPath(char *out_path, size_t out_size, const NcmPlaceHolderId *placeholder_id) {
    return ncmContentStorageGetPlaceHolderPath(&this->m_storage, out_path, out_size, placeholder_id);
}

Result IContentStorage::CleanupAllPlaceHolder() {
    return ncmContentStorageCleanupAllPlaceHolder(&this->m_storage);
}

Result IContentStorage::ListPlaceHolder(NcmPlaceHolderId *out_ids, s32 count, s32 *out_count) {
    return ncmContentStorageListPlaceHolder(&this->m_storage, out_ids, count, out_count);
}

Result IContentStorage::GetContentCount(s32 *out_count) {
    return ncmContentStorageGetContentCount(&this->m_storage, out_count);
}

Result IContentStorage::ListContentId(NcmContentId *out_ids, s32 count, s32 *out_count, s32 start_offset) {
    return ncmContentStorageListContentId(&this->m_storage, out_ids, count, out_count, start_offset);
}

Result IContentStorage::GetSizeFromContentId(s64 *out_size, const NcmContentId *content_id) {
    return ncmContentStorageGetSizeFromContentId(&this->m_storage, out_size, content_id);
}

Result IContentStorage::DisableForcibly() {
    return ncmContentStorageDisableForcibly(&this->m_storage);
}

Result IContentStorage::RevertToPlaceHolder(const NcmPlaceHolderId *placeholder_id, const NcmContentId *old_content_id, const NcmContentId *new_content_id) {
    return ncmContentStorageRevertToPlaceHolder(&this->m_storage, placeholder_id, old_content_id, new_content_id);
}

Result IContentStorage::SetPlaceHolderSize(const NcmPlaceHolderId *placeholder_id, s64 size) {
    return ncmContentStorageSetPlaceHolderSize(&this->m_storage, placeholder_id, size);
}

Result IContentStorage::ReadContentIdFile(void *out_data, size_t out_data_size, const NcmContentId *content_id, s64 offset) {
    return ncmContentStorageReadContentIdFile(&this->m_storage, out_data, out_data_size, content_id, offset);
}

Result IContentStorage::GetRightsIdFromPlaceHolderId(NcmRightsId *out_rights_id, const NcmPlaceHolderId *placeholder_id) {
    return ncmContentStorageGetRightsIdFromPlaceHolderId(&this->m_storage, out_rights_id, placeholder_id);
}

Result IContentStorage::GetRightsIdFromContentId(NcmRightsId *out_rights_id, const NcmContentId *content_id) {
    return ncmContentStorageGetRightsIdFromContentId(&this->m_storage, out_rights_id, content_id);
}

Result IContentStorage::WriteContentForDebug(const NcmContentId *content_id, s64 offset, const void *data, size_t data_size) {
    return ncmContentStorageWriteContentForDebug(&this->m_storage, content_id, offset, data, data_size);
}

Result IContentStorage::GetFreeSpaceSize(s64 *out_size) {
    return ncmContentStorageGetFreeSpaceSize(&this->m_storage, out_size);
}

Result IContentStorage::GetTotalSpaceSize(s64 *out_size) {
    return ncmContentStorageGetTotalSpaceSize(&this->m_storage, out_size);
}

Result IContentStorage::FlushPlaceHolder() {
    return ncmContentStorageFlushPlaceHolder(&this->m_storage);
}

Result IContentStorage::GetSizeFromPlaceHolderId(s64 *out_size, const NcmPlaceHolderId *placeholder_id) {
    return ncmContentStorageGetSizeFromPlaceHolderId(&this->m_storage, out_size, placeholder_id);
}

Result IContentStorage::RepairInvalidFileAttribute() {
    return ncmContentStorageRepairInvalidFileAttribute(&this->m_storage);
}

Result IContentStorage::GetRightsIdFromPlaceHolderIdWithCache(NcmRightsId *out_rights_id, const NcmPlaceHolderId *placeholder_id, const NcmContentId *cache_content_id) {
    return ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(&this->m_storage, out_rights_id, placeholder_id, cache_content_id);
}

static char path_buffer[FS_MAX_PATH];

const char *IContentStorage::GetPathBuffer(const NcmContentId *content_id) {
    Result rc = this->GetPath(path_buffer, FS_MAX_PATH, content_id);

    if (R_FAILED(rc))
        return nullptr;

    return path_buffer;
}

const char *IContentStorage::GetPlaceHolderPathBuffer(const NcmPlaceHolderId *placeholder_id) {
    Result rc = this->GetPlaceHolderPath(path_buffer, FS_MAX_PATH, placeholder_id);

    if (R_FAILED(rc))
        return nullptr;

    return path_buffer;
}
