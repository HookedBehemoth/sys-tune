#pragma once

#include <opus/opusfile.h>

namespace hwopus {

    bool Allocate();
    void Free();
    int OpusDecodeCallback(void *ctx, OpusMSDecoder *decoder, void *pcm, const ogg_packet *op, int nsamples, int nchannels, int format, int li);

}
