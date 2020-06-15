#pragma once

struct AlphaSymbol {
    int width;
    int height;
    const unsigned char *data;
};

namespace symbol {
    extern AlphaSymbol RepeatAll;
    extern AlphaSymbol RepeatOne;
    extern AlphaSymbol SdCard;
    extern AlphaSymbol Shuffle;
    extern AlphaSymbol Prev;
    extern AlphaSymbol Next;
    extern AlphaSymbol Play;
    extern AlphaSymbol Pause;
    extern AlphaSymbol Stop;
}
