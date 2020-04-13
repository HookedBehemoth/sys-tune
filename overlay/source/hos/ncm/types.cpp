#include "types.hpp"

namespace ncm {

    std::string StringFromContentId(const NcmContentId &nca_id) {
        std::string dst;
        dst.resize(0x21);
        char *ptr = dst.data();
        for (const u8 c : nca_id.c) {
            std::snprintf(ptr, 3, "%02X", c);
            ptr += 2;
        }
        return dst;
    }

    NcmContentId ContentIdFromString(const char *src) {
        NcmContentId out;
        for (size_t i = 0; i < 0x20; i += 2) {
            char tmp[3];
            strlcpy(tmp, src + i, sizeof(tmp));
            out.c[i / 2] = static_cast<u8>(std::strtoul(tmp, nullptr, 16));
        }
        return out;
    }

}
