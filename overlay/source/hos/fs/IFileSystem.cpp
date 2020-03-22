#include "IFileSystem.hpp"

#include <utility>

IFileSystem::IFileSystem()
    : m_fs(), open(false) {}

IFileSystem::IFileSystem(FsFileSystem &&fs)
    : m_fs(fs), open(true) {
    fs = {};
}

IFileSystem::IFileSystem(IFileSystem &&fs)
    : m_fs(fs.m_fs), open(fs.open) {
    fs.m_fs = {};
    fs.open = false;
}

IFileSystem &IFileSystem::operator=(IFileSystem &&fs) {
    std::swap(this->m_fs, fs.m_fs);
    std::swap(this->open, fs.open);
    return *this;
}

IFileSystem::~IFileSystem() {
    this->Close();
}

bool IFileSystem::IsOpen() {
    return this->open;
}

void IFileSystem::ForceClose() {
    this->Close();
}

void IFileSystem::Close() {
    if (this->open) {
        fsFsClose(&this->m_fs);
        this->open = false;
    }
}

Result IFileSystem::CreateFile(s64 size, u32 option, const char *path) {
    return fsFsCreateFile(&this->m_fs, path, size, option);
}

Result IFileSystem::DeleteFile(const char *path) {
    return fsFsDeleteFile(&this->m_fs, path);
}

Result IFileSystem::CreateDirectory(const char *path) {
    return fsFsCreateDirectory(&this->m_fs, path);
}

Result IFileSystem::DeleteDirectory(const char *path) {
    return fsFsDeleteFile(&this->m_fs, path);
}

Result IFileSystem::DeleteDirectoryRecursively(const char *path) {
    return fsFsDeleteDirectoryRecursively(&this->m_fs, path);
}

Result IFileSystem::RenameFile(const char *cur_path, const char *new_path) {
    return fsFsRenameFile(&this->m_fs, cur_path, new_path);
}

Result IFileSystem::RenameDirectory(const char *cur_path, const char *new_path) {
    return fsFsRenameDirectory(&this->m_fs, cur_path, new_path);
}

Result IFileSystem::GetEntryType(FsDirEntryType *out, const char *path) {
    return fsFsGetEntryType(&this->m_fs, path, out);
}

Result IFileSystem::OpenFile(std::unique_ptr<IFile> *out, u32 mode, const char *path) {
    FsFile fsFile;
    Result rc = fsFsOpenFile(&this->m_fs, path, mode, &fsFile);
    if (R_SUCCEEDED(rc))
        *out = std::make_unique<IFile>(std::move(fsFile));
    return rc;
}

Result IFileSystem::OpenFile(IFile *out, u32 mode, const char *path) {
    FsFile fsFile;
    Result rc = fsFsOpenFile(&this->m_fs, path, mode, &fsFile);
    if (R_SUCCEEDED(rc))
        *out = IFile(std::move(fsFile));
    return rc;
}

Result IFileSystem::OpenDirectory(std::unique_ptr<IDirectory> *out, u32 mode, const char *path) {
    FsDir fsDir;
    Result rc = fsFsOpenDirectory(&this->m_fs, path, mode, &fsDir);
    if (R_SUCCEEDED(rc))
        *out = std::make_unique<IDirectory>(std::move(fsDir));
    return rc;
}

Result IFileSystem::OpenDirectory(IDirectory *out, u32 mode, const char *path) {
    FsDir fsDir;
    Result rc = fsFsOpenDirectory(&this->m_fs, path, mode, &fsDir);
    if (R_SUCCEEDED(rc) && out)
        *out = IDirectory(std::move(fsDir));
    return rc;
}

Result IFileSystem::Commit() {
    return fsFsCommit(&this->m_fs);
}

Result IFileSystem::GetFreeSpace(s64 *out, const char *path) {
    return fsFsGetFreeSpace(&this->m_fs, path, out);
}

Result IFileSystem::GetTotalSpace(s64 *out, const char *path) {
    return fsFsGetTotalSpace(&this->m_fs, path, out);
}

Result IFileSystem::GetFileTimeStampRaw(FsTimeStampRaw *out, const char *path) {
    return fsFsGetFileTimeStampRaw(&this->m_fs, path, out);
}
