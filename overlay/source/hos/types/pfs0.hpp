#pragma once
#include "string_table.hpp"

#include <switch.h>

namespace pfs0 {

    constexpr const u8 Magic[4] = {'P', 'F', 'S', '0'};

    struct FileEntry {
        u64 data_offset;
        u64 file_size;
        u32 string_table_offset;
        u32 padding;
    };

    struct BaseHeader {
        u32 magic;
        u32 num_files;
        u32 string_table_size;
        u32 reserved;

        u64 GetHeaderSize() {
            return sizeof(pfs0::BaseHeader) + num_files * sizeof(pfs0::FileEntry) + string_table_size;
        }
        u64 GetStringTableOffset() {
            return sizeof(pfs0::BaseHeader) + num_files * sizeof(pfs0::FileEntry);
        }
        StringTable GetStringTable(const u8 *ptr) {
            return StringTable(ptr + this->GetStringTableOffset());
        }
        u64 GetEntryOffset() {
            return sizeof(pfs0::BaseHeader);
        }
        const pfs0::FileEntry *GetEntries(const u8 *ptr) {
            return reinterpret_cast<const pfs0::FileEntry *>(ptr + this->GetEntryOffset());
        }
    };

}
