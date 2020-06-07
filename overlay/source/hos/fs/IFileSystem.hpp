#pragma once
#include "../util/defines.hpp"
#include "IDirectory.hpp"
#include "IFile.hpp"

#include <cstdio>
#include <memory>
#include <switch.h>

namespace fs {

    class IFileSystem {
        NON_COPYABLE(IFileSystem);

      private:
        ::FsFileSystem m_fs;
        bool open;
        char format_buffer[FS_MAX_PATH];

      public:
        IFileSystem();
        IFileSystem(FsFileSystem &&fs);
        IFileSystem(IFileSystem &&fs);
        IFileSystem &operator=(IFileSystem &&fs);
        ~IFileSystem();

      public:
        bool IsOpen();
        void ForceClose();

      private:
        void Close();

      public:
        Result CreateFile(s64 size, u32 option, const char *path);
        Result DeleteFile(const char *path);
        Result CreateDirectory(const char *path);
        Result DeleteDirectory(const char *path);
        Result DeleteDirectoryRecursively(const char *path);
        Result RenameFile(const char *cur_path, const char *new_path);
        Result RenameDirectory(const char *cur_path, const char *new_path);
        Result GetEntryType(FsDirEntryType *out, const char *path);
        Result OpenFile(IFile *out, u32 mode, const char *path);
        Result OpenFile(std::unique_ptr<IFile> *out, u32 mode, const char *path);
        Result OpenDirectory(IDirectory *out, u32 mode, const char *path);
        Result OpenDirectory(std::unique_ptr<IDirectory> *out, u32 mode, const char *path);
        Result Commit();
        Result GetFreeSpace(s64 *out, const char *path);
        Result GetTotalSpace(s64 *out, const char *path);

        /* [3.0.0+] */
        Result GetFileTimeStampRaw(FsTimeStampRaw *out, const char *path);

        /* Format */
        template <typename... Args>
        Result CreateFileFormat(s64 size, u32 option, const char *format, Args &&... args) {
            std::snprintf(this->format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->CreateFile(size, option, format_buffer);
        }
        template <typename... Args>
        Result DeleteFileFormat(const char *format, Args &&... args) {
            std::snprintf(format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->DeleteFile(format_buffer);
        }
        template <typename... Args>
        Result CreateDirectoryFormat(const char *format, Args &&... args) {
            std::snprintf(format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->CreateDirectory(format_buffer);
        }
        template <typename... Args>
        Result DeleteDirectoryFormat(const char *format, Args &&... args) {
            std::snprintf(format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->DeleteDirectory(format_buffer);
        }
        template <typename... Args>
        Result DeleteDirectoryRecursivelyFormat(const char *format, Args &&... args) {
            std::snprintf(format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->DeleteDirectoryRecursively(format_buffer);
        }
        template <typename File, typename... Args>
        Result OpenFileFormat(File *out, u32 mode, const char *format, Args &&... args) {
            std::snprintf(format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->OpenFile(out, mode, format_buffer);
        }
        template <typename Dir, typename... Args>
        Result OpenDirectoryFormat(Dir *out, u32 mode, const char *format, Args &&... args) {
            std::snprintf(format_buffer, FS_MAX_PATH, format, std::forward<Args>(args)...);
            return this->OpenDirectory(out, mode, format_buffer);
        }

      public:
        Result OpenFileSystem(FsFileSystemType fsType, const char *contentPath);
        Result OpenFileSystemWithPatch(u64 id, FsFileSystemType fsType); /// [2.0.0+]
        Result OpenFileSystemWithId(u64 id, FsFileSystemType fsType, const char *contentPath);
        Result OpenBisFileSystem(FsBisPartitionId partitionId, const char *string);
        Result OpenSdCardFileSystem();
        Result OpenImageDirectoryFileSystem(FsImageDirectoryId image_directory_id);
        Result OpenContentStorageFileSystem(FsContentStorageId content_storage_id);
        Result OpenCustomStorageFileSystem(FsCustomStorageId custom_storage_id); /// [7.0.0+]
    };

}
