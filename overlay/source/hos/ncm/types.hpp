#pragma once

#include "../util/defines.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <switch.h>

namespace ncm {

    struct Version {
        union {
            struct {
                u32 bugfix : 16;
                u32 micro : 4;
                u32 minor : 6;
                u32 major : 6;
            };
            u32 version;
        };
        Version()
            : version(0) {}
        Version(u8 major, u8 minor, u8 micro, u16 bugfix)
            : bugfix(bugfix), micro(micro), minor(minor), major(major) {
        }
        ALWAYS_INLINE bool operator<(const Version &v) {
            return this->version < v.version;
        }
        ALWAYS_INLINE bool operator>(const Version &v) {
            return this->version > v.version;
        }
        ALWAYS_INLINE bool operator==(const Version &v) {
            return this->version == v.version;
        }
        ALWAYS_INLINE bool operator!=(const Version &v) {
            return this->version == v.version;
        }
    };
    static_assert(sizeof(Version) == 0x4);

    struct SystemVersion : Version {
        bool IsSupported() {
            u32 hos_version = hosversionGet();
            return (this->major <= HOSVER_MAJOR(hos_version)) || (this->minor <= HOSVER_MINOR(hos_version)) || (this->micro <= HOSVER_MICRO(hos_version));
        }
    };
    static_assert(sizeof(SystemVersion) == 0x4);

    std::string StringFromContentId(const NcmContentId &nca_id);
    NcmContentId ContentIdFromString(const char *src);

    enum class ContentMetaType : u8 {
        Any = 0,
        SystemProgram = 0x1,
        SystemData = 0x2,
        SystemUpdate = 0x3,
        BootImagePackage = 0x4,
        BootImagePackageSafe = 0x5,
        Application = 0x80,
        Patch = 0x81,
        AddOnContent = 0x82,
        Delta = 0x83,
    };

    enum class ContentType : u8 {
        Meta = 0x0,
        Program = 0x1,
        Data = 0x2,
        Control = 0x3,
        OfflineHtml = 0x4,
        LegalInformation = 0x5,
        DeltaFragment = 0x6,
    };

    enum class ContentInstallType : u8 {
        Full = 0,
        FragmentOnly = 1,
        Unknown = 7,
    };

    enum ContentMetaAttribute {
        ContentMetaAttribute_None = 0,
        ContentMetaAttribute_IncludesExFatDriver = BIT(0),
        ContentMetaAttribute_Rebootless = BIT(1),
    };

    struct ContentMetaKey {
        u64 id;
        Version version;
        ContentMetaType type;
        ContentInstallType install_type;
    };
    static_assert(sizeof(ContentMetaKey) == 0x10);

    struct ContentInfo {
        NcmContentId content_id;
        u64 size : 48;
        u64 content_type : 8;
        u64 id_offset : 8;
    };
    static_assert(sizeof(ContentInfo) == 0x18);

    struct ContentMetaInfo {
        u64 id;
        Version version;
        ContentMetaType type;
        u8 attr; ///< \ref ContentMetaAttribute
    };
    static_assert(sizeof(ContentMetaInfo) == 0x10);

    struct ApplicationMetaExtendedHeader {
        u64 patch_id;
        SystemVersion required_system_version;
        Version required_application_version;
    };
    static_assert(sizeof(ApplicationMetaExtendedHeader) == 0x10);

    struct PatchMetaExtendedHeader {
        u64 application_id;
        SystemVersion required_system_version;
        u32 extended_data_size;
        u8 reserved[0x8];
    };
    static_assert(sizeof(PatchMetaExtendedHeader) == 0x18);

    struct AddOnContentMetaExtendedHeader {
        u64 application_id;
        Version required_application_version;
    };
    static_assert(sizeof(AddOnContentMetaExtendedHeader) == 0x10);

    struct PackedContentMetaHeader {
        u64 application_id;
        Version version;
        ContentMetaType type;
        u16 extended_header_size;
        u16 content_count;
        u16 content_meta_count;
        u8 attributes;
        u8 storage_id;
        u8 install_type;
        bool comitted;
        SystemVersion RequiredDownloadSystemVersion;
    };
    static_assert(sizeof(PackedContentMetaHeader) == 0x20);

    struct PackedContentInfo {
        u8 hash[SHA256_HASH_SIZE];
        ContentInfo content_info;
    };
    static_assert(sizeof(PackedContentInfo) == 0x38);

}
