#pragma once
#include <cstddef>

struct StringTable {
    const char *ptr;

    StringTable(const u8 *data)
        : ptr(reinterpret_cast<const char *>(data)) {}

    StringTable(const char *data)
        : ptr(data) {}

    template <typename Entry>
    const char *GetName(const Entry &entry) { return ptr + entry.string_table_offset; }
    const char *GetString(size_t offset) { return ptr + offset; }
};
