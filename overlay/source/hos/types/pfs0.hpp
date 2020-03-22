#pragma once
#include "string_table.hpp"

#include <switch.h>

constexpr const u8 PFS0Magic[4] = {'P', 'F', 'S', '0'};

struct PFS0FileEntry {
    u64 dataOffset;
    u64 fileSize;
    u32 stringTableOffset;
    u32 padding;
};

struct PFS0BaseHeader {
    u32 magic;
    u32 numFiles;
    u32 stringTableSize;
    u32 reserved;

    u64 GetHeaderSize() {
        return sizeof(PFS0BaseHeader) + numFiles * sizeof(PFS0FileEntry) + stringTableSize;
    }
    u64 GetStringTableOffset() {
        return sizeof(PFS0BaseHeader) + numFiles * sizeof(PFS0FileEntry);
    }
    StringTable GetStringTable(const u8 *ptr) {
        return StringTable(ptr + this->GetStringTableOffset());
    }
    u64 GetEntryOffset() {
        return sizeof(PFS0BaseHeader);
    }
    const PFS0FileEntry *GetEntries(const u8 *ptr) {
        return reinterpret_cast<const PFS0FileEntry *>(ptr + this->GetEntryOffset());
    }
};
