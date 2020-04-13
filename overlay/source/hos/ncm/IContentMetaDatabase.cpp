#include "IContentMetaDatabase.hpp"

#include "../util/result.hpp"

#include <utility>

namespace ncm {

    IContentMetaDatabase::IContentMetaDatabase()
        : m_db(), open(false) {}

    IContentMetaDatabase::IContentMetaDatabase(NcmContentMetaDatabase &&db)
        : m_db(db), open(true) {
        db = {};
    }

    IContentMetaDatabase::IContentMetaDatabase(IContentMetaDatabase &&db)
        : m_db(db.m_db), open(db.open) {
        db.m_db = {};
        db.open = false;
    }

    IContentMetaDatabase &IContentMetaDatabase::operator=(IContentMetaDatabase &&storage) {
        std::swap(this->m_db, storage.m_db);
        std::swap(this->open, storage.open);
        return *this;
    }

    IContentMetaDatabase::~IContentMetaDatabase() {
        this->Close();
    }

    bool IContentMetaDatabase::IsOpen() {
        return this->open;
    }

    void IContentMetaDatabase::ForceClose() {
        this->Close();
    }

    void IContentMetaDatabase::Close() {
        if (this->open) {
            ncmContentMetaDatabaseClose(&this->m_db);
            this->open = false;
        }
    }

    Result IContentMetaDatabase::Set(const NcmContentMetaKey *key, const void *data, u64 data_size) {
        return ncmContentMetaDatabaseSet(&this->m_db, key, data, data_size);
    }

    Result IContentMetaDatabase::Get(const NcmContentMetaKey *key, u64 *out_size, void *out_data, u64 out_data_size) {
        return ncmContentMetaDatabaseGet(&this->m_db, key, out_size, out_data, out_data_size);
    }

    Result IContentMetaDatabase::Remove(const NcmContentMetaKey *key) {
        return ncmContentMetaDatabaseRemove(&this->m_db, key);
    }

    Result IContentMetaDatabase::GetContentIdByType(NcmContentId *out_content_id, const NcmContentMetaKey *key, NcmContentType type) {
        return ncmContentMetaDatabaseGetContentIdByType(&this->m_db, out_content_id, key, type);
    }

    Result IContentMetaDatabase::ListContentInfo(s32 *out_entries_written, NcmContentInfo *out_info, s32 count, const NcmContentMetaKey *key, s32 start_index) {
        return ncmContentMetaDatabaseListContentInfo(&this->m_db, out_entries_written, out_info, count, key, start_index);
    }

    Result IContentMetaDatabase::List(s32 *out_entries_total, s32 *out_entries_written, NcmContentMetaKey *out_keys, s32 count, NcmContentMetaType meta_type, u64 id, u64 id_min, u64 id_max, NcmContentInstallType install_type) {
        return ncmContentMetaDatabaseList(&this->m_db, out_entries_total, out_entries_written, out_keys, count, meta_type, id, id_min, id_max, install_type);
    }

    Result IContentMetaDatabase::GetLatestContentMetaKey(NcmContentMetaKey *out_key, u64 id) {
        return ncmContentMetaDatabaseGetLatestContentMetaKey(&this->m_db, out_key, id);
    }

    Result IContentMetaDatabase::ListApplication(s32 *out_entries_total, s32 *out_entries_written, NcmApplicationContentMetaKey *out_keys, s32 count, NcmContentMetaType meta_type) {
        return ncmContentMetaDatabaseListApplication(&this->m_db, out_entries_total, out_entries_written, out_keys, count, meta_type);
    }

    Result IContentMetaDatabase::Has(bool *out, const NcmContentMetaKey *key) {
        return ncmContentMetaDatabaseHas(&this->m_db, out, key);
    }

    Result IContentMetaDatabase::HasAll(bool *out, const NcmContentMetaKey *keys, s32 count) {
        return ncmContentMetaDatabaseHasAll(&this->m_db, out, keys, count);
    }

    Result IContentMetaDatabase::GetSize(u64 *out_size, const NcmContentMetaKey *key) {
        return ncmContentMetaDatabaseGetSize(&this->m_db, out_size, key);
    }

    Result IContentMetaDatabase::GetRequiredSystemVersion(u32 *out_version, const NcmContentMetaKey *key) {
        return ncmContentMetaDatabaseGetRequiredSystemVersion(&this->m_db, out_version, key);
    }

    Result IContentMetaDatabase::GetPatchId(u64 *out_patch_id, const NcmContentMetaKey *key) {
        return ncmContentMetaDatabaseGetPatchId(&this->m_db, out_patch_id, key);
    }

    Result IContentMetaDatabase::DisableForcibly() {
        return ncmContentMetaDatabaseDisableForcibly(&this->m_db);
    }

    Result IContentMetaDatabase::LookupOrphanContent(bool *out_orphaned, const NcmContentId *content_ids, s32 count) {
        return ncmContentMetaDatabaseLookupOrphanContent(&this->m_db, out_orphaned, content_ids, count);
    }

    Result IContentMetaDatabase::Commit() {
        return ncmContentMetaDatabaseCommit(&this->m_db);
    }

    Result IContentMetaDatabase::HasContent(bool *out, const NcmContentMetaKey *key, const NcmContentId *content_id) {
        return ncmContentMetaDatabaseHasContent(&this->m_db, out, key, content_id);
    }

    Result IContentMetaDatabase::ListContentMetaInfo(s32 *out_entries_written, void *out_meta_info, s32 count, const NcmContentMetaKey *key, s32 start_index) {
        return ncmContentMetaDatabaseListContentMetaInfo(&this->m_db, out_entries_written, out_meta_info, count, key, start_index);
    }

    Result IContentMetaDatabase::GetAttributes(const NcmContentMetaKey *key, u8 *out) {
        return ncmContentMetaDatabaseGetAttributes(&this->m_db, key, out);
    }

    Result IContentMetaDatabase::GetRequiredApplicationVersion(u32 *out_version, const NcmContentMetaKey *key) {
        return ncmContentMetaDatabaseGetRequiredApplicationVersion(&this->m_db, out_version, key);
    }

    Result IContentMetaDatabase::GetContentIdByTypeAndIdOffset(NcmContentId *out_content_id, const NcmContentMetaKey *key, NcmContentType type, u8 id_offset) {
        return ncmContentMetaDatabaseGetContentIdByTypeAndIdOffset(&this->m_db, out_content_id, key, type, id_offset);
    }

    Result IContentMetaDatabase::OpenContentMetaDatabase(NcmStorageId storage_id) {
        this->Close();
        R_TRY(ncmOpenContentMetaDatabase(&this->m_db, storage_id));
        this->open = true;
        return ResultSuccess();
    }

}
