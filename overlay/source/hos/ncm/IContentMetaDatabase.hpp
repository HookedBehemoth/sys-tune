#pragma once

#include <switch.h>

namespace ncm {

    class IContentMetaDatabase {
        IContentMetaDatabase(const IContentMetaDatabase &) = delete;
        IContentMetaDatabase &operator=(const IContentMetaDatabase &) = delete;

      private:
        NcmContentMetaDatabase m_db;
        bool open;

      public:
        IContentMetaDatabase();
        IContentMetaDatabase(NcmContentMetaDatabase &&db);
        IContentMetaDatabase(IContentMetaDatabase &&db);
        IContentMetaDatabase &operator=(IContentMetaDatabase &&db);
        ~IContentMetaDatabase();

      public:
        bool IsOpen();
        void ForceClose();

      private:
        void Close();

      public:
        Result Set(const NcmContentMetaKey *key, const void *data, u64 data_size);
        Result Get(const NcmContentMetaKey *key, u64 *out_size, void *out_data, u64 out_data_size);
        Result Remove(const NcmContentMetaKey *key);
        Result GetContentIdByType(NcmContentId *out_content_id, const NcmContentMetaKey *key, NcmContentType type);
        Result ListContentInfo(s32 *out_entries_written, NcmContentInfo *out_info, s32 count, const NcmContentMetaKey *key, s32 start_index);
        Result List(s32 *out_entries_total, s32 *out_entries_written, NcmContentMetaKey *out_keys, s32 count, NcmContentMetaType meta_type, u64 id, u64 id_min, u64 id_max, NcmContentInstallType install_type);
        Result GetLatestContentMetaKey(NcmContentMetaKey *out_key, u64 id);
        Result ListApplication(s32 *out_entries_total, s32 *out_entries_written, NcmApplicationContentMetaKey *out_keys, s32 count, NcmContentMetaType meta_type);
        Result Has(bool *out, const NcmContentMetaKey *key);
        Result HasAll(bool *out, const NcmContentMetaKey *keys, s32 count);
        Result GetSize(u64 *out_size, const NcmContentMetaKey *key);
        Result GetRequiredSystemVersion(u32 *out_version, const NcmContentMetaKey *key);
        Result GetPatchId(u64 *out_patch_id, const NcmContentMetaKey *key);
        Result DisableForcibly();
        Result LookupOrphanContent(bool *out_orphaned, const NcmContentId *content_ids, s32 count);
        Result Commit();
        Result HasContent(bool *out, const NcmContentMetaKey *key, const NcmContentId *content_id);
        Result ListContentMetaInfo(s32 *out_entries_written, void *out_meta_info, s32 count, const NcmContentMetaKey *key, s32 start_index);
        Result GetAttributes(const NcmContentMetaKey *key, u8 *out);

        /* [2.0.0+] */
        Result GetRequiredApplicationVersion(u32 *out_version, const NcmContentMetaKey *key);

        /* [5.0.0+] */
        Result GetContentIdByTypeAndIdOffset(NcmContentId *out_content_id, const NcmContentMetaKey *key, NcmContentType type, u8 id_offset);

      public:
        Result OpenContentMetaDatabase(NcmStorageId storage_id);
    };

}
