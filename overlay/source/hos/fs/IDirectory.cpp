#include "IDirectory.hpp"

#include <utility>

IDirectory::IDirectory()
    : m_dir(), open(false) {}

IDirectory::IDirectory(FsDir &&dir)
    : m_dir(dir), open(true) {
    dir = {};
}

IDirectory::IDirectory(IDirectory &&dir)
    : m_dir(dir.m_dir), open(dir.open) {
    dir.m_dir = {};
    dir.open = false;
}

IDirectory &IDirectory::operator=(IDirectory &&dir) {
    std::swap(this->m_dir, dir.m_dir);
    std::swap(this->open, dir.open);
    return *this;
}

IDirectory::~IDirectory() {
    this->Close();
}

void IDirectory::ForceClose() {
    this->Close();
}

bool IDirectory::IsOpen() {
    return this->open;
}

void IDirectory::Close() {
    if (this->open) {
        fsDirClose(&this->m_dir);
        this->open = false;
    }
}

Result IDirectory::Read(s64 *total_entries, FsDirectoryEntry *entries, size_t max_entries) {
    return fsDirRead(&this->m_dir, total_entries, max_entries, entries);
}

Result IDirectory::GetEntryCount(s64 *count) {
    return fsDirGetEntryCount(&this->m_dir, count);
}
