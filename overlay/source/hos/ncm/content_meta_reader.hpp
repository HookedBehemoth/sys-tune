#pragma once

#include "../util/result.hpp"
#include "types.hpp"

#include <vector>

namespace ncm {

    class ContentMetaReader {
      private:
        u8 *m_ptr;
        u64 m_size;
        std::vector<ContentInfo> content_infos;

      public:
        ContentMetaReader(u8 *ptr, u64 size);
        void ParseInfos();

      private:
        PackedContentMetaHeader *GetHeader();

        template <typename HeaderType>
        HeaderType *GetExtendedHeader() {
            return reinterpret_cast<HeaderType *>(this->m_ptr + sizeof(PackedContentMetaHeader));
        }

      public:
        ContentMetaKey GetContentMetaKey();
        std::vector<u8> MakeContentMetaValue(const ContentInfo &content_meta_info);

        const std::vector<ContentInfo> &GetContentInfos();

        void IgnoreRequiredVersion(bool system, bool application);
    };

}
