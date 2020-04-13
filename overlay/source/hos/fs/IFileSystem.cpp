#include "IFileSystem.hpp"

#include "../util/result.hpp"

#include <utility>

namespace fs {

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
        R_TRY(fsFsOpenFile(&this->m_fs, path, mode, &fsFile));
        *out = std::make_unique<IFile>(std::move(fsFile));
        return ResultSuccess();
    }

    Result IFileSystem::OpenFile(IFile *out, u32 mode, const char *path) {
        FsFile fsFile;
        R_TRY(fsFsOpenFile(&this->m_fs, path, mode, &fsFile));
        *out = IFile(std::move(fsFile));
        return ResultSuccess();
    }

    Result IFileSystem::OpenDirectory(std::unique_ptr<IDirectory> *out, u32 mode, const char *path) {
        FsDir fsDir;
        R_TRY(fsFsOpenDirectory(&this->m_fs, path, mode, &fsDir));
        *out = std::make_unique<IDirectory>(std::move(fsDir));
        return ResultSuccess();
    }

    Result IFileSystem::OpenDirectory(IDirectory *out, u32 mode, const char *path) {
        FsDir fsDir;
        R_TRY(fsFsOpenDirectory(&this->m_fs, path, mode, &fsDir));
        *out = IDirectory(std::move(fsDir));
        return ResultSuccess();
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

    Result IFileSystem::OpenFileSystem(FsFileSystemType fsType, const char *contentPath) {
        this->Close();
        R_TRY(fsOpenFileSystem(&this->m_fs, fsType, contentPath));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenFileSystemWithPatch(u64 id, FsFileSystemType fsType) {
        this->Close();
        R_TRY(fsOpenFileSystemWithPatch(&this->m_fs, id, fsType));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenFileSystemWithId(u64 id, FsFileSystemType fsType, const char *contentPath) {
        this->Close();
        R_TRY(fsOpenFileSystemWithId(&this->m_fs, id, fsType, contentPath));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenBisFileSystem(FsBisPartitionId partitionId, const char *string) {
        this->Close();
        R_TRY(fsOpenBisFileSystem(&this->m_fs, partitionId, string));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenSdCardFileSystem() {
        this->Close();
        R_TRY(fsOpenSdCardFileSystem(&this->m_fs));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenImageDirectoryFileSystem(FsImageDirectoryId id) {
        this->Close();
        R_TRY(fsOpenImageDirectoryFileSystem(&this->m_fs, id));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenContentStorageFileSystem(FsContentStorageId content_storage_id) {
        this->Close();
        R_TRY(fsOpenContentStorageFileSystem(&this->m_fs, content_storage_id));
        this->open = true;
        return ResultSuccess();
    }

    Result IFileSystem::OpenCustomStorageFileSystem(FsCustomStorageId custom_storage_id) {
        this->Close();
        R_TRY(fsOpenCustomStorageFileSystem(&this->m_fs, custom_storage_id));
        this->open = true;
        return ResultSuccess();
    }

}
