#pragma once

#include <switch.h>

class IContentStorage {
    IContentStorage(const IContentStorage &) = delete;
    IContentStorage &operator=(const IContentStorage &) = delete;

  private:
    NcmContentStorage m_storage;
    bool open;

  public:
    IContentStorage();
    IContentStorage(NcmContentStorage &&storage);
    IContentStorage(IContentStorage &&storage);
    IContentStorage &operator=(IContentStorage &&storage);
    ~IContentStorage();

  public:
    Result Open(NcmStorageId storageId);
    bool IsOpen();
    void ForceClose();

  private:
    void Close();

  public:
    /* Stock */
    Result GeneratePlaceHolderId(NcmPlaceHolderId *out_id);
    Result CreatePlaceHolder(const NcmContentId *content_id, const NcmPlaceHolderId *placeholder_id, s64 size);
    Result DeletePlaceHolder(const NcmPlaceHolderId *placeholder_id);
    Result HasPlaceHolder(bool *out, const NcmPlaceHolderId *placeholder_id);
    Result WritePlaceHolder(const NcmPlaceHolderId *placeholder_id, u64 offset, const void *data, size_t data_size);
    Result Register(const NcmContentId *content_id, const NcmPlaceHolderId *placeholder_id);
    Result Delete(const NcmContentId *content_id);
    Result Has(bool *out, const NcmContentId *content_id);
    Result GetPath(char *out_path, size_t out_size, const NcmContentId *content_id);
    Result GetPlaceHolderPath(char *out_path, size_t out_size, const NcmPlaceHolderId *placeholder_id);
    Result CleanupAllPlaceHolder();
    Result ListPlaceHolder(NcmPlaceHolderId *out_ids, s32 count, s32 *out_count);
    Result GetContentCount(s32 *out_count);
    Result ListContentId(NcmContentId *out_ids, s32 count, s32 *out_count, s32 start_offset);
    Result GetSizeFromContentId(s64 *out_size, const NcmContentId *content_id);
    Result DisableForcibly();

    /* [2.0.0+] */
    Result RevertToPlaceHolder(const NcmPlaceHolderId *placeholder_id, const NcmContentId *old_content_id, const NcmContentId *new_content_id);
    Result SetPlaceHolderSize(const NcmPlaceHolderId *placeholder_id, s64 size);
    Result ReadContentIdFile(void *out_data, size_t out_data_size, const NcmContentId *content_id, s64 offset);
    Result GetRightsIdFromPlaceHolderId(NcmRightsId *out_rights_id, const NcmPlaceHolderId *placeholder_id);
    Result GetRightsIdFromContentId(NcmRightsId *out_rights_id, const NcmContentId *content_id);
    Result WriteContentForDebug(const NcmContentId *content_id, s64 offset, const void *data, size_t data_size);
    Result GetFreeSpaceSize(s64 *out_size);
    Result GetTotalSpaceSize(s64 *out_size);

    /* [3.0.0+] */
    Result FlushPlaceHolder();

    /* [4.0.0+] */
    Result GetSizeFromPlaceHolderId(s64 *out_size, const NcmPlaceHolderId *placeholder_id);
    Result RepairInvalidFileAttribute();

    /* [8.0.0+] */
    Result GetRightsIdFromPlaceHolderIdWithCache(NcmRightsId *out_rights_id, const NcmPlaceHolderId *placeholder_id, const NcmContentId *cache_content_id);

    /* Extended. */
    /* Uses an internal buffer. Not thread safe. */
    const char *GetPathBuffer(const NcmContentId *content_id);
    const char *GetPlaceHolderPathBuffer(const NcmPlaceHolderId *placeholder_id);
};
