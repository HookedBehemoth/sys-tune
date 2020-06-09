#include "nx_hwopus.hpp"

#include <cstring>
#include <new>
#include <switch.h>

namespace hwopus {

    namespace {

        constexpr const size_t OpusBufferSize = sizeof(HwopusHeader) + 4096 * 48;
        u8 *opus_buffer                       = nullptr;

    }

    bool Allocate() {
        opus_buffer = new (std::nothrow) u8[OpusBufferSize];
        return opus_buffer != nullptr;
    }

    void Free() {
        if (opus_buffer != nullptr) {
            delete[] opus_buffer;
            opus_buffer = nullptr;
        }
    }

    int OpusDecodeCallback(void *ctx, OpusMSDecoder *opus_decoder, void *pcm, const ogg_packet *op, int nsamples, int nchannels, int format, int li) {
        HwopusDecoder *decoder = static_cast<HwopusDecoder *>(ctx);
        HwopusHeader *hdr      = NULL;
        size_t pktsize, pktsize_extra;

        s32 DecodedDataSize    = 0;
        s32 DecodedSampleCount = 0;

        if (format != OP_DEC_FORMAT_SHORT)
            return OPUS_BAD_ARG;

        pktsize       = op->bytes;   //Opus packet size.
        pktsize_extra = pktsize + 8; //Packet size with HwopusHeader.

        if (pktsize_extra > OpusBufferSize)
            return OPUS_INTERNAL_ERROR;

        hdr = (HwopusHeader *)opus_buffer;
        memset(opus_buffer, 0, pktsize_extra);

        hdr->size = __builtin_bswap32(pktsize);
        memcpy(&opus_buffer[sizeof(HwopusHeader)], op->packet, pktsize);

        Result rc = hwopusDecodeInterleaved(decoder, &DecodedDataSize, &DecodedSampleCount, opus_buffer, pktsize_extra, static_cast<s16 *>(pcm), nsamples * nchannels * sizeof(opus_int16));

        if (R_FAILED(rc))
            return OPUS_INTERNAL_ERROR;
        if (static_cast<size_t>(DecodedDataSize) != pktsize_extra || DecodedSampleCount != nsamples)
            return OPUS_INVALID_PACKET;

        return 0;
    }

}
