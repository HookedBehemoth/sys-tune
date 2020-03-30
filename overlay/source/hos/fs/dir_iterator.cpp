#include "dir_iterator.hpp"

DirectoryIterator::DirectoryIterator(IDirectory *dir)
    : m_dir(dir) {
    m_dir->Read(&this->count, &this->entry, 1);
}

const FsDirectoryEntry &DirectoryIterator::operator*() const {
    return entry;
}

const FsDirectoryEntry *DirectoryIterator::operator->() const {
    return &**this;
}

DirectoryIterator &DirectoryIterator::operator++() {
    this->m_dir->Read(&this->count, &this->entry, 1);
    return *this;
}

bool DirectoryIterator::operator!=(const DirectoryIterator &__rhs) {
    return this->count > 0;
}
