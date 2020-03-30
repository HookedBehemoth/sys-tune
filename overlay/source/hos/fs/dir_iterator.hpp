#pragma once

#include "IFileSystem.hpp"

class DirectoryIterator {
  private:
    IDirectory *m_dir;
    FsDirectoryEntry entry;
    s64 count;

  public:
    DirectoryIterator() = default;
    DirectoryIterator(IDirectory *dir);

    ~DirectoryIterator() = default;

    const FsDirectoryEntry &operator*() const;
    const FsDirectoryEntry *operator->() const;
    DirectoryIterator &operator++();

    bool operator!=(const DirectoryIterator &rhs);
};

inline DirectoryIterator begin(DirectoryIterator iter) noexcept { return iter; }

inline DirectoryIterator end(DirectoryIterator) noexcept { return DirectoryIterator(); }
