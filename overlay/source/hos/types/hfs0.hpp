#pragma once
#include "string_table.hpp"

#include <switch.h>

namespace hfs0 {

    constexpr const u8 Magic[4] = {'H', 'F', 'S', '0'};

    struct FileEntry {
        u64 data_offset;
        u64 file_size;
        u32 string_table_offset;
        u32 hashed_size;
        u64 padding;
        u8 hash[0x20];
    };

    struct BaseHeader {
        u32 magic;
        u32 num_files;
        u32 string_table_size;
        u32 reserved;

        u64 GetHeaderSize() {
            return sizeof(hfs0::BaseHeader) + num_files * sizeof(hfs0::FileEntry) + string_table_size;
        }
        u64 GetStringTableOffset() {
            return sizeof(hfs0::BaseHeader) + num_files * sizeof(hfs0::FileEntry);
        }
        StringTable GetStringTable(const u8 *ptr) {
            return StringTable(ptr + GetStringTableOffset());
        }
        u64 GetEntryOffset() {
            return sizeof(hfs0::BaseHeader);
        }
        const hfs0::FileEntry *GetEntries(const u8 *ptr) {
            return reinterpret_cast<const hfs0::FileEntry *>(ptr + this->GetEntryOffset());
        }
    };

}
