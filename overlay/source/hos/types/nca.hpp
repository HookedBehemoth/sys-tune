#pragma once

#include <switch.h>

namespace nca {

    constexpr const u8 Magic0[4] = {'N', 'C', 'A', '0'};
    constexpr const u8 Magic1[4] = {'N', 'C', 'A', '1'};
    constexpr const u8 Magic2[4] = {'N', 'C', 'A', '2'};
    constexpr const u8 Magic3[4] = {'N', 'C', 'A', '3'};

    struct HierarchicalSha256 {
        u8 hash[0x20];
        u32 blockSize;
        u32 unk_x24;
        u64 hashTableOffset;
        u64 hashTableSize;
        u64 relativeOffset;
        u64 relativeSize;
    };
    static_assert(sizeof(HierarchicalSha256) == 0x48);

    struct HierarchicalIntegrity {
        u64 pad_x0;
        u32 magic;
        u32 unk_xc;
        u32 masterHashSize;
        u32 unk_x14;
        struct {
            u64 offset;
            u64 size;
            u32 blockSize;
            u32 pad_x14;
        } level[6];
        u8 pad_xa8[0x20];
        u8 hash[0x20];
        u8 pad_xe8[0x10];
    };
    static_assert(sizeof(HierarchicalIntegrity) == 0xF8);

    struct PatchInfo {
        struct {
            u64 offset;
            u64 size;
            u32 magic;
            u32 unk_x14;
            u32 unk_x18;
            u32 unk_x1c;
        } entry[2];
    };

    enum class FsType : u8 {
        RomFs = 0,
        PartitionFs = 1,
    };

    enum class FsHashType : u8 {
        Auto = 0,
        HierarchicalSha256 = 2,
        HierarchicalIntegrity = 3,
    };

    enum class FsEncryptionType : u8 {
        Auto = 0,
        None = 1,
        AesCtrOld = 2,
        AesCtr = 3,
        AesCtrEx = 4,
    };

    struct FsEntry {
        u32 startOffset;
        u32 endOffset;
        u8 pad_x8[0x8];
    };
    static_assert(sizeof(nca::FsEntry) == 0x10);

    struct FsHeader {
        u16 version;
        nca::FsType fsType;
        nca::FsHashType hashType;
        nca::FsEncryptionType encryptionType;
        u8 pad_x5[3];
        union HashInfo {
            HierarchicalSha256 sha256;
            HierarchicalIntegrity integrity;
        };
        PatchInfo pathInfo;
        u32 generation;
        u32 secureValue;
        u8 sparseInfo[0x30];
        u8 pad_x178[0x88];
    };

    enum class DistributionType : u8 {
        System = 0,
        Gamecard = 1,
    };

    enum class ContentType : u8 {
        Program = 0,
        Meta = 1,
        Control = 2,
        Manual = 3,
        Data = 4,
        PublicData = 5,
    };

    enum class KeyAreaEncryptionKeyIndex : u8 {
        Application = 0,
        Ocean = 1,
        System = 2,
    };

    enum class KeyGenerationOld : u8 {
        Version100 = 0,
        Version200 = 2,
    };

    enum class KeyGeneration : u8 {
        Version301 = 3,
        Version400 = 4,
        Version500 = 5,
        Version600 = 6,
        Version620 = 7,
        Version700 = 8,
        Version810 = 9,
        Version900 = 10,
        Version910 = 11,
        Invalid = 0xff,
    };

    enum class FixedKeyGeneration : u8 {
        Version100 = 0,
        Version910 = 1,
    };

    struct Header {
        u8 fixedKeySignature[0x100]; /* RSA-2048 signature over the header using a fixed key. */
        u8 npdmKeySignature[0x100];  /* RSA-2048 signature over the header using a key from NPDM. */
        u8 magic[4];                 /* Magicnum "NCA3" ("NCA2", "NCA1" or "NCA0" for pre-1.0.0 NCAs) */
        nca::DistributionType distribution;
        nca::ContentType content;
        nca::KeyGenerationOld oldKeyGeneration;
        nca::KeyAreaEncryptionKeyIndex keyIndex;
        u64 contentSize; /* Entire archive size. */
        u64 programId;
        u32 contentIndex;
        union {
            u32 sdkVersion; /* What SDK was this built with? */
            struct {
                u8 sdkRevision;
                u8 sdkMicro;
                u8 sdkMinor;
                u8 sdkMajor;
            };
        };
        nca::KeyGeneration keyGeneration;           /* Which key generation do we need. */
        nca::FixedKeyGeneration fixedKeyGeneration; /* Which fixed key to use. */
        u8 pad_x220[0xe];
        FsRightsId rightsId;
        nca::FsEntry fsEntries[4];
        u8 fsEntryHashes[0x20][4];
        u8 encryptedKeyArea[0x10][4];
        u8 pad[0xc0];
    };
    static_assert(sizeof(nca::Header) == 0x400);

}
