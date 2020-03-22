#pragma once
#include "string_table.hpp"

#include <switch.h>

constexpr const u8 HFS0Magic[4] = {'H', 'F', 'S', '0'};

struct HFS0FileEntry {
    u64 dataOffset;
    u64 fileSize;
    u32 stringTableOffset;
    u32 hashedSize;
    u64 padding;
    u8 hash[0x20];
};

struct HFS0BaseHeader {
    u32 magic;
    u32 numFiles;
    u32 stringTableSize;
    u32 reserved;

    u64 GetHeaderSize() {
        return sizeof(HFS0BaseHeader) + numFiles * sizeof(HFS0FileEntry) + stringTableSize;
    }
    u64 GetStringTableOffset() {
        return sizeof(HFS0BaseHeader) + numFiles * sizeof(HFS0FileEntry);
    }
    StringTable GetStringTable(const u8 *ptr) {
        return StringTable(ptr + GetStringTableOffset());
    }
    u64 GetEntryOffset() {
        return sizeof(HFS0BaseHeader);
    }
    const HFS0FileEntry *GetEntries(const u8 *ptr) {
        return reinterpret_cast<const HFS0FileEntry *>(ptr + this->GetEntryOffset());
    }
};
