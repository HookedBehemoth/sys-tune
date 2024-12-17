/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_audio.h
 *
 *  Access to the raw audio mixing buffer for the SDL library.
 */

#ifndef SDL_audio_EX_h_
#define SDL_audio_EX_h_

#include <stdint.h>
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief Audio format flags.
 *
 *  These are what the 16 bits in SDL_AudioFormat currently mean...
 *  (Unspecified bits are always zero).
 *
 *  \verbatim
    ++-----------------------sample is signed if set
    ||
    ||       ++-----------sample is bigendian if set
    ||       ||
    ||       ||          ++---sample is float if set
    ||       ||          ||
    ||       ||          || +---sample bit size---+
    ||       ||          || |                     |
    15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    \endverbatim
 *
 *  There are macros in SDL 2.0 and later to query these bits.
 */
typedef uint16_t SDL_AudioFormat;

/**
 *  \name Audio flags
 */
/* @{ */

#define SDL_AUDIO_MASK_BITSIZE       (0xFF)
#define SDL_AUDIO_MASK_DATATYPE      (1<<8)
#define SDL_AUDIO_MASK_ENDIAN        (1<<12)
#define SDL_AUDIO_MASK_SIGNED        (1<<15)
#define SDL_AUDIO_BITSIZE(x)         (x & SDL_AUDIO_MASK_BITSIZE)
#define SDL_AUDIO_ISFLOAT(x)         (x & SDL_AUDIO_MASK_DATATYPE)
#define SDL_AUDIO_ISBIGENDIAN(x)     (x & SDL_AUDIO_MASK_ENDIAN)
#define SDL_AUDIO_ISSIGNED(x)        (x & SDL_AUDIO_MASK_SIGNED)
#define SDL_AUDIO_ISINT(x)           (!SDL_AUDIO_ISFLOAT(x))
#define SDL_AUDIO_ISLITTLEENDIAN(x)  (!SDL_AUDIO_ISBIGENDIAN(x))
#define SDL_AUDIO_ISUNSIGNED(x)      (!SDL_AUDIO_ISSIGNED(x))

/**
 *  \name Audio format flags
 *
 *  Defaults to LSB byte order.
 */
/* @{ */
#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB
/* @} */

/**
 *  \name int32 support
 */
/* @{ */
#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB
/* @} */

/**
 *  \name float32 support
 */
/* @{ */
#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB
/* @} */

/**
 *  \name Native audio byte ordering
 */
/* @{ */
#if 1
#define AUDIO_U16SYS    AUDIO_U16LSB
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_S32SYS    AUDIO_S32LSB
#define AUDIO_F32SYS    AUDIO_F32LSB
#else
#define AUDIO_U16SYS    AUDIO_U16MSB
#define AUDIO_S16SYS    AUDIO_S16MSB
#define AUDIO_S32SYS    AUDIO_S32MSB
#define AUDIO_F32SYS    AUDIO_F32MSB
#endif
/* @} */


struct SDL_AudioCVT_EX;
typedef void  (*SDL_AudioFilter_EX) (struct SDL_AudioCVT_EX * cvt,
                                          SDL_AudioFormat format);

/**
 *  \brief Upper limit of filters in SDL_AudioCVT_EX
 *
 *  The maximum number of SDL_AudioFilter_EX functions in SDL_AudioCVT_EX is
 *  currently limited to 9. The SDL_AudioCVT_EX.filters array has 10 pointers,
 *  one of which is the terminating NULL pointer.
 */
#define SDL_AUDIOCVT_MAX_FILTERS 9

/**
 *  \struct SDL_AudioCVT_EX
 *  \brief A structure to hold a set of audio conversion filters and buffers.
 *
 *  Note that various parts of the conversion pipeline can take advantage
 *  of SIMD operations (like SSE2, for example). SDL_AudioCVT_EX doesn't require
 *  you to pass it aligned data, but can possibly run much faster if you
 *  set both its (buf) field to a pointer that is aligned to 16 bytes, and its
 *  (len) field to something that's a multiple of 16, if possible.
 */
#ifdef __GNUC__
/* This structure is 84 bytes on 32-bit architectures, make sure GCC doesn't
   pad it out to 88 bytes to guarantee ABI compatibility between compilers.
   vvv
   The next time we rev the ABI, make sure to size the ints and add padding.
*/
#define SDL_AUDIOCVT_PACKED __attribute__((packed))
#else
#define SDL_AUDIOCVT_PACKED
#endif
/* */
typedef struct SDL_AudioCVT_EX
{
    int needed;                 /**< Set to 1 if conversion possible */
    SDL_AudioFormat src_format; /**< Source audio format */
    SDL_AudioFormat dst_format; /**< Target audio format */
    double rate_incr;           /**< Rate conversion increment */
    uint8_t *buf;                 /**< Buffer to hold entire audio data */
    int len;                    /**< Length of original audio buffer */
    int len_cvt;                /**< Length of converted audio buffer */
    int len_mult;               /**< buffer must be len*len_mult big */
    double len_ratio;           /**< Given len, final size is len*len_ratio */
    SDL_AudioFilter_EX filters[SDL_AUDIOCVT_MAX_FILTERS + 1]; /**< NULL-terminated list of filter functions */
    int filter_index;           /**< Current audio conversion function */
} SDL_AUDIOCVT_PACKED SDL_AudioCVT_EX;


/* Function prototypes */

/**
 *  This function takes a source format and rate and a destination format
 *  and rate, and initializes the \c cvt structure with information needed
 *  by SDL_ConvertAudio_EX() to convert a buffer of audio data from one format
 *  to the other. An unsupported format causes an error and -1 will be returned.
 *
 *  \return 0 if no conversion is needed, 1 if the audio filter is set up,
 *  or -1 on error.
 */
int SDL_BuildAudioCVT_EX(SDL_AudioCVT_EX * cvt,
                                              SDL_AudioFormat src_format,
                                              uint8_t src_channels,
                                              int src_rate,
                                              SDL_AudioFormat dst_format,
                                              uint8_t dst_channels,
                                              int dst_rate);

/**
 *  Once you have initialized the \c cvt structure using SDL_BuildAudioCVT_EX(),
 *  created an audio buffer \c cvt->buf, and filled it with \c cvt->len bytes of
 *  audio data in the source format, this function will convert it in-place
 *  to the desired format.
 *
 *  The data conversion may expand the size of the audio data, so the buffer
 *  \c cvt->buf should be allocated after the \c cvt structure is initialized by
 *  SDL_BuildAudioCVT_EX(), and should be \c cvt->len*cvt->len_mult bytes long.
 *
 *  \return 0 on success or -1 if \c cvt->buf is NULL.
 */
int SDL_ConvertAudio_EX(SDL_AudioCVT_EX * cvt);

/* SDL_AudioStream is a new audio conversion interface.
   The benefits vs SDL_AudioCVT_EX:
    - it can handle resampling data in chunks without generating
      artifacts, when it doesn't have the complete buffer available.
    - it can handle incoming data in any variable size.
    - You push data as you have it, and pull it when you need it
 */
/* this is opaque to the outside world. */
struct _SDL_AudioStream;
typedef struct _SDL_AudioStream SDL_AudioStream;

/**
 *  Create a new audio stream
 *
 *  \param src_format The format of the source audio
 *  \param src_channels The number of channels of the source audio
 *  \param src_rate The sampling rate of the source audio
 *  \param dst_format The format of the desired audio output
 *  \param dst_channels The number of channels of the desired audio output
 *  \param dst_rate The sampling rate of the desired audio output
 *  \return 0 on success, or -1 on error.
 *
 *  \sa SDL_AudioStreamPutEX
 *  \sa SDL_AudioStreamGetEX
 *  \sa SDL_AudioStreamAvailableEX
 *  \sa SDL_AudioStreamFlushEX
 *  \sa SDL_AudioStreamClearEX
 *  \sa SDL_FreeAudioStreamEX
 */
SDL_AudioStream * SDL_NewAudioStreamEX(const SDL_AudioFormat src_format,
                                           const uint8_t src_channels,
                                           const int src_rate,
                                           const SDL_AudioFormat dst_format,
                                           const uint8_t dst_channels,
                                           const int dst_rate);

/**
 *  Add data to be converted/resampled to the stream
 *
 *  \param stream The stream the audio data is being added to
 *  \param buf A pointer to the audio data to add
 *  \param len The number of bytes to write to the stream
 *  \return 0 on success, or -1 on error.
 *
 *  \sa SDL_NewAudioStreamEX
 *  \sa SDL_AudioStreamGetEX
 *  \sa SDL_AudioStreamAvailableEX
 *  \sa SDL_AudioStreamFlushEX
 *  \sa SDL_AudioStreamClearEX
 *  \sa SDL_FreeAudioStreamEX
 */
int SDL_AudioStreamPutEX(SDL_AudioStream *stream, const void *buf, int len);

/**
 *  Get converted/resampled data from the stream
 *
 *  \param stream The stream the audio is being requested from
 *  \param buf A buffer to fill with audio data
 *  \param len The maximum number of bytes to fill
 *  \return The number of bytes read from the stream, or -1 on error
 *
 *  \sa SDL_NewAudioStreamEX
 *  \sa SDL_AudioStreamPutEX
 *  \sa SDL_AudioStreamAvailableEX
 *  \sa SDL_AudioStreamFlushEX
 *  \sa SDL_AudioStreamClearEX
 *  \sa SDL_FreeAudioStreamEX
 */
int SDL_AudioStreamGetEX(SDL_AudioStream *stream, void *buf, int len);

/**
 * Get the number of converted/resampled bytes available. The stream may be
 *  buffering data behind the scenes until it has enough to resample
 *  correctly, so this number might be lower than what you expect, or even
 *  be zero. Add more data or flush the stream if you need the data now.
 *
 *  \sa SDL_NewAudioStreamEX
 *  \sa SDL_AudioStreamPutEX
 *  \sa SDL_AudioStreamGetEX
 *  \sa SDL_AudioStreamFlushEX
 *  \sa SDL_AudioStreamClearEX
 *  \sa SDL_FreeAudioStreamEX
 */
int SDL_AudioStreamAvailableEX(SDL_AudioStream *stream);

/**
 * Tell the stream that you're done sending data, and anything being buffered
 *  should be converted/resampled and made available immediately.
 *
 * It is legal to add more data to a stream after flushing, but there will
 *  be audio gaps in the output. Generally this is intended to signal the
 *  end of input, so the complete output becomes available.
 *
 *  \sa SDL_NewAudioStreamEX
 *  \sa SDL_AudioStreamPutEX
 *  \sa SDL_AudioStreamGetEX
 *  \sa SDL_AudioStreamAvailableEX
 *  \sa SDL_AudioStreamClearEX
 *  \sa SDL_FreeAudioStreamEX
 */
int SDL_AudioStreamFlushEX(SDL_AudioStream *stream);

/**
 *  Clear any pending data in the stream without converting it
 *
 *  \sa SDL_NewAudioStreamEX
 *  \sa SDL_AudioStreamPutEX
 *  \sa SDL_AudioStreamGetEX
 *  \sa SDL_AudioStreamAvailableEX
 *  \sa SDL_AudioStreamFlushEX
 *  \sa SDL_FreeAudioStreamEX
 */
void SDL_AudioStreamClearEX(SDL_AudioStream *stream);

/**
 * Free an audio stream
 *
 *  \sa SDL_NewAudioStreamEX
 *  \sa SDL_AudioStreamPutEX
 *  \sa SDL_AudioStreamGetEX
 *  \sa SDL_AudioStreamAvailableEX
 *  \sa SDL_AudioStreamFlushEX
 *  \sa SDL_AudioStreamClearEX
 */
void SDL_FreeAudioStreamEX(SDL_AudioStream *stream);

#define SDL_MIX_MAXVOLUME 128

// /* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* SDL_audio_EX_h_ */
