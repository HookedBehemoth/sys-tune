#include "content_meta_reader.hpp"

#include <cstring>

namespace ncm {

    ContentMetaReader::ContentMetaReader(u8 *ptr, u64 size)
        : m_ptr(ptr), m_size(size) {}

    void ContentMetaReader::ParseInfos() {
        /* Retreive header. */
        PackedContentMetaHeader *header = this->GetHeader();

        /* Retreive content info array. */
        auto *packed_infos = reinterpret_cast<PackedContentInfo *>(this->m_ptr + sizeof(PackedContentMetaHeader) + header->extended_header_size);
        this->content_infos.clear();
        this->content_infos.reserve(header->content_count);

        /* Filter out delta fragments. */
        for (u16 i = 0; i < header->content_count; i++) {
            const auto &packed_info = packed_infos[i];
            if (packed_info.content_info.content_type < u8(ContentType::DeltaFragment))
                this->content_infos.push_back(packed_info.content_info);
        }
    }

    PackedContentMetaHeader *ContentMetaReader::GetHeader() {
        return reinterpret_cast<PackedContentMetaHeader *>(this->m_ptr);
    }

    ContentMetaKey ContentMetaReader::GetContentMetaKey() {
        PackedContentMetaHeader *header = this->GetHeader();

        return ContentMetaKey{
            .id = header->application_id,
            .version = header->version,
            .type = header->type,
        };
    }

    std::vector<u8> ContentMetaReader::MakeContentMetaValue(const ContentInfo &content_meta_info) {
        PackedContentMetaHeader *header = this->GetHeader();

        /* Add one for content meta. */
        u16 info_count = this->content_infos.size() + 1;

        /* Calculate meta size. */
        size_t buffer_size = sizeof(NcmContentMetaHeader) + header->extended_header_size + info_count * sizeof(ContentInfo);

        /* Append patch size. */
        if (header->type == ContentMetaType::Patch)
            buffer_size += this->GetExtendedHeader<NcmPatchMetaExtendedHeader>()->extended_data_size;

        std::vector<u8> buffer(buffer_size, '\0');

        u8 *data = buffer.data();

        /* Write Content meta header. */
        NcmContentMetaHeader *meta_header = reinterpret_cast<NcmContentMetaHeader *>(data);
        meta_header->extended_header_size = header->extended_header_size;
        meta_header->content_count = info_count;
        data += sizeof(NcmContentMetaHeader);

        /* Write extended header data. */
        std::memcpy(data, this->GetExtendedHeader<u8>(), header->extended_header_size);
        data += header->extended_header_size;

        /* Add content meta info. */
        std::memcpy(data, &content_meta_info, sizeof(ContentInfo));
        data += sizeof(ContentInfo);

        /* Add remaining content infos from cnmt. */
        size_t infos_size = this->content_infos.size() * sizeof(ContentInfo);
        std::memcpy(data, this->content_infos.data(), infos_size);
        data += infos_size;

        /* If patch, append patch extra data. */
        if (header->type == ContentMetaType::Patch) {
            auto *ext_header = this->GetExtendedHeader<PatchMetaExtendedHeader>();
            std::memcpy(data, this->m_ptr + sizeof(PackedContentMetaHeader) + header->extended_header_size + header->content_count * sizeof(PackedContentInfo), ext_header->extended_data_size);
        }

        return buffer;
    }

    void ContentMetaReader::IgnoreRequiredVersion(bool system, bool application) {
        /* Retreive header. */
        PackedContentMetaHeader *header = this->GetHeader();

        /* Cast type specific header and change version. */
        if (header->type == ContentMetaType::Application) {
            auto *ext_header = this->GetExtendedHeader<ApplicationMetaExtendedHeader>();
            if (system)
                ext_header->required_system_version = {};
            if (application)
                ext_header->required_application_version = {};
        } else if (header->type == ContentMetaType::Patch) {
            auto *ext_header = this->GetExtendedHeader<PatchMetaExtendedHeader>();
            if (system)
                ext_header->required_system_version = {};
        } else if (header->type == ContentMetaType::AddOnContent) {
            auto *ext_header = this->GetExtendedHeader<AddOnContentMetaExtendedHeader>();
            if (application)
                ext_header->required_application_version = {};
        }
    }

    const std::vector<ContentInfo> &ContentMetaReader::GetContentInfos() {
        /* Return filtered vector. */
        return this->content_infos;
    }

}
