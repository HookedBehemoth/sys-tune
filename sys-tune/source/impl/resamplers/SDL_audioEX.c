#include "SDL_audioEX.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288   /**< pi */
#endif

#define SDL_zeropEX(x) memset((x), 0, sizeof(*(x)))
#define SDL_zeroaEX(x) memset((x), 0, sizeof((x)))
#define SDL_minEX(a,b) ((a) < (b) ? (a) : (b))

#define DEBUG_AUDIOSTREAM 0

static int SDL_OutOfMemoryEX() { return -1; }
#ifndef NDEBUG
#include <stdio.h>
static int SDL_PrintError(const char* e) { printf(e);  return -1; }
#else
static int SDL_PrintError(const char* e) { (void)e;  return -1; }
#endif

// BEGIN AUDIO_c.h
#ifndef DEBUG_CONVERT
#define DEBUG_CONVERT 0
#endif

#if DEBUG_CONVERT
#define LOG_DEBUG_CONVERT(from, to) fprintf(stderr, "Converting %s to %s.\n", from, to);
#else
#define LOG_DEBUG_CONVERT(from, to)
#endif

/* Functions and variables exported from SDL_audio.c for SDL_sysaudio.c */

/* Choose the audio filter functions below */
void SDL_ChooseAudioConverters(void);

/* You need to call SDL_PrepareResampleFilter() before using the internal resampler. */
static int SDL_PrepareResampleFilter(void);

// BEGIN AUDIOTYPECVT
#ifdef __ARM_NEON
#include <arm_neon.h>
#define HAVE_NEON_INTRINSICS 1
#endif

#ifdef __SSE2__
#include <immintrin.h>
#define HAVE_SSE2_INTRINSICS 1
#endif

#if defined(__x86_64__) && HAVE_SSE2_INTRINSICS
#define NEED_SCALAR_CONVERTER_FALLBACKS 0  /* x86_64 guarantees SSE2. */
#elif __MACOSX__ && HAVE_SSE2_INTRINSICS
#define NEED_SCALAR_CONVERTER_FALLBACKS 0  /* Mac OS X/Intel guarantees SSE2. */
#elif defined(__ARM_ARCH) && (__ARM_ARCH >= 8) && HAVE_NEON_INTRINSICS
#define NEED_SCALAR_CONVERTER_FALLBACKS 0  /* ARMv8+ promise NEON. */
#elif defined(__APPLE__) && defined(__ARM_ARCH) && (__ARM_ARCH >= 7) && HAVE_NEON_INTRINSICS
#define NEED_SCALAR_CONVERTER_FALLBACKS 0  /* All Apple ARMv7 chips promise NEON support. */
#endif

/* Set to zero if platform is guaranteed to use a SIMD codepath here. */
#ifndef NEED_SCALAR_CONVERTER_FALLBACKS
#define NEED_SCALAR_CONVERTER_FALLBACKS 1
#endif

/* Function pointers set to a CPU-specific implementation. */
SDL_AudioFilter_EX SDL_Convert_S8_to_F32 = NULL;
SDL_AudioFilter_EX SDL_Convert_U8_to_F32 = NULL;
SDL_AudioFilter_EX SDL_Convert_S16_to_F32 = NULL;
SDL_AudioFilter_EX SDL_Convert_U16_to_F32 = NULL;
SDL_AudioFilter_EX SDL_Convert_S32_to_F32 = NULL;
SDL_AudioFilter_EX SDL_Convert_F32_to_S8 = NULL;
SDL_AudioFilter_EX SDL_Convert_F32_to_U8 = NULL;
SDL_AudioFilter_EX SDL_Convert_F32_to_S16 = NULL;
SDL_AudioFilter_EX SDL_Convert_F32_to_U16 = NULL;
SDL_AudioFilter_EX SDL_Convert_F32_to_S32 = NULL;


#define DIVBY128 0.0078125f
#define DIVBY32768 0.000030517578125f
#define DIVBY8388607 0.00000011920930376163766f


#if NEED_SCALAR_CONVERTER_FALLBACKS
static void
SDL_Convert_S8_to_F32_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const int8_t *src = ((const int8_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 4)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S8", "AUDIO_F32");

    for (i = cvt->len_cvt; i; --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY128;
    }

    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_U8_to_F32_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const uint8_t *src = ((const uint8_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 4)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_U8", "AUDIO_F32");

    for (i = cvt->len_cvt; i; --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
    }

    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_S16_to_F32_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const int16_t *src = ((const int16_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 2)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S16", "AUDIO_F32");

    for (i = cvt->len_cvt / sizeof (int16_t); i; --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY32768;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_U16_to_F32_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;

    const uint16_t *src = ((const uint16_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 2)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_U16", "AUDIO_F32");

    for (i = cvt->len_cvt / sizeof (uint16_t); i; --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY32768) - 1.0f;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_S32_to_F32_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const int32_t *src = (const int32_t *) cvt->buf;
    float *dst = (float *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S32", "AUDIO_F32");

    for (i = cvt->len_cvt / sizeof (int32_t); i; --i, ++src, ++dst) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
    }

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_F32_to_S8_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const float *src = (const float *) cvt->buf;
    int8_t *dst = (int8_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S8");

    for (i = cvt->len_cvt / sizeof (float); i; --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 127;
        } else if (sample <= -1.0f) {
            *dst = -128;
        } else {
            *dst = (int8_t)(sample * 127.0f);
        }
    }

    cvt->len_cvt /= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S8);
    }
}

static void
SDL_Convert_F32_to_U8_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const float *src = (const float *) cvt->buf;
    uint8_t *dst = (uint8_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_U8");

    for (i = cvt->len_cvt / sizeof (float); i; --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 255;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint8_t)((sample + 1.0f) * 127.0f);
        }
    }

    cvt->len_cvt /= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_U8);
    }
}

static void
SDL_Convert_F32_to_S16_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const float *src = (const float *) cvt->buf;
    int16_t *dst = (int16_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S16");

    for (i = cvt->len_cvt / sizeof (float); i; --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 32767;
        } else if (sample <= -1.0f) {
            *dst = -32768;
        } else {
            *dst = (int16_t)(sample * 32767.0f);
        }
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S16SYS);
    }
}

static void
SDL_Convert_F32_to_U16_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const float *src = (const float *) cvt->buf;
    uint16_t *dst = (uint16_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_U16");

    for (i = cvt->len_cvt / sizeof (float); i; --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 65535;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint16_t)((sample + 1.0f) * 32767.0f);
        }
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_U16SYS);
    }
}

static void
SDL_Convert_F32_to_S32_Scalar(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    (void)format;
    const float *src = (const float *) cvt->buf;
    int32_t *dst = (int32_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S32");

    for (i = cvt->len_cvt / sizeof (float); i; --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 2147483647;
        } else if (sample <= -1.0f) {
            *dst = (int32_t) -2147483648LL;
        } else {
            *dst = ((int32_t)(sample * 8388607.0f)) << 8;
        }
    }

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S32SYS);
    }
}
#endif


#if HAVE_SSE2_INTRINSICS
static void
SDL_Convert_S8_to_F32_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const int8_t *src = ((const int8_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 4)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S8", "AUDIO_F32 (using SSE2)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt; i && (((size_t) (dst-15)) & 15); --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY128;
    }

    src -= 15; dst -= 15;  /* adjust to read SSE blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128i *mmsrc = (const __m128i *) src;
        const __m128i zero = _mm_setzero_si128();
        const __m128 divby128 = _mm_set1_ps(DIVBY128);
        while (i >= 16) {   /* 16 * 8-bit */
            const __m128i bytes = _mm_load_si128(mmsrc);  /* get 16 sint8 into an XMM register. */
            /* treat as int16, shift left to clear every other sint16, then back right with sign-extend. Now sint16. */
            const __m128i shorts1 = _mm_srai_epi16(_mm_slli_epi16(bytes, 8), 8);
            /* right-shift-sign-extend gets us sint16 with the other set of values. */
            const __m128i shorts2 = _mm_srai_epi16(bytes, 8);
            /* unpack against zero to make these int32, shift to make them sign-extend, convert to float, multiply. Whew! */
            const __m128 floats1 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srai_epi32(_mm_slli_epi32(_mm_unpacklo_epi16(shorts1, zero), 16), 16)), divby128);
            const __m128 floats2 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srai_epi32(_mm_slli_epi32(_mm_unpacklo_epi16(shorts2, zero), 16), 16)), divby128);
            const __m128 floats3 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srai_epi32(_mm_slli_epi32(_mm_unpackhi_epi16(shorts1, zero), 16), 16)), divby128);
            const __m128 floats4 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_srai_epi32(_mm_slli_epi32(_mm_unpackhi_epi16(shorts2, zero), 16), 16)), divby128);
            /* Interleave back into correct order, store. */
            _mm_store_ps(dst, _mm_unpacklo_ps(floats1, floats2));
            _mm_store_ps(dst+4, _mm_unpackhi_ps(floats1, floats2));
            _mm_store_ps(dst+8, _mm_unpacklo_ps(floats3, floats4));
            _mm_store_ps(dst+12, _mm_unpackhi_ps(floats3, floats4));
            i -= 16; mmsrc--; dst -= 16;
        }

        src = (const int8_t *) mmsrc;
    }

    src += 15; dst += 15;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) *src) * DIVBY128;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_U8_to_F32_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const uint8_t *src = ((const uint8_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 4)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_U8", "AUDIO_F32 (using SSE2)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt; i && (((size_t) (dst-15)) & 15); --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
    }

    src -= 15; dst -= 15;  /* adjust to read SSE blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128i *mmsrc = (const __m128i *) src;
        const __m128i zero = _mm_setzero_si128();
        const __m128 divby128 = _mm_set1_ps(DIVBY128);
        const __m128 minus1 = _mm_set1_ps(-1.0f);
        while (i >= 16) {   /* 16 * 8-bit */
            const __m128i bytes = _mm_load_si128(mmsrc);  /* get 16 uint8 into an XMM register. */
            /* treat as int16, shift left to clear every other sint16, then back right with zero-extend. Now uint16. */
            const __m128i shorts1 = _mm_srli_epi16(_mm_slli_epi16(bytes, 8), 8);
            /* right-shift-zero-extend gets us uint16 with the other set of values. */
            const __m128i shorts2 = _mm_srli_epi16(bytes, 8);
            /* unpack against zero to make these int32, convert to float, multiply, add. Whew! */
            /* Note that AVX2 can do floating point multiply+add in one instruction, fwiw. SSE2 cannot. */
            const __m128 floats1 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts1, zero)), divby128), minus1);
            const __m128 floats2 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts2, zero)), divby128), minus1);
            const __m128 floats3 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(shorts1, zero)), divby128), minus1);
            const __m128 floats4 = _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi16(shorts2, zero)), divby128), minus1);
            /* Interleave back into correct order, store. */
            _mm_store_ps(dst, _mm_unpacklo_ps(floats1, floats2));
            _mm_store_ps(dst+4, _mm_unpackhi_ps(floats1, floats2));
            _mm_store_ps(dst+8, _mm_unpacklo_ps(floats3, floats4));
            _mm_store_ps(dst+12, _mm_unpackhi_ps(floats3, floats4));
            i -= 16; mmsrc--; dst -= 16;
        }

        src = (const uint8_t *) mmsrc;
    }

    src += 15; dst += 15;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_S16_to_F32_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const int16_t *src = ((const int16_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 2)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S16", "AUDIO_F32 (using SSE2)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt / sizeof (int16_t); i && (((size_t) (dst-7)) & 15); --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY32768;
    }

    src -= 7; dst -= 7;  /* adjust to read SSE blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 divby32768 = _mm_set1_ps(DIVBY32768);
        while (i >= 8) {   /* 8 * 16-bit */
            const __m128i ints = _mm_load_si128((__m128i const *) src);  /* get 8 sint16 into an XMM register. */
            /* treat as int32, shift left to clear every other sint16, then back right with sign-extend. Now sint32. */
            const __m128i a = _mm_srai_epi32(_mm_slli_epi32(ints, 16), 16);
            /* right-shift-sign-extend gets us sint32 with the other set of values. */
            const __m128i b = _mm_srai_epi32(ints, 16);
            /* Interleave these back into the right order, convert to float, multiply, store. */
            _mm_store_ps(dst, _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi32(a, b)), divby32768));
            _mm_store_ps(dst+4, _mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi32(a, b)), divby32768));
            i -= 8; src -= 8; dst -= 8;
        }
    }

    src += 7; dst += 7;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) *src) * DIVBY32768;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_U16_to_F32_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const uint16_t *src = ((const uint16_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 2)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_U16", "AUDIO_F32 (using SSE2)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt / sizeof (int16_t); i && (((size_t) (dst-7)) & 15); --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY32768) - 1.0f;
    }

    src -= 7; dst -= 7;  /* adjust to read SSE blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 divby32768 = _mm_set1_ps(DIVBY32768);
        const __m128 minus1 = _mm_set1_ps(-1.0f);
        while (i >= 8) {   /* 8 * 16-bit */
            const __m128i ints = _mm_load_si128((__m128i const *) src);  /* get 8 sint16 into an XMM register. */
            /* treat as int32, shift left to clear every other sint16, then back right with zero-extend. Now sint32. */
            const __m128i a = _mm_srli_epi32(_mm_slli_epi32(ints, 16), 16);
            /* right-shift-sign-extend gets us sint32 with the other set of values. */
            const __m128i b = _mm_srli_epi32(ints, 16);
            /* Interleave these back into the right order, convert to float, multiply, store. */
            _mm_store_ps(dst, _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpacklo_epi32(a, b)), divby32768), minus1));
            _mm_store_ps(dst+4, _mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_unpackhi_epi32(a, b)), divby32768), minus1));
            i -= 8; src -= 8; dst -= 8;
        }
    }

    src += 7; dst += 7;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = (((float) *src) * DIVBY32768) - 1.0f;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_S32_to_F32_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const int32_t *src = (const int32_t *) cvt->buf;
    float *dst = (float *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S32", "AUDIO_F32 (using SSE2)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (int32_t); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 divby8388607 = _mm_set1_ps(DIVBY8388607);
        const __m128i *mmsrc = (const __m128i *) src;
        while (i >= 4) {   /* 4 * sint32 */
            /* shift out lowest bits so int fits in a float32. Small precision loss, but much faster. */
            _mm_store_ps(dst, _mm_mul_ps(_mm_cvtepi32_ps(_mm_srai_epi32(_mm_load_si128(mmsrc), 8)), divby8388607));
            i -= 4; mmsrc++; dst += 4;
        }
        src = (const int32_t *) mmsrc;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
        i--; src++; dst++;
    }

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_F32_to_S8_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    int8_t *dst = (int8_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S8 (using SSE2)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 127;
        } else if (sample <= -1.0f) {
            *dst = -128;
        } else {
            *dst = (int8_t)(sample * 127.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 negone = _mm_set1_ps(-1.0f);
        const __m128 mulby127 = _mm_set1_ps(127.0f);
        __m128i *mmdst = (__m128i *) dst;
        while (i >= 16) {   /* 16 * float32 */
            const __m128i ints1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+4)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints3 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+8)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints4 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+12)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            _mm_store_si128(mmdst, _mm_packs_epi16(_mm_packs_epi32(ints1, ints2), _mm_packs_epi32(ints3, ints4)));  /* pack down, store out. */
            i -= 16; src += 16; mmdst++;
        }
        dst = (int8_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 127;
        } else if (sample <= -1.0f) {
            *dst = -128;
        } else {
            *dst = (int8_t)(sample * 127.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S8);
    }
}

static void
SDL_Convert_F32_to_U8_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    uint8_t *dst = cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_U8 (using SSE2)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 255;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint8_t)((sample + 1.0f) * 127.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 negone = _mm_set1_ps(-1.0f);
        const __m128 mulby127 = _mm_set1_ps(127.0f);
        __m128i *mmdst = (__m128i *) dst;
        while (i >= 16) {   /* 16 * float32 */
            const __m128i ints1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src)), one), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+4)), one), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints3 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+8)), one), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints4 = _mm_cvtps_epi32(_mm_mul_ps(_mm_add_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+12)), one), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            _mm_store_si128(mmdst, _mm_packus_epi16(_mm_packs_epi32(ints1, ints2), _mm_packs_epi32(ints3, ints4)));  /* pack down, store out. */
            i -= 16; src += 16; mmdst++;
        }
        dst = (uint8_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 255;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint8_t)((sample + 1.0f) * 127.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_U8);
    }
}

static void
SDL_Convert_F32_to_S16_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    int16_t *dst = (int16_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S16 (using SSE2)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 32767;
        } else if (sample <= -1.0f) {
            *dst = -32768;
        } else {
            *dst = (int16_t)(sample * 32767.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 negone = _mm_set1_ps(-1.0f);
        const __m128 mulby32767 = _mm_set1_ps(32767.0f);
        __m128i *mmdst = (__m128i *) dst;
        while (i >= 8) {   /* 8 * float32 */
            const __m128i ints1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src)), one), mulby32767));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+4)), one), mulby32767));  /* load 4 floats, clamp, convert to sint32 */
            _mm_store_si128(mmdst, _mm_packs_epi32(ints1, ints2));  /* pack to sint16, store out. */
            i -= 8; src += 8; mmdst++;
        }
        dst = (int16_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 32767;
        } else if (sample <= -1.0f) {
            *dst = -32768;
        } else {
            *dst = (int16_t)(sample * 32767.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S16SYS);
    }
}

static void
SDL_Convert_F32_to_U16_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    uint16_t *dst = (uint16_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_U16 (using SSE2)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 65535;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint16_t)((sample + 1.0f) * 32767.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        /* This calculates differently than the scalar path because SSE2 can't
           pack int32 data down to unsigned int16. _mm_packs_epi32 does signed
           saturation, so that would corrupt our data. _mm_packus_epi32 exists,
           but not before SSE 4.1. So we convert from float to sint16, packing
           that down with legit signed saturation, and then xor the top bit
           against 1. This results in the correct unsigned 16-bit value, even
           though it looks like dark magic. */
        const __m128 mulby32767 = _mm_set1_ps(32767.0f);
        const __m128i topbit = _mm_set1_epi16(-32768);
        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 negone = _mm_set1_ps(-1.0f);
        __m128i *mmdst = (__m128i *) dst;
        while (i >= 8) {   /* 8 * float32 */
            const __m128i ints1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src)), one), mulby32767));  /* load 4 floats, clamp, convert to sint32 */
            const __m128i ints2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src+4)), one), mulby32767));  /* load 4 floats, clamp, convert to sint32 */
            _mm_store_si128(mmdst, _mm_xor_si128(_mm_packs_epi32(ints1, ints2), topbit));  /* pack to sint16, xor top bit, store out. */
            i -= 8; src += 8; mmdst++;
        }
        dst = (uint16_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 65535;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint16_t)((sample + 1.0f) * 32767.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_U16SYS);
    }
}

static void
SDL_Convert_F32_to_S32_SSE2(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    int32_t *dst = (int32_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S32 (using SSE2)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 2147483647;
        } else if (sample <= -1.0f) {
            *dst = (int32_t) -2147483648LL;
        } else {
            *dst = ((int32_t)(sample * 8388607.0f)) << 8;
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));
    assert(!i || ((((size_t) src) & 15) == 0));

    {
        /* Aligned! Do SSE blocks as long as we have 16 bytes available. */
        const __m128 one = _mm_set1_ps(1.0f);
        const __m128 negone = _mm_set1_ps(-1.0f);
        const __m128 mulby8388607 = _mm_set1_ps(8388607.0f);
        __m128i *mmdst = (__m128i *) dst;
        while (i >= 4) {   /* 4 * float32 */
            _mm_store_si128(mmdst, _mm_slli_epi32(_mm_cvtps_epi32(_mm_mul_ps(_mm_min_ps(_mm_max_ps(negone, _mm_load_ps(src)), one), mulby8388607)), 8));  /* load 4 floats, clamp, convert to sint32 */
            i -= 4; src += 4; mmdst++;
        }
        dst = (int32_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 2147483647;
        } else if (sample <= -1.0f) {
            *dst = (int32_t) -2147483648LL;
        } else {
            *dst = ((int32_t)(sample * 8388607.0f)) << 8;
        }
        i--; src++; dst++;
    }

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S32SYS);
    }
}
#endif


#if HAVE_NEON_INTRINSICS
static void
SDL_Convert_S8_to_F32_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const int8_t *src = ((const int8_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 4)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S8", "AUDIO_F32 (using NEON)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt; i && (((size_t) (dst-15)) & 15); --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY128;
    }

    src -= 15; dst -= 15;  /* adjust to read NEON blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const int8_t *mmsrc = (const int8_t *) src;
        const float32x4_t divby128 = vdupq_n_f32(DIVBY128);
        while (i >= 16) {   /* 16 * 8-bit */
            const int8x16_t bytes = vld1q_s8(mmsrc);  /* get 16 sint8 into a NEON register. */
            const int16x8_t int16hi = vmovl_s8(vget_high_s8(bytes));  /* convert top 8 bytes to 8 int16 */
            const int16x8_t int16lo = vmovl_s8(vget_low_s8(bytes));   /* convert bottom 8 bytes to 8 int16 */
            /* split int16 to two int32, then convert to float, then multiply to normalize, store. */
            vst1q_f32(dst, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_low_s16(int16lo))), divby128));
            vst1q_f32(dst+4, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_high_s16(int16lo))), divby128));
            vst1q_f32(dst+8, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_low_s16(int16hi))), divby128));
            vst1q_f32(dst+12, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_high_s16(int16hi))), divby128));
            i -= 16; mmsrc -= 16; dst -= 16;
        }

        src = (const int8_t *) mmsrc;
    }

    src += 15; dst += 15;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) *src) * DIVBY128;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_U8_to_F32_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const uint8_t *src = ((const uint8_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 4)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_U8", "AUDIO_F32 (using NEON)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt; i && (((size_t) (dst-15)) & 15); --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
    }

    src -= 15; dst -= 15;  /* adjust to read NEON blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const uint8_t *mmsrc = (const uint8_t *) src;
        const float32x4_t divby128 = vdupq_n_f32(DIVBY128);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        while (i >= 16) {   /* 16 * 8-bit */
            const uint8x16_t bytes = vld1q_u8(mmsrc);  /* get 16 uint8 into a NEON register. */
            const uint16x8_t uint16hi = vmovl_u8(vget_high_u8(bytes));  /* convert top 8 bytes to 8 uint16 */
            const uint16x8_t uint16lo = vmovl_u8(vget_low_u8(bytes));   /* convert bottom 8 bytes to 8 uint16 */
            /* split uint16 to two uint32, then convert to float, then multiply to normalize, subtract to adjust for sign, store. */
            vst1q_f32(dst, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_low_u16(uint16lo))), divby128));
            vst1q_f32(dst+4, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_high_u16(uint16lo))), divby128));
            vst1q_f32(dst+8, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_low_u16(uint16hi))), divby128));
            vst1q_f32(dst+12, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_high_u16(uint16hi))), divby128));
            i -= 16; mmsrc -= 16; dst -= 16;
        }

        src = (const uint8_t *) mmsrc;
    }

    src += 15; dst += 15;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = (((float) *src) * DIVBY128) - 1.0f;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_S16_to_F32_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const int16_t *src = ((const int16_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 2)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S16", "AUDIO_F32 (using NEON)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt / sizeof (int16_t); i && (((size_t) (dst-7)) & 15); --i, --src, --dst) {
        *dst = ((float) *src) * DIVBY32768;
    }

    src -= 7; dst -= 7;  /* adjust to read NEON blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t divby32768 = vdupq_n_f32(DIVBY32768);
        while (i >= 8) {   /* 8 * 16-bit */
            const int16x8_t ints = vld1q_s16((int16_t const *) src);  /* get 8 sint16 into a NEON register. */
            /* split int16 to two int32, then convert to float, then multiply to normalize, store. */
            vst1q_f32(dst, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_low_s16(ints))), divby32768));
            vst1q_f32(dst+4, vmulq_f32(vcvtq_f32_s32(vmovl_s16(vget_high_s16(ints))), divby32768));
            i -= 8; src -= 8; dst -= 8;
        }
    }

    src += 7; dst += 7;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) *src) * DIVBY32768;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_U16_to_F32_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const uint16_t *src = ((const uint16_t *) (cvt->buf + cvt->len_cvt)) - 1;
    float *dst = ((float *) (cvt->buf + cvt->len_cvt * 2)) - 1;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_U16", "AUDIO_F32 (using NEON)");

    /* Get dst aligned to 16 bytes (since buffer is growing, we don't have to worry about overreading from src) */
    for (i = cvt->len_cvt / sizeof (int16_t); i && (((size_t) (dst-7)) & 15); --i, --src, --dst) {
        *dst = (((float) *src) * DIVBY32768) - 1.0f;
    }

    src -= 7; dst -= 7;  /* adjust to read NEON blocks from the start. */
    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t divby32768 = vdupq_n_f32(DIVBY32768);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        while (i >= 8) {   /* 8 * 16-bit */
            const uint16x8_t uints = vld1q_u16((uint16_t const *) src);  /* get 8 uint16 into a NEON register. */
            /* split uint16 to two int32, then convert to float, then multiply to normalize, subtract for sign, store. */
            vst1q_f32(dst, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_low_u16(uints))), divby32768));
            vst1q_f32(dst+4, vmlaq_f32(negone, vcvtq_f32_u32(vmovl_u16(vget_high_u16(uints))), divby32768));
            i -= 8; src -= 8; dst -= 8;
        }
    }

    src += 7; dst += 7;  /* adjust for any scalar finishing. */

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = (((float) *src) * DIVBY32768) - 1.0f;
        i--; src--; dst--;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_S32_to_F32_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const int32_t *src = (const int32_t *) cvt->buf;
    float *dst = (float *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_S32", "AUDIO_F32 (using NEON)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (int32_t); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t divby8388607 = vdupq_n_f32(DIVBY8388607);
        const int32_t *mmsrc = (const int32_t *) src;
        while (i >= 4) {   /* 4 * sint32 */
            /* shift out lowest bits so int fits in a float32. Small precision loss, but much faster. */
            vst1q_f32(dst, vmulq_f32(vcvtq_f32_s32(vshrq_n_s32(vld1q_s32(mmsrc), 8)), divby8388607));
            i -= 4; mmsrc += 4; dst += 4;
        }
        src = (const int32_t *) mmsrc;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        *dst = ((float) (*src>>8)) * DIVBY8388607;
        i--; src++; dst++;
    }

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_F32SYS);
    }
}

static void
SDL_Convert_F32_to_S8_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    int8_t *dst = (int8_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S8 (using NEON)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 127;
        } else if (sample <= -1.0f) {
            *dst = -128;
        } else {
            *dst = (int8_t)(sample * 127.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t one = vdupq_n_f32(1.0f);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        const float32x4_t mulby127 = vdupq_n_f32(127.0f);
        int8_t *mmdst = (int8_t *) dst;
        while (i >= 16) {   /* 16 * float32 */
            const int32x4_t ints1 = vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const int32x4_t ints2 = vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+4)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const int32x4_t ints3 = vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+8)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const int32x4_t ints4 = vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+12)), one), mulby127));  /* load 4 floats, clamp, convert to sint32 */
            const int8x8_t i8lo = vmovn_s16(vcombine_s16(vmovn_s32(ints1), vmovn_s32(ints2))); /* narrow to sint16, combine, narrow to sint8 */
            const int8x8_t i8hi = vmovn_s16(vcombine_s16(vmovn_s32(ints3), vmovn_s32(ints4))); /* narrow to sint16, combine, narrow to sint8 */
            vst1q_s8(mmdst, vcombine_s8(i8lo, i8hi));  /* combine to int8x16_t, store out */
            i -= 16; src += 16; mmdst += 16;
        }
        dst = (int8_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 127;
        } else if (sample <= -1.0f) {
            *dst = -128;
        } else {
            *dst = (int8_t)(sample * 127.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S8);
    }
}

static void
SDL_Convert_F32_to_U8_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    uint8_t *dst = (uint8_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_U8 (using NEON)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 255;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint8_t)((sample + 1.0f) * 127.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t one = vdupq_n_f32(1.0f);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        const float32x4_t mulby127 = vdupq_n_f32(127.0f);
        uint8_t *mmdst = (uint8_t *) dst;
        while (i >= 16) {   /* 16 * float32 */
            const uint32x4_t uints1 = vcvtq_u32_f32(vmulq_f32(vaddq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src)), one), one), mulby127));  /* load 4 floats, clamp, convert to uint32 */
            const uint32x4_t uints2 = vcvtq_u32_f32(vmulq_f32(vaddq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+4)), one), one), mulby127));  /* load 4 floats, clamp, convert to uint32 */
            const uint32x4_t uints3 = vcvtq_u32_f32(vmulq_f32(vaddq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+8)), one), one), mulby127));  /* load 4 floats, clamp, convert to uint32 */
            const uint32x4_t uints4 = vcvtq_u32_f32(vmulq_f32(vaddq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+12)), one), one), mulby127));  /* load 4 floats, clamp, convert to uint32 */
            const uint8x8_t ui8lo = vmovn_u16(vcombine_u16(vmovn_u32(uints1), vmovn_u32(uints2))); /* narrow to uint16, combine, narrow to uint8 */
            const uint8x8_t ui8hi = vmovn_u16(vcombine_u16(vmovn_u32(uints3), vmovn_u32(uints4))); /* narrow to uint16, combine, narrow to uint8 */
            vst1q_u8(mmdst, vcombine_u8(ui8lo, ui8hi));  /* combine to uint8x16_t, store out */
            i -= 16; src += 16; mmdst += 16;
        }

        dst = (uint8_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 255;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint8_t)((sample + 1.0f) * 127.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_U8);
    }
}

static void
SDL_Convert_F32_to_S16_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    int16_t *dst = (int16_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S16 (using NEON)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 32767;
        } else if (sample <= -1.0f) {
            *dst = -32768;
        } else {
            *dst = (int16_t)(sample * 32767.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t one = vdupq_n_f32(1.0f);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        const float32x4_t mulby32767 = vdupq_n_f32(32767.0f);
        int16_t *mmdst = (int16_t *) dst;
        while (i >= 8) {   /* 8 * float32 */
            const int32x4_t ints1 = vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src)), one), mulby32767));  /* load 4 floats, clamp, convert to sint32 */
            const int32x4_t ints2 = vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+4)), one), mulby32767));  /* load 4 floats, clamp, convert to sint32 */
            vst1q_s16(mmdst, vcombine_s16(vmovn_s32(ints1), vmovn_s32(ints2)));  /* narrow to sint16, combine, store out. */
            i -= 8; src += 8; mmdst += 8;
        }
        dst = (int16_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 32767;
        } else if (sample <= -1.0f) {
            *dst = -32768;
        } else {
            *dst = (int16_t)(sample * 32767.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S16SYS);
    }
}

static void
SDL_Convert_F32_to_U16_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    uint16_t *dst = (uint16_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_U16 (using NEON)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 65535;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint16_t)((sample + 1.0f) * 32767.0f);
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));

    /* Make sure src is aligned too. */
    if ((((size_t) src) & 15) == 0) {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t one = vdupq_n_f32(1.0f);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        const float32x4_t mulby32767 = vdupq_n_f32(32767.0f);
        uint16_t *mmdst = (uint16_t *) dst;
        while (i >= 8) {   /* 8 * float32 */
            const uint32x4_t uints1 = vcvtq_u32_f32(vmulq_f32(vaddq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src)), one), one), mulby32767));  /* load 4 floats, clamp, convert to uint32 */
            const uint32x4_t uints2 = vcvtq_u32_f32(vmulq_f32(vaddq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src+4)), one), one), mulby32767));  /* load 4 floats, clamp, convert to uint32 */
            vst1q_u16(mmdst, vcombine_u16(vmovn_u32(uints1), vmovn_u32(uints2)));  /* narrow to uint16, combine, store out. */
            i -= 8; src += 8; mmdst += 8;
        }
        dst = (uint16_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 65535;
        } else if (sample <= -1.0f) {
            *dst = 0;
        } else {
            *dst = (uint16_t)((sample + 1.0f) * 32767.0f);
        }
        i--; src++; dst++;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_U16SYS);
    }
}

static void
SDL_Convert_F32_to_S32_NEON(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) cvt->buf;
    int32_t *dst = (int32_t *) cvt->buf;
    int i;

    LOG_DEBUG_CONVERT("AUDIO_F32", "AUDIO_S32 (using NEON)");

    /* Get dst aligned to 16 bytes */
    for (i = cvt->len_cvt / sizeof (float); i && (((size_t) dst) & 15); --i, ++src, ++dst) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 2147483647;
        } else if (sample <= -1.0f) {
            *dst = (-2147483647) - 1;
        } else {
            *dst = ((int32_t)(sample * 8388607.0f)) << 8;
        }
    }

    assert(!i || ((((size_t) dst) & 15) == 0));
    assert(!i || ((((size_t) src) & 15) == 0));

    {
        /* Aligned! Do NEON blocks as long as we have 16 bytes available. */
        const float32x4_t one = vdupq_n_f32(1.0f);
        const float32x4_t negone = vdupq_n_f32(-1.0f);
        const float32x4_t mulby8388607 = vdupq_n_f32(8388607.0f);
        int32_t *mmdst = (int32_t *) dst;
        while (i >= 4) {   /* 4 * float32 */
            vst1q_s32(mmdst, vshlq_n_s32(vcvtq_s32_f32(vmulq_f32(vminq_f32(vmaxq_f32(negone, vld1q_f32(src)), one), mulby8388607)), 8));
            i -= 4; src += 4; mmdst += 4;
        }
        dst = (int32_t *) mmdst;
    }

    /* Finish off any leftovers with scalar operations. */
    while (i) {
        const float sample = *src;
        if (sample >= 1.0f) {
            *dst = 2147483647;
        } else if (sample <= -1.0f) {
            *dst = (-2147483647) - 1;
        } else {
            *dst = ((int32_t)(sample * 8388607.0f)) << 8;
        }
        i--; src++; dst++;
    }

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, AUDIO_S32SYS);
    }
}
#endif



void SDL_ChooseAudioConverters(void)
{
    static bool converters_chosen = false;

    if (converters_chosen) {
        return;
    }

#define SET_CONVERTER_FUNCS(fntype) \
        SDL_Convert_S8_to_F32 = SDL_Convert_S8_to_F32_##fntype; \
        SDL_Convert_U8_to_F32 = SDL_Convert_U8_to_F32_##fntype; \
        SDL_Convert_S16_to_F32 = SDL_Convert_S16_to_F32_##fntype; \
        SDL_Convert_U16_to_F32 = SDL_Convert_U16_to_F32_##fntype; \
        SDL_Convert_S32_to_F32 = SDL_Convert_S32_to_F32_##fntype; \
        SDL_Convert_F32_to_S8 = SDL_Convert_F32_to_S8_##fntype; \
        SDL_Convert_F32_to_U8 = SDL_Convert_F32_to_U8_##fntype; \
        SDL_Convert_F32_to_S16 = SDL_Convert_F32_to_S16_##fntype; \
        SDL_Convert_F32_to_U16 = SDL_Convert_F32_to_U16_##fntype; \
        SDL_Convert_F32_to_S32 = SDL_Convert_F32_to_S32_##fntype; \
        converters_chosen = true

#if HAVE_SSE2_INTRINSICS
    //if (SDL_HasSSE2()) {
        SET_CONVERTER_FUNCS(SSE2);
        return;
    //}
#endif

#if HAVE_NEON_INTRINSICS
    // if (SDL_HasNEON()) {
        SET_CONVERTER_FUNCS(NEON);
        return;
    // }
#endif

#if NEED_SCALAR_CONVERTER_FALLBACKS
    SET_CONVERTER_FUNCS(Scalar);
#endif

#undef SET_CONVERTER_FUNCS

    assert(converters_chosen == true);
}

// BEGIN DATAQUEUE
struct SDL_DataQueue;
typedef struct SDL_DataQueue SDL_DataQueue;

SDL_DataQueue *SDL_NewDataQueue(const size_t packetlen, const size_t initialslack);
void SDL_FreeDataQueue(SDL_DataQueue *queue);
void SDL_ClearDataQueue(SDL_DataQueue *queue, const size_t slack);
int SDL_WriteToDataQueue(SDL_DataQueue *queue, const void *data, const size_t len);
size_t SDL_ReadFromDataQueue(SDL_DataQueue *queue, void *buf, const size_t len);
size_t SDL_PeekIntoDataQueue(SDL_DataQueue *queue, void *buf, const size_t len);
size_t SDL_CountDataQueue(SDL_DataQueue *queue);
void *SDL_ReserveSpaceInDataQueue(SDL_DataQueue *queue, const size_t len);

typedef struct SDL_DataQueuePacket
{
    size_t datalen;  /* bytes currently in use in this packet. */
    size_t startpos;  /* bytes currently consumed in this packet. */
    struct SDL_DataQueuePacket *next;  /* next item in linked list. */
    uint8_t data[1];  /* packet data */ // SDL_VARIABLE_LENGTH_ARRAY
} SDL_DataQueuePacket;

struct SDL_DataQueue
{
    SDL_DataQueuePacket *head; /* device fed from here. */
    SDL_DataQueuePacket *tail; /* queue fills to here. */
    SDL_DataQueuePacket *pool; /* these are unused packets. */
    size_t packet_size;   /* size of new packets */
    size_t queued_bytes;  /* number of bytes of data in the queue. */
};

static void
SDL_FreeDataQueueList(SDL_DataQueuePacket *packet)
{
    while (packet) {
        SDL_DataQueuePacket *next = packet->next;
        free(packet);
        packet = next;
    }
}


/* this all expects that you managed thread safety elsewhere. */

SDL_DataQueue *
SDL_NewDataQueue(const size_t _packetlen, const size_t initialslack)
{
    SDL_DataQueue *queue = (SDL_DataQueue *) malloc(sizeof (SDL_DataQueue));

    if (!queue) {
        SDL_OutOfMemoryEX();
        return NULL;
    } else {
        const size_t packetlen = _packetlen ? _packetlen : 1024;
        const size_t wantpackets = (initialslack + (packetlen - 1)) / packetlen;
        size_t i;

        SDL_zeropEX(queue);
        queue->packet_size = packetlen;

        for (i = 0; i < wantpackets; i++) {
            SDL_DataQueuePacket *packet = (SDL_DataQueuePacket *) malloc(sizeof (SDL_DataQueuePacket) + packetlen);
            if (packet) { /* don't care if this fails, we'll deal later. */
                packet->datalen = 0;
                packet->startpos = 0;
                packet->next = queue->pool;
                queue->pool = packet;
            }
        }
    }

    return queue;
}

void
SDL_FreeDataQueue(SDL_DataQueue *queue)
{
    if (queue) {
        SDL_FreeDataQueueList(queue->head);
        SDL_FreeDataQueueList(queue->pool);
        free(queue);
    }
}

void
SDL_ClearDataQueue(SDL_DataQueue *queue, const size_t slack)
{
    const size_t packet_size = queue ? queue->packet_size : 1;
    const size_t slackpackets = (slack + (packet_size-1)) / packet_size;
    SDL_DataQueuePacket *packet;
    SDL_DataQueuePacket *prev = NULL;
    size_t i;

    if (!queue) {
        return;
    }

    packet = queue->head;

    /* merge the available pool and the current queue into one list. */
    if (packet) {
        queue->tail->next = queue->pool;
    } else {
        packet = queue->pool;
    }

    /* Remove the queued packets from the device. */
    queue->tail = NULL;
    queue->head = NULL;
    queue->queued_bytes = 0;
    queue->pool = packet;

    /* Optionally keep some slack in the pool to reduce malloc pressure. */
    for (i = 0; packet && (i < slackpackets); i++) {
        prev = packet;
        packet = packet->next;
    }

    if (prev) {
        prev->next = NULL;
    } else {
        queue->pool = NULL;
    }

    SDL_FreeDataQueueList(packet);  /* free extra packets */
}

static SDL_DataQueuePacket *
AllocateDataQueuePacket(SDL_DataQueue *queue)
{
    SDL_DataQueuePacket *packet;

    assert(queue != NULL);

    packet = queue->pool;
    if (packet != NULL) {
        /* we have one available in the pool. */
        queue->pool = packet->next;
    } else {
        /* Have to allocate a new one! */
        packet = (SDL_DataQueuePacket *) malloc(sizeof (SDL_DataQueuePacket) + queue->packet_size);
        if (packet == NULL) {
            return NULL;
        }
    }

    packet->datalen = 0;
    packet->startpos = 0;
    packet->next = NULL;
                
    assert((queue->head != NULL) == (queue->queued_bytes != 0));
    if (queue->tail == NULL) {
        queue->head = packet;
    } else {
        queue->tail->next = packet;
    }
    queue->tail = packet;
    return packet;
}


int
SDL_WriteToDataQueue(SDL_DataQueue *queue, const void *_data, const size_t _len)
{
    size_t len = _len;
    const uint8_t *data = (const uint8_t *) _data;
    const size_t packet_size = queue ? queue->packet_size : 0;
    SDL_DataQueuePacket *orighead;
    SDL_DataQueuePacket *origtail;
    size_t origlen;
    size_t datalen;

    if (!queue) {
        return SDL_PrintError("queue");
    }

    orighead = queue->head;
    origtail = queue->tail;
    origlen = origtail ? origtail->datalen : 0;

    while (len > 0) {
        SDL_DataQueuePacket *packet = queue->tail;
        assert(!packet || (packet->datalen <= packet_size));
        if (!packet || (packet->datalen >= packet_size)) {
            /* tail packet missing or completely full; we need a new packet. */
            packet = AllocateDataQueuePacket(queue);
            if (!packet) {
                /* uhoh, reset so we've queued nothing new, free what we can. */
                if (!origtail) {
                    packet = queue->head;  /* whole queue. */
                } else {
                    packet = origtail->next;  /* what we added to existing queue. */
                    origtail->next = NULL;
                    origtail->datalen = origlen;
                }
                queue->head = orighead;
                queue->tail = origtail;
                queue->pool = NULL;

                SDL_FreeDataQueueList(packet);  /* give back what we can. */
                return SDL_OutOfMemoryEX();
            }
        }

        datalen = SDL_minEX(len, packet_size - packet->datalen);
        memcpy(packet->data + packet->datalen, data, datalen);
        data += datalen;
        len -= datalen;
        packet->datalen += datalen;
        queue->queued_bytes += datalen;
    }

    return 0;
}

size_t
SDL_PeekIntoDataQueue(SDL_DataQueue *queue, void *_buf, const size_t _len)
{
    size_t len = _len;
    uint8_t *buf = (uint8_t *) _buf;
    uint8_t *ptr = buf;
    SDL_DataQueuePacket *packet;

    if (!queue) {
        return 0;
    }

    for (packet = queue->head; len && packet; packet = packet->next) {
        const size_t avail = packet->datalen - packet->startpos;
        const size_t cpy = SDL_minEX(len, avail);
        assert(queue->queued_bytes >= avail);

        memcpy(ptr, packet->data + packet->startpos, cpy);
        ptr += cpy;
        len -= cpy;
    }

    return (size_t) (ptr - buf);
}

size_t
SDL_ReadFromDataQueue(SDL_DataQueue *queue, void *_buf, const size_t _len)
{
    size_t len = _len;
    uint8_t *buf = (uint8_t *) _buf;
    uint8_t *ptr = buf;
    SDL_DataQueuePacket *packet;

    if (!queue) {
        return 0;
    }

    while ((len > 0) && ((packet = queue->head) != NULL)) {
        const size_t avail = packet->datalen - packet->startpos;
        const size_t cpy = SDL_minEX(len, avail);
        assert(queue->queued_bytes >= avail);

        memcpy(ptr, packet->data + packet->startpos, cpy);
        packet->startpos += cpy;
        ptr += cpy;
        queue->queued_bytes -= cpy;
        len -= cpy;

        if (packet->startpos == packet->datalen) {  /* packet is done, put it in the pool. */
            queue->head = packet->next;
            assert((packet->next != NULL) || (packet == queue->tail));
            packet->next = queue->pool;
            queue->pool = packet;
        }
    }

    assert((queue->head != NULL) == (queue->queued_bytes != 0));

    if (queue->head == NULL) {
        queue->tail = NULL;  /* in case we drained the queue entirely. */
    }

    return (size_t) (ptr - buf);
}

size_t
SDL_CountDataQueue(SDL_DataQueue *queue)
{
    return queue ? queue->queued_bytes : 0;
}

void *
SDL_ReserveSpaceInDataQueue(SDL_DataQueue *queue, const size_t len)
{
    SDL_DataQueuePacket *packet;

    if (!queue) {
        SDL_PrintError("queue");
        return NULL;
    } else if (len == 0) {
        SDL_PrintError("len");
        return NULL;
    } else if (len > queue->packet_size) {
        SDL_PrintError("len is larger than packet size");
        return NULL;
    }

    packet = queue->head;
    if (packet) {
        const size_t avail = queue->packet_size - packet->datalen;
        if (len <= avail) {  /* we can use the space at end of this packet. */
            void *retval = packet->data + packet->datalen;
            packet->datalen += len;
            queue->queued_bytes += len;
            return retval;
        }
    }

    /* Need a fresh packet. */
    packet = AllocateDataQueuePacket(queue);
    if (!packet) {
        SDL_OutOfMemoryEX();
        return NULL;
    }

    packet->datalen = len;
    queue->queued_bytes += len;
    return packet->data;
}

// AUDIOCVT
/* Convert from stereo to mono. Average left and right. */
static void
SDL_ConvertStereoToMono(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    float *dst = (float *) cvt->buf;
    const float *src = dst;
    int i;

    LOG_DEBUG_CONVERT("stereo", "mono");
    assert(format == AUDIO_F32SYS);

    for (i = cvt->len_cvt / 8; i; --i, src += 2) {
        *(dst++) = (src[0] + src[1]) * 0.5f;
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert from 5.1 to stereo. Average left and right, distribute center, discard LFE. */
static void
SDL_Convert51ToStereo(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    float *dst = (float *) cvt->buf;
    const float *src = dst;
    int i;

    LOG_DEBUG_CONVERT("5.1", "stereo");
    assert(format == AUDIO_F32SYS);

    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (i = cvt->len_cvt / (sizeof (float) * 6); i; --i, src += 6, dst += 2) {
        const float front_center_distributed = src[2] * 0.5f;
        dst[0] = (src[0] + front_center_distributed + src[4]) / 2.5f;  /* left */
        dst[1] = (src[1] + front_center_distributed + src[5]) / 2.5f;  /* right */
    }

    cvt->len_cvt /= 3;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert from quad to stereo. Average left and right. */
static void
SDL_ConvertQuadToStereo(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    float *dst = (float *) cvt->buf;
    const float *src = dst;
    int i;

    LOG_DEBUG_CONVERT("quad", "stereo");
    assert(format == AUDIO_F32SYS);

    for (i = cvt->len_cvt / (sizeof (float) * 4); i; --i, src += 4, dst += 2) {
        dst[0] = (src[0] + src[2]) * 0.5f; /* left */
        dst[1] = (src[1] + src[3]) * 0.5f; /* right */
    }

    cvt->len_cvt /= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert from 7.1 to 5.1. Distribute sides across front and back. */
static void
SDL_Convert71To51(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    float *dst = (float *) cvt->buf;
    const float *src = dst;
    int i;

    LOG_DEBUG_CONVERT("7.1", "5.1");
    assert(format == AUDIO_F32SYS);

    for (i = cvt->len_cvt / (sizeof (float) * 8); i; --i, src += 8, dst += 6) {
        const float surround_left_distributed = src[6] * 0.5f;
        const float surround_right_distributed = src[7] * 0.5f;
        dst[0] = (src[0] + surround_left_distributed) / 1.5f;  /* FL */
        dst[1] = (src[1] + surround_right_distributed) / 1.5f;  /* FR */
        dst[2] = src[2] / 1.5f; /* CC */
        dst[3] = src[3] / 1.5f; /* LFE */
        dst[4] = (src[4] + surround_left_distributed) / 1.5f;  /* BL */
        dst[5] = (src[5] + surround_right_distributed) / 1.5f;  /* BR */
    }

    cvt->len_cvt /= 8;
    cvt->len_cvt *= 6;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Convert from 5.1 to quad. Distribute center across front, discard LFE. */
static void
SDL_Convert51ToQuad(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    float *dst = (float *) cvt->buf;
    const float *src = dst;
    int i;

    LOG_DEBUG_CONVERT("5.1", "quad");
    assert(format == AUDIO_F32SYS);

    /* SDL's 4.0 layout: FL+FR+BL+BR */
    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (i = cvt->len_cvt / (sizeof (float) * 6); i; --i, src += 6, dst += 4) {
        const float front_center_distributed = src[2] * 0.5f;
        dst[0] = (src[0] + front_center_distributed) / 1.5f;  /* FL */
        dst[1] = (src[1] + front_center_distributed) / 1.5f;  /* FR */
        dst[2] = src[4] / 1.5f;  /* BL */
        dst[3] = src[5] / 1.5f;  /* BR */
    }

    cvt->len_cvt /= 6;
    cvt->len_cvt *= 4;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Upmix mono to stereo (by duplication) */
static void
SDL_ConvertMonoToStereo(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) (cvt->buf + cvt->len_cvt);
    float *dst = (float *) (cvt->buf + cvt->len_cvt * 2);
    int i;

    LOG_DEBUG_CONVERT("mono", "stereo");
    assert(format == AUDIO_F32SYS);

    for (i = cvt->len_cvt / sizeof (float); i; --i) {
        src--;
        dst -= 2;
        dst[0] = dst[1] = *src;
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Upmix stereo to a pseudo-5.1 stream */
static void
SDL_ConvertStereoTo51(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    int i;
    float lf, rf, ce;
    const float *src = (const float *) (cvt->buf + cvt->len_cvt);
    float *dst = (float *) (cvt->buf + cvt->len_cvt * 3);

    LOG_DEBUG_CONVERT("stereo", "5.1");
    assert(format == AUDIO_F32SYS);

    for (i = cvt->len_cvt / (sizeof(float) * 2); i; --i) {
        dst -= 6;
        src -= 2;
        lf = src[0];
        rf = src[1];
        ce = (lf + rf) * 0.5f;
        /* !!! FIXME: FL and FR may clip */
        dst[0] = lf + (lf - ce);  /* FL */
        dst[1] = rf + (rf - ce);  /* FR */
        dst[2] = ce;  /* FC */
        dst[3] = 0;   /* LFE (only meant for special LFE effects) */
        dst[4] = lf;  /* BL */
        dst[5] = rf;  /* BR */
    }

    cvt->len_cvt *= 3;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Upmix quad to a pseudo-5.1 stream */
static void
SDL_ConvertQuadTo51(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    int i;
    float lf, rf, lb, rb, ce;
    const float *src = (const float *) (cvt->buf + cvt->len_cvt);
    float *dst = (float *) (cvt->buf + cvt->len_cvt * 3 / 2);

    LOG_DEBUG_CONVERT("quad", "5.1");
    assert(format == AUDIO_F32SYS);
    assert(cvt->len_cvt % (sizeof(float) * 4) == 0);

    for (i = cvt->len_cvt / (sizeof(float) * 4); i; --i) {
        dst -= 6;
        src -= 4;
        lf = src[0];
        rf = src[1];
        lb = src[2];
        rb = src[3];
        ce = (lf + rf) * 0.5f;
        /* !!! FIXME: FL and FR may clip */
        dst[0] = lf + (lf - ce);  /* FL */
        dst[1] = rf + (rf - ce);  /* FR */
        dst[2] = ce;  /* FC */
        dst[3] = 0;   /* LFE (only meant for special LFE effects) */
        dst[4] = lb;  /* BL */
        dst[5] = rb;  /* BR */
    }

    cvt->len_cvt = cvt->len_cvt * 3 / 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Upmix stereo to a pseudo-4.0 stream (by duplication) */
static void
SDL_ConvertStereoToQuad(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    const float *src = (const float *) (cvt->buf + cvt->len_cvt);
    float *dst = (float *) (cvt->buf + cvt->len_cvt * 2);
    float lf, rf;
    int i;

    LOG_DEBUG_CONVERT("stereo", "quad");
    assert(format == AUDIO_F32SYS);

    for (i = cvt->len_cvt / (sizeof(float) * 2); i; --i) {
        dst -= 4;
        src -= 2;
        lf = src[0];
        rf = src[1];
        dst[0] = lf;  /* FL */
        dst[1] = rf;  /* FR */
        dst[2] = lf;  /* BL */
        dst[3] = rf;  /* BR */
    }

    cvt->len_cvt *= 2;
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}


/* Upmix 5.1 to 7.1 */
static void
SDL_Convert51To71(SDL_AudioCVT_EX * cvt, SDL_AudioFormat format)
{
    float lf, rf, lb, rb, ls, rs;
    int i;
    const float *src = (const float *) (cvt->buf + cvt->len_cvt);
    float *dst = (float *) (cvt->buf + cvt->len_cvt * 4 / 3);

    LOG_DEBUG_CONVERT("5.1", "7.1");
    assert(format == AUDIO_F32SYS);
    assert(cvt->len_cvt % (sizeof(float) * 6) == 0);

    for (i = cvt->len_cvt / (sizeof(float) * 6); i; --i) {
        dst -= 8;
        src -= 6;
        lf = src[0];
        rf = src[1];
        lb = src[4];
        rb = src[5];
        ls = (lf + lb) * 0.5f;
        rs = (rf + rb) * 0.5f;
        /* !!! FIXME: these four may clip */
        lf += lf - ls;
        rf += rf - ls;
        lb += lb - ls;
        rb += rb - ls;
        dst[3] = src[3];  /* LFE */
        dst[2] = src[2];  /* FC */
        dst[7] = rs; /* SR */
        dst[6] = ls; /* SL */
        dst[5] = rb;  /* BR */
        dst[4] = lb;  /* BL */
        dst[1] = rf;  /* FR */
        dst[0] = lf;  /* FL */
    }

    cvt->len_cvt = cvt->len_cvt * 4 / 3;

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index] (cvt, format);
    }
}

/* SDL's resampler uses a "bandlimited interpolation" algorithm:
     https://ccrma.stanford.edu/~jos/resample/ */

#define RESAMPLER_ZERO_CROSSINGS 5
#define RESAMPLER_BITS_PER_SAMPLE 16
#define RESAMPLER_SAMPLES_PER_ZERO_CROSSING  (1 << ((RESAMPLER_BITS_PER_SAMPLE / 2) + 1))
#define RESAMPLER_FILTER_SIZE ((RESAMPLER_SAMPLES_PER_ZERO_CROSSING * RESAMPLER_ZERO_CROSSINGS) + 1)

/* This is a "modified" bessel function, so you can't use POSIX j0() */
static double
bessel(const double x)
{
    const double xdiv2 = x / 2.0;
    double i0 = 1.0f;
    double f = 1.0f;
    int i = 1;

    while (true) {
        const double diff = pow(xdiv2, i * 2) / pow(f, 2);
        if (diff < 1.0e-21f) {
            break;
        }
        i0 += diff;
        i++;
        f *= (double) i;
    }

    return i0;
}

/* build kaiser table with cardinal sine applied to it, and array of differences between elements. */
static void
kaiser_and_sinc(float *table, float *diffs, const int tablelen, const double beta)
{
    const int lenm1 = tablelen - 1;
    const int lenm1div2 = lenm1 / 2;
    int i;

    table[0] = 1.0f;
    for (i = 1; i < tablelen; i++) {
        const double kaiser = bessel(beta * sqrt(1.0 - pow(((i - lenm1) / 2.0) / lenm1div2, 2.0))) / bessel(beta);
        table[tablelen - i] = (float) kaiser;
    }

    for (i = 1; i < tablelen; i++) {
        const float x = (((float) i) / ((float) RESAMPLER_SAMPLES_PER_ZERO_CROSSING)) * ((float) M_PI);
        table[i] *= sinf(x) / x;
        diffs[i - 1] = table[i] - table[i - 1];
    }
    diffs[lenm1] = 0.0f;
}


// static SDL_SpinLock ResampleFilterSpinlock = 0;
static float *ResamplerFilter = NULL;
static float *ResamplerFilterDifference = NULL;

static int
SDL_PrepareResampleFilter(void)
{
    // SDL_AtomicLock(&ResampleFilterSpinlock);
    if (!ResamplerFilter) {
        /* if dB > 50, beta=(0.1102 * (dB - 8.7)), according to Matlab. */
        const double dB = 80.0;
        const double beta = 0.1102 * (dB - 8.7);
        // const size_t alloclen = RESAMPLER_FILTER_SIZE * sizeof (float);
        static float resampler_filter_buffer[RESAMPLER_FILTER_SIZE] = { 0 };
        static float resampler_filter_difference_buffer[RESAMPLER_FILTER_SIZE] = { 0 };
        ResamplerFilter = resampler_filter_buffer;
        ResamplerFilterDifference = resampler_filter_difference_buffer;
        kaiser_and_sinc(ResamplerFilter, ResamplerFilterDifference, RESAMPLER_FILTER_SIZE, beta);
    }
    // SDL_AtomicUnlock(&ResampleFilterSpinlock);
    return 0;
}

static int
ResamplerPadding(const int inrate, const int outrate)
{
    if (inrate == outrate) {
        return 0;
    } else if (inrate > outrate) {
        return (int) ceil(((float) (RESAMPLER_SAMPLES_PER_ZERO_CROSSING * inrate) / ((float) outrate)));
    }
    return RESAMPLER_SAMPLES_PER_ZERO_CROSSING;
}

/* lpadding and rpadding are expected to be buffers of (ResamplePadding(inrate, outrate) * chans * sizeof (float)) bytes. */
static int
SDL_ResampleAudio(const int chans, const int inrate, const int outrate,
                        const float *lpadding, const float *rpadding,
                        const float *inbuf, const int inbuflen,
                        float *outbuf, const int outbuflen)
{
    const double finrate = (double) inrate;
    const double outtimeincr = 1.0 / ((float) outrate);
    const double  ratio = ((float) outrate) / ((float) inrate);
    const int paddinglen = ResamplerPadding(inrate, outrate);
    const int framelen = chans * (int)sizeof (float);
    const int inframes = inbuflen / framelen;
    const int wantedoutframes = (int) ((inbuflen / framelen) * ratio);  /* outbuflen isn't total to write, it's total available. */
    const int maxoutframes = outbuflen / framelen;
    const int outframes = SDL_minEX(wantedoutframes, maxoutframes);
    float *dst = outbuf;
    double outtime = 0.0;
    int i, j, chan;

    for (i = 0; i < outframes; i++) {
        const int srcindex = (int) (outtime * inrate);
        const double intime = ((double) srcindex) / finrate;
        const double innexttime = ((double) (srcindex + 1)) / finrate;
        const double interpolation1 = 1.0 - ((innexttime - outtime) / (innexttime - intime));
        const int filterindex1 = (int) (interpolation1 * RESAMPLER_SAMPLES_PER_ZERO_CROSSING);
        const double interpolation2 = 1.0 - interpolation1;
        const int filterindex2 = (int) (interpolation2 * RESAMPLER_SAMPLES_PER_ZERO_CROSSING);

        for (chan = 0; chan < chans; chan++) {
            float outsample = 0.0f;

            /* do this twice to calculate the sample, once for the "left wing" and then same for the right. */
            /* !!! FIXME: do both wings in one loop */
            for (j = 0; (filterindex1 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)) < RESAMPLER_FILTER_SIZE; j++) {
                const int srcframe = srcindex - j;
                /* !!! FIXME: we can bubble this conditional out of here by doing a pre loop. */
                const float insample = (srcframe < 0) ? lpadding[((paddinglen + srcframe) * chans) + chan] : inbuf[(srcframe * chans) + chan];
                outsample += (float)(insample * (ResamplerFilter[filterindex1 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)] + (interpolation1 * ResamplerFilterDifference[filterindex1 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)])));
            }

            for (j = 0; (filterindex2 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)) < RESAMPLER_FILTER_SIZE; j++) {
                const int srcframe = srcindex + 1 + j;
                /* !!! FIXME: we can bubble this conditional out of here by doing a post loop. */
                const float insample = (srcframe >= inframes) ? rpadding[((srcframe - inframes) * chans) + chan] : inbuf[(srcframe * chans) + chan];
                outsample += (float)(insample * (ResamplerFilter[filterindex2 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)] + (interpolation2 * ResamplerFilterDifference[filterindex2 + (j * RESAMPLER_SAMPLES_PER_ZERO_CROSSING)])));
            }
            *(dst++) = outsample;
        }

        outtime += outtimeincr;
    }

    return outframes * chans * sizeof (float);
}

int
SDL_ConvertAudio_EX(SDL_AudioCVT_EX * cvt)
{
    /* !!! FIXME: (cvt) should be const; stack-copy it here. */
    /* !!! FIXME: (actually, we can't...len_cvt needs to be updated. Grr.) */

    /* Make sure there's data to convert */
    if (cvt->buf == NULL) {
        return SDL_PrintError("No buffer allocated for conversion");
    }

    /* Return okay if no conversion is necessary */
    cvt->len_cvt = cvt->len;
    if (cvt->filters[0] == NULL) {
        return 0;
    }

    /* Set up the conversion and go! */
    cvt->filter_index = 0;
    cvt->filters[0] (cvt, cvt->src_format);
    return 0;
}

static void
SDL_Convert_Byteswap(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format)
{
#if DEBUG_CONVERT
    printf("Converting byte order\n");
#endif
    assert(0 && "we are doing byte swap!!!");
    /*
    switch (SDL_AUDIO_BITSIZE(format)) {
        #define CASESWAP(b) \
            case b: { \
                uint##_t *ptr = (uint##_t *) cvt->buf; \
                int i; \
                for (i = cvt->len_cvt / sizeof (*ptr); i; --i, ++ptr) { \
                    *ptr = SDL_Swap##b(*ptr); \
                } \
                break; \
            }

        CASESWAP(16);
        CASESWAP(32);
        CASESWAP(64);

        #undef CASESWAP

        default: assert(!"unhandled byteswap datatype!"); break;
    }
    */

    if (cvt->filters[++cvt->filter_index]) {
        /* flip endian flag for data. */
        if (format & SDL_AUDIO_MASK_ENDIAN) {
            format &= ~SDL_AUDIO_MASK_ENDIAN;
        } else {
            format |= SDL_AUDIO_MASK_ENDIAN;
        }
        cvt->filters[cvt->filter_index](cvt, format);
    }
}

static int
SDL_AddAudioCVTFilter(SDL_AudioCVT_EX *cvt, const SDL_AudioFilter_EX filter)
{
    if (cvt->filter_index >= SDL_AUDIOCVT_MAX_FILTERS) {
        return SDL_PrintError("Too many filters needed for conversion, exceeded maximum of ");
    }
    if (filter == NULL) {
        return SDL_PrintError("Audio filter pointer is NULL");
    }
    cvt->filters[cvt->filter_index++] = filter;
    cvt->filters[cvt->filter_index] = NULL; /* Moving terminator */
    return 0;
}

static int
SDL_BuildAudioTypeCVTToFloat(SDL_AudioCVT_EX *cvt, const SDL_AudioFormat src_fmt)
{
    int retval = 0;  /* 0 == no conversion necessary. */

    // assert(0 && "might check endian");
    // if ((SDL_AUDIO_ISBIGENDIAN(src_fmt) != 0) == (SDL_BYTEORDER == SDL_LIL_ENDIAN)) {
    //     if (SDL_AddAudioCVTFilter(cvt, SDL_Convert_Byteswap) < 0) {
    //         return -1;
    //     }
    //     retval = 1;  /* added a converter. */
    // }

    if (!SDL_AUDIO_ISFLOAT(src_fmt)) {
        const uint16_t src_bitsize = SDL_AUDIO_BITSIZE(src_fmt);
        const uint16_t dst_bitsize = 32;
        SDL_AudioFilter_EX filter = NULL;

        // assert(0 && "trying to do float convert");
        switch (src_fmt & ~SDL_AUDIO_MASK_ENDIAN) {
            case AUDIO_S8: filter = SDL_Convert_S8_to_F32; break;
            case AUDIO_U8: filter = SDL_Convert_U8_to_F32; break;
            case AUDIO_S16: filter = SDL_Convert_S16_to_F32; break;
            case AUDIO_U16: filter = SDL_Convert_U16_to_F32; break;
            case AUDIO_S32: filter = SDL_Convert_S32_to_F32; break;
            default: assert(!"Unexpected audio format!"); break;
        }

        if (!filter) {
            return SDL_PrintError("No conversion from source format to float available");
        }

        if (SDL_AddAudioCVTFilter(cvt, filter) < 0) {
            return -1;
        }
        if (src_bitsize < dst_bitsize) {
            const int mult = (dst_bitsize / src_bitsize);
            cvt->len_mult *= mult;
            cvt->len_ratio *= mult;
        } else if (src_bitsize > dst_bitsize) {
            cvt->len_ratio /= (src_bitsize / dst_bitsize);
        }

        retval = 1;  /* added a converter. */
    }

    return retval;
}

static int
SDL_BuildAudioTypeCVTFromFloat(SDL_AudioCVT_EX *cvt, const SDL_AudioFormat dst_fmt)
{
    int retval = 0;  /* 0 == no conversion necessary. */

    if (!SDL_AUDIO_ISFLOAT(dst_fmt)) {
        const uint16_t dst_bitsize = SDL_AUDIO_BITSIZE(dst_fmt);
        const uint16_t src_bitsize = 32;
        SDL_AudioFilter_EX filter = NULL;
        // assert(0 && "trying to do from float convert!");
        switch (dst_fmt & ~SDL_AUDIO_MASK_ENDIAN) {
            case AUDIO_S8: filter = SDL_Convert_F32_to_S8; break;
            case AUDIO_U8: filter = SDL_Convert_F32_to_U8; break;
            case AUDIO_S16: filter = SDL_Convert_F32_to_S16; break;
            case AUDIO_U16: filter = SDL_Convert_F32_to_U16; break;
            case AUDIO_S32: filter = SDL_Convert_F32_to_S32; break;
            default: assert(!"Unexpected audio format!"); break;
        }

        if (!filter) {
            return SDL_PrintError("No conversion from float to format 0x%.4x available");
        }

        if (SDL_AddAudioCVTFilter(cvt, filter) < 0) {
            return -1;
        }
        if (src_bitsize < dst_bitsize) {
            const int mult = (dst_bitsize / src_bitsize);
            cvt->len_mult *= mult;
            cvt->len_ratio *= mult;
        } else if (src_bitsize > dst_bitsize) {
            cvt->len_ratio /= (src_bitsize / dst_bitsize);
        }
        retval = 1;  /* added a converter. */
    }

    // assert(0 && "might check byte order");
    // if ((SDL_AUDIO_ISBIGENDIAN(dst_fmt) != 0) == (SDL_BYTEORDER == SDL_LIL_ENDIAN)) {
    //     if (SDL_AddAudioCVTFilter(cvt, SDL_Convert_Byteswap) < 0) {
    //         return -1;
    //     }
    //     retval = 1;  /* added a converter. */
    // }

    return retval;
}

static void
SDL_ResampleCVT(SDL_AudioCVT_EX *cvt, const int chans, const SDL_AudioFormat format)
{
    /* !!! FIXME in 2.1: there are ten slots in the filter list, and the theoretical maximum we use is six (seven with NULL terminator).
       !!! FIXME in 2.1:   We need to store data for this resampler, because the cvt structure doesn't store the original sample rates,
       !!! FIXME in 2.1:   so we steal the ninth and tenth slot.  :( */
    const int inrate = (int) (size_t) cvt->filters[SDL_AUDIOCVT_MAX_FILTERS-1];
    const int outrate = (int) (size_t) cvt->filters[SDL_AUDIOCVT_MAX_FILTERS];
    const float *src = (const float *) cvt->buf;
    const int srclen = cvt->len_cvt;
    /*float *dst = (float *) cvt->buf;
    const int dstlen = (cvt->len * cvt->len_mult);*/
    /* !!! FIXME: remove this if we can get the resampler to work in-place again. */
    float *dst = (float *) (cvt->buf + srclen);
    const int dstlen = (cvt->len * cvt->len_mult) - srclen;
    const int requestedpadding = ResamplerPadding(inrate, outrate);
    int paddingsamples;
    float *padding;

    if (requestedpadding < INT32_MAX / chans) {
        paddingsamples = requestedpadding * chans;
    } else {
        paddingsamples = 0;
    }
    assert(format == AUDIO_F32SYS);

    /* we keep no streaming state here, so pad with silence on both ends. */
    padding = (float *) calloc(paddingsamples ? paddingsamples : 1, sizeof (float));
    if (!padding) {
        SDL_OutOfMemoryEX();
        return;
    }

    cvt->len_cvt = SDL_ResampleAudio(chans, inrate, outrate, padding, padding, src, srclen, dst, dstlen);

    free(padding);

    memmove(cvt->buf, dst, cvt->len_cvt);  /* !!! FIXME: remove this if we can get the resampler to work in-place again. */

    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, format);
    }
}

/* !!! FIXME: We only have this macro salsa because SDL_AudioCVT_EX doesn't
   !!! FIXME:  store channel info, so we have to have function entry
   !!! FIXME:  points for each supported channel count and multiple
   !!! FIXME:  vs arbitrary. When we rev the ABI, clean this up. */
#define RESAMPLER_FUNCS(chans) \
    static void \
    SDL_ResampleCVT_c##chans(SDL_AudioCVT_EX *cvt, SDL_AudioFormat format) { \
        SDL_ResampleCVT(cvt, chans, format); \
    }
RESAMPLER_FUNCS(1)
RESAMPLER_FUNCS(2)
RESAMPLER_FUNCS(4)
RESAMPLER_FUNCS(6)
RESAMPLER_FUNCS(8)
#undef RESAMPLER_FUNCS

static SDL_AudioFilter_EX
ChooseCVTResampler(const int dst_channels)
{
    switch (dst_channels) {
        case 1: return SDL_ResampleCVT_c1;
        case 2: return SDL_ResampleCVT_c2;
        case 4: return SDL_ResampleCVT_c4;
        case 6: return SDL_ResampleCVT_c6;
        case 8: return SDL_ResampleCVT_c8;
        default: break;
    }

    return NULL;
}

static int
SDL_BuildAudioResampleCVT(SDL_AudioCVT_EX * cvt, const int dst_channels,
                          const int src_rate, const int dst_rate)
{
    SDL_AudioFilter_EX filter;

    if (src_rate == dst_rate) {
        return 0;  /* no conversion necessary. */
    }

    filter = ChooseCVTResampler(dst_channels);
    if (filter == NULL) {
        return SDL_PrintError("No conversion available for these rates");
    }

    if (SDL_PrepareResampleFilter() < 0) {
        return -1;
    }

    /* Update (cvt) with filter details... */
    if (SDL_AddAudioCVTFilter(cvt, filter) < 0) {
        return -1;
    }

    /* !!! FIXME in 2.1: there are ten slots in the filter list, and the theoretical maximum we use is six (seven with NULL terminator).
       !!! FIXME in 2.1:   We need to store data for this resampler, because the cvt structure doesn't store the original sample rates,
       !!! FIXME in 2.1:   so we steal the ninth and tenth slot.  :( */
    if (cvt->filter_index >= (SDL_AUDIOCVT_MAX_FILTERS-2)) {
        return SDL_PrintError("Too many filters needed for conversion, exceeded maximum of");
    }
    cvt->filters[SDL_AUDIOCVT_MAX_FILTERS-1] = (SDL_AudioFilter_EX) (size_t) src_rate;
    cvt->filters[SDL_AUDIOCVT_MAX_FILTERS] = (SDL_AudioFilter_EX) (size_t) dst_rate;

    if (src_rate < dst_rate) {
        const double mult = ((double) dst_rate) / ((double) src_rate);
        cvt->len_mult *= (int) ceil(mult);
        cvt->len_ratio *= mult;
    } else {
        cvt->len_ratio /= ((double) src_rate) / ((double) dst_rate);
    }

    /* !!! FIXME: remove this if we can get the resampler to work in-place again. */
    /* the buffer is big enough to hold the destination now, but
       we need it large enough to hold a separate scratch buffer. */
    cvt->len_mult *= 2;

    return 1;               /* added a converter. */
}

static bool
SDL_SupportedAudioFormat(const SDL_AudioFormat fmt)
{
    switch (fmt) {
        case AUDIO_U8:
        case AUDIO_S8:
        case AUDIO_U16LSB:
        case AUDIO_S16LSB:
        case AUDIO_U16MSB:
        case AUDIO_S16MSB:
        case AUDIO_S32LSB:
        case AUDIO_S32MSB:
        case AUDIO_F32LSB:
        case AUDIO_F32MSB:
            return true;  /* supported. */

        default:
            break;
    }

    return false;  /* unsupported. */
}

static bool
SDL_SupportedChannelCount(const int channels)
{
    switch (channels) {
        case 1:  /* mono */
        case 2:  /* stereo */
        case 4:  /* quad */
        case 6:  /* 5.1 */
        case 8:  /* 7.1 */
          return true;  /* supported. */

        default:
            break;
    }

    return false;  /* unsupported. */
}


/* Creates a set of audio filters to convert from one format to another.
   Returns 0 if no conversion is needed, 1 if the audio filter is set up,
   or -1 if an error like invalid parameter, unsupported format, etc. occurred.
*/

int
SDL_BuildAudioCVT_EX(SDL_AudioCVT_EX * cvt,
                  SDL_AudioFormat src_fmt, uint8_t src_channels, int src_rate,
                  SDL_AudioFormat dst_fmt, uint8_t dst_channels, int dst_rate)
{
    /* Sanity check target pointer */
    if (cvt == NULL) {
        return SDL_PrintError("cvt");
    }

    /* Make sure we zero out the audio conversion before error checking */
    SDL_zeropEX(cvt);

    if (!SDL_SupportedAudioFormat(src_fmt)) {
        return SDL_PrintError("Invalid source format");
    } else if (!SDL_SupportedAudioFormat(dst_fmt)) {
        return SDL_PrintError("Invalid destination format");
    } else if (!SDL_SupportedChannelCount(src_channels)) {
        return SDL_PrintError("Invalid source channels");
    } else if (!SDL_SupportedChannelCount(dst_channels)) {
        return SDL_PrintError("Invalid destination channels");
    } else if (src_rate <= 0) {
        return SDL_PrintError("Source rate is equal to or less than zero");
    } else if (dst_rate <= 0) {
        return SDL_PrintError("Destination rate is equal to or less than zero");
    } else if (src_rate >= INT32_MAX / RESAMPLER_SAMPLES_PER_ZERO_CROSSING) {
        return SDL_PrintError("Source rate is too high");
    } else if (dst_rate >= INT32_MAX / RESAMPLER_SAMPLES_PER_ZERO_CROSSING) {
        return SDL_PrintError("Destination rate is too high");
    }

#if DEBUG_CONVERT
    printf("Build format %04x->%04x, channels %u->%u, rate %d->%d\n",
           src_fmt, dst_fmt, src_channels, dst_channels, src_rate, dst_rate);
#endif

    /* Start off with no conversion necessary */
    cvt->src_format = src_fmt;
    cvt->dst_format = dst_fmt;
    cvt->needed = 0;
    cvt->filter_index = 0;
    SDL_zeroaEX(cvt->filters);
    cvt->len_mult = 1;
    cvt->len_ratio = 1.0;
    cvt->rate_incr = ((double) dst_rate) / ((double) src_rate);

    /* Make sure we've chosen audio conversion functions (MMX, scalar, etc.) */
    // assert(0 && "choosing audio converters");
    SDL_ChooseAudioConverters();

    /* Type conversion goes like this now:
        - byteswap to CPU native format first if necessary.
        - convert to native Float32 if necessary.
        - resample and change channel count if necessary.
        - convert back to native format.
        - byteswap back to foreign format if necessary.

       The expectation is we can process data faster in float32
       (possibly with SIMD), and making several passes over the same
       buffer is likely to be CPU cache-friendly, avoiding the
       biggest performance hit in modern times. Previously we had
       (script-generated) custom converters for every data type and
       it was a bloat on SDL compile times and final library size. */

    /* see if we can skip float conversion entirely. */
    if (src_rate == dst_rate && src_channels == dst_channels) {
        if (src_fmt == dst_fmt) {
            return 0;
        }

        /* just a byteswap needed? */
        if ((src_fmt & ~SDL_AUDIO_MASK_ENDIAN) == (dst_fmt & ~SDL_AUDIO_MASK_ENDIAN)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_Convert_Byteswap) < 0) {
                return -1;
            }
            cvt->needed = 1;
            return 1;
        }
    }

    /* Convert data types, if necessary. Updates (cvt). */
    if (SDL_BuildAudioTypeCVTToFloat(cvt, src_fmt) < 0) {
        return -1;              /* shouldn't happen, but just in case... */
    }

    /* Channel conversion */
    if (src_channels < dst_channels) {
        /* Upmixing */
        /* Mono -> Stereo [-> ...] */
        if ((src_channels == 1) && (dst_channels > 1)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_ConvertMonoToStereo) < 0) {
                return -1;
            }
            cvt->len_mult *= 2;
            src_channels = 2;
            cvt->len_ratio *= 2;
        }
        /* [Mono ->] Stereo -> 5.1 [-> 7.1] */
        if ((src_channels == 2) && (dst_channels >= 6)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_ConvertStereoTo51) < 0) {
                return -1;
            }
            src_channels = 6;
            cvt->len_mult *= 3;
            cvt->len_ratio *= 3;
        }
        /* Quad -> 5.1 [-> 7.1] */
        if ((src_channels == 4) && (dst_channels >= 6)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_ConvertQuadTo51) < 0) {
                return -1;
            }
            src_channels = 6;
            cvt->len_mult = (cvt->len_mult * 3 + 1) / 2;
            cvt->len_ratio *= 1.5;
        }
        /* [[Mono ->] Stereo ->] 5.1 -> 7.1 */
        if ((src_channels == 6) && (dst_channels == 8)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_Convert51To71) < 0) {
                return -1;
            }
            src_channels = 8;
            cvt->len_mult = (cvt->len_mult * 4 + 2) / 3;
            /* Should be numerically exact with every valid input to this
               function */
            cvt->len_ratio = cvt->len_ratio * 4 / 3;
        }
        /* [Mono ->] Stereo -> Quad */
        if ((src_channels == 2) && (dst_channels == 4)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_ConvertStereoToQuad) < 0) {
                return -1;
            }
            src_channels = 4;
            cvt->len_mult *= 2;
            cvt->len_ratio *= 2;
        }
    } else if (src_channels > dst_channels) {
        /* Downmixing */
        /* 7.1 -> 5.1 [-> Stereo [-> Mono]] */
        /* 7.1 -> 5.1 [-> Quad] */
        if ((src_channels == 8) && (dst_channels <= 6)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_Convert71To51) < 0) {
                return -1;
            }
            src_channels = 6;
            cvt->len_ratio *= 0.75;
        }
        /* [7.1 ->] 5.1 -> Stereo [-> Mono] */
        if ((src_channels == 6) && (dst_channels <= 2)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_Convert51ToStereo) < 0) {
                return -1;
            }
            src_channels = 2;
            cvt->len_ratio /= 3;
        }
        /* 5.1 -> Quad */
        if ((src_channels == 6) && (dst_channels == 4)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_Convert51ToQuad) < 0) {
                return -1;
            }
            src_channels = 4;
            cvt->len_ratio = cvt->len_ratio * 2 / 3;
        }
        /* Quad -> Stereo [-> Mono] */
        if ((src_channels == 4) && (dst_channels <= 2)) {
            if (SDL_AddAudioCVTFilter(cvt, SDL_ConvertQuadToStereo) < 0) {
                return -1;
            }
            src_channels = 2;
            cvt->len_ratio /= 2;
        }
        /* [... ->] Stereo -> Mono */
        if ((src_channels == 2) && (dst_channels == 1)) {
            SDL_AudioFilter_EX filter = NULL;

            #if HAVE_SSE3_INTRINSICS
            if (SDL_HasSSE3()) {
                filter = SDL_ConvertStereoToMono_SSE3;
            }
            #endif

            if (!filter) {
                filter = SDL_ConvertStereoToMono;
            }

            if (SDL_AddAudioCVTFilter(cvt, filter) < 0) {
                return -1;
            }

            src_channels = 1;
            cvt->len_ratio /= 2;
        }
    }

    if (src_channels != dst_channels) {
        /* All combinations of supported channel counts should have been
           handled by now, but let's be defensive */
      return SDL_PrintError("Invalid channel combination");
    }
    
    /* Do rate conversion, if necessary. Updates (cvt). */
    if (SDL_BuildAudioResampleCVT(cvt, dst_channels, src_rate, dst_rate) < 0) {
        return -1;              /* shouldn't happen, but just in case... */
    }

    /* Move to final data type. */
    if (SDL_BuildAudioTypeCVTFromFloat(cvt, dst_fmt) < 0) {
        return -1;              /* shouldn't happen, but just in case... */
    }

    cvt->needed = (cvt->filter_index != 0);
    return (cvt->needed);
}

typedef int (*SDL_ResampleAudioStreamFunc)(SDL_AudioStream *stream, const void *inbuf, const int inbuflen, void *outbuf, const int outbuflen);
typedef void (*SDL_ResetAudioStreamResamplerFunc)(SDL_AudioStream *stream);
typedef void (*SDL_CleanupAudioStreamResamplerFunc)(SDL_AudioStream *stream);

struct _SDL_AudioStream
{
    SDL_AudioCVT_EX cvt_before_resampling;
    SDL_AudioCVT_EX cvt_after_resampling;
    SDL_DataQueue *queue;
    bool first_run;
    uint8_t *staging_buffer;
    int staging_buffer_size;
    int staging_buffer_filled;
    uint8_t *work_buffer_base;  /* maybe unaligned pointer from realloc(). */
    int work_buffer_len;
    int src_sample_frame_size;
    SDL_AudioFormat src_format;
    uint8_t src_channels;
    int src_rate;
    int dst_sample_frame_size;
    SDL_AudioFormat dst_format;
    uint8_t dst_channels;
    int dst_rate;
    double rate_incr;
    uint8_t pre_resample_channels;
    int packetlen;
    int resampler_padding_samples;
    float *resampler_padding;
    void *resampler_state;
    SDL_ResampleAudioStreamFunc resampler_func;
    SDL_ResetAudioStreamResamplerFunc reset_resampler_func;
    SDL_CleanupAudioStreamResamplerFunc cleanup_resampler_func;
};

static uint8_t *
EnsureStreamBufferSize(SDL_AudioStream *stream, const int newlen)
{
    uint8_t *ptr;
    size_t offset;

    if (stream->work_buffer_len >= newlen) {
        ptr = stream->work_buffer_base;
    } else {
        ptr = (uint8_t *) realloc(stream->work_buffer_base, newlen + 32);
        if (!ptr) {
            SDL_OutOfMemoryEX();
            return NULL;
        }
        /* Make sure we're aligned to 16 bytes for SIMD code. */
        stream->work_buffer_base = ptr;
        stream->work_buffer_len = newlen;
    }

    offset = ((size_t) ptr) & 15;
    return offset ? ptr + (16 - offset) : ptr;
}

#ifdef HAVE_LIBSAMPLERATE_H

#include <samplerate.h>
static int SRC_converter = SRC_SINC_MEDIUM_QUALITY;//SRC_SINC_FASTEST;

static int
SDL_ResampleAudioStream_SRC(SDL_AudioStream *stream, const void *_inbuf, const int inbuflen, void *_outbuf, const int outbuflen)
{
    const float *inbuf = (const float *) _inbuf;
    float *outbuf = (float *) _outbuf;
    const int framelen = sizeof(float) * stream->pre_resample_channels;
    SRC_STATE *state = (SRC_STATE *)stream->resampler_state;
    SRC_DATA data;
    int result;

    assert(inbuf != ((const float *) outbuf));  /* SDL_AudioStreamPut() shouldn't allow in-place resamples. */

    data.data_in = (float *)inbuf; /* Older versions of libsamplerate had a non-const pointer, but didn't write to it */
    data.input_frames = inbuflen / framelen;
    data.input_frames_used = 0;

    data.data_out = outbuf;
    data.output_frames = outbuflen / framelen;

    data.end_of_input = 0;
    data.src_ratio = stream->rate_incr;

    result = src_process(state, &data);
    if (result != 0) {
        // SDL_SetError("src_process() failed: %s", SRC_src_strerror(result));
        return 0;
    }

    /* If this fails, we need to store them off somewhere */
    assert(data.input_frames_used == data.input_frames);

    return data.output_frames_gen * (sizeof(float) * stream->pre_resample_channels);
}

static void
SDL_ResetAudioStreamResampler_SRC(SDL_AudioStream *stream)
{
    src_reset((SRC_STATE *)stream->resampler_state);
}

static void
SDL_CleanupAudioStreamResampler_SRC(SDL_AudioStream *stream)
{
    SRC_STATE *state = (SRC_STATE *)stream->resampler_state;
    if (state) {
        src_delete(state);
    }

    stream->resampler_state = NULL;
    stream->resampler_func = NULL;
    stream->reset_resampler_func = NULL;
    stream->cleanup_resampler_func = NULL;
}

static bool
SetupLibSampleRateResampling(SDL_AudioStream *stream)
{
    int result = 0;
    SRC_STATE *state = NULL;

    state = src_new(SRC_converter, stream->pre_resample_channels, &result);
    if (!state) {
        printf("fail to init libsamplerate\n");
        // SDL_SetError("src_new() failed: %s", src_strerror(result));
        SDL_CleanupAudioStreamResampler_SRC(stream);
        return false;
    }

    stream->resampler_state = state;
    stream->resampler_func = SDL_ResampleAudioStream_SRC;
    stream->reset_resampler_func = SDL_ResetAudioStreamResampler_SRC;
    stream->cleanup_resampler_func = SDL_CleanupAudioStreamResampler_SRC;

    return true;
}
#endif /* HAVE_LIBSAMPLERATE_H */

static int
SDL_ResampleAudioStream(SDL_AudioStream *stream, const void *_inbuf, const int inbuflen, void *_outbuf, const int outbuflen)
{
    const uint8_t *inbufend = ((const uint8_t *) _inbuf) + inbuflen;
    const float *inbuf = (const float *) _inbuf;
    float *outbuf = (float *) _outbuf;
    const int chans = (int) stream->pre_resample_channels;
    const int inrate = stream->src_rate;
    const int outrate = stream->dst_rate;
    const int paddingsamples = stream->resampler_padding_samples;
    const int paddingbytes = paddingsamples * sizeof (float);
    float *lpadding = (float *) stream->resampler_state;
    const float *rpadding = (const float *) inbufend; /* we set this up so there are valid padding samples at the end of the input buffer. */
    const int cpy = SDL_minEX(inbuflen, paddingbytes);
    int retval;

    assert(inbuf != ((const float *) outbuf));  /* SDL_AudioStreamPutEX() shouldn't allow in-place resamples. */

    retval = SDL_ResampleAudio(chans, inrate, outrate, lpadding, rpadding, inbuf, inbuflen, outbuf, outbuflen);

    /* update our left padding with end of current input, for next run. */
    memcpy((lpadding + paddingsamples) - (cpy / sizeof (float)), inbufend - cpy, cpy);
    return retval;
}

static void
SDL_ResetAudioStreamResampler(SDL_AudioStream *stream)
{
    /* set all the padding to silence. */
    const int len = stream->resampler_padding_samples;
    memset(stream->resampler_state, '\0', len * sizeof (float));
}

static void
SDL_CleanupAudioStreamResampler(SDL_AudioStream *stream)
{
    free(stream->resampler_state);
}

SDL_AudioStream *
SDL_NewAudioStreamEX(const SDL_AudioFormat src_format,
                   const uint8_t src_channels,
                   const int src_rate,
                   const SDL_AudioFormat dst_format,
                   const uint8_t dst_channels,
                   const int dst_rate)
{
    const int packetlen = 4096;  /* !!! FIXME: good enough for now. */
    uint8_t pre_resample_channels;
    SDL_AudioStream *retval;

    retval = (SDL_AudioStream *) calloc(1, sizeof (SDL_AudioStream));
    if (!retval) {
        return NULL;
    }

    /* If increasing channels, do it after resampling, since we'd just
       do more work to resample duplicate channels. If we're decreasing, do
       it first so we resample the interpolated data instead of interpolating
       the resampled data (!!! FIXME: decide if that works in practice, though!). */
    pre_resample_channels = SDL_minEX(src_channels, dst_channels);

    retval->first_run = true;
    retval->src_sample_frame_size = (SDL_AUDIO_BITSIZE(src_format) / 8) * src_channels;
    retval->src_format = src_format;
    retval->src_channels = src_channels;
    retval->src_rate = src_rate;
    retval->dst_sample_frame_size = (SDL_AUDIO_BITSIZE(dst_format) / 8) * dst_channels;
    retval->dst_format = dst_format;
    retval->dst_channels = dst_channels;
    retval->dst_rate = dst_rate;
    retval->pre_resample_channels = pre_resample_channels;
    retval->packetlen = packetlen;
    retval->rate_incr = ((double) dst_rate) / ((double) src_rate);
    retval->resampler_padding_samples = ResamplerPadding(retval->src_rate, retval->dst_rate) * pre_resample_channels;
    retval->resampler_padding = (float *) calloc(retval->resampler_padding_samples ? retval->resampler_padding_samples : 1, sizeof (float));

    if (retval->resampler_padding == NULL) {
        SDL_FreeAudioStreamEX(retval);
        SDL_OutOfMemoryEX();
        return NULL;
    }

    retval->staging_buffer_size = ((retval->resampler_padding_samples / retval->pre_resample_channels) * retval->src_sample_frame_size);
    if (retval->staging_buffer_size > 0) {
        retval->staging_buffer = (uint8_t *) malloc(retval->staging_buffer_size);
        if (retval->staging_buffer == NULL) {
            SDL_FreeAudioStreamEX(retval);
            SDL_OutOfMemoryEX();
            return NULL;
        }
    }

    /* Not resampling? It's an easy conversion (and maybe not even that!) */
    if (src_rate == dst_rate) {
        retval->cvt_before_resampling.needed = false;
        if (SDL_BuildAudioCVT_EX(&retval->cvt_after_resampling, src_format, src_channels, dst_rate, dst_format, dst_channels, dst_rate) < 0) {
            SDL_FreeAudioStreamEX(retval);
            return NULL;  /* SDL_BuildAudioCVT_EX should have called fprintf.stderr,  */
        }
    } else {
        /* Don't resample at first. Just get us to Float32 format. */
        /* !!! FIXME: convert to int32 on devices without hardware float. */
        if (SDL_BuildAudioCVT_EX(&retval->cvt_before_resampling, src_format, src_channels, src_rate, AUDIO_F32SYS, pre_resample_channels, src_rate) < 0) {
            SDL_FreeAudioStreamEX(retval);
            return NULL;  /* SDL_BuildAudioCVT_EX should have called fprintf.stderr,  */
        }

    #ifdef HAVE_LIBSAMPLERATE_H
        SetupLibSampleRateResampling(retval);
    #endif

        if (!retval->resampler_func) {
            retval->resampler_state = calloc(retval->resampler_padding_samples, sizeof (float));
            if (!retval->resampler_state) {
                SDL_FreeAudioStreamEX(retval);
                SDL_OutOfMemoryEX();
                return NULL;
            }

            if (SDL_PrepareResampleFilter() < 0) {
                free(retval->resampler_state);
                retval->resampler_state = NULL;
                SDL_FreeAudioStreamEX(retval);
                return NULL;
            }

            retval->resampler_func = SDL_ResampleAudioStream;
            retval->reset_resampler_func = SDL_ResetAudioStreamResampler;
            retval->cleanup_resampler_func = SDL_CleanupAudioStreamResampler;
        }

        /* Convert us to the final format after resampling. */
        if (SDL_BuildAudioCVT_EX(&retval->cvt_after_resampling, AUDIO_F32SYS, pre_resample_channels, dst_rate, dst_format, dst_channels, dst_rate) < 0) {
            SDL_FreeAudioStreamEX(retval);
            return NULL;  /* SDL_BuildAudioCVT_EX should have called fprintf.stderr,  */
        }
    }

    retval->queue = SDL_NewDataQueue(packetlen, packetlen * 2);
    if (!retval->queue) {
        SDL_FreeAudioStreamEX(retval);
        return NULL;  /* SDL_NewDataQueue should have called fprintf.stderr,  */
    }

    return retval;
}

static int
SDL_AudioStreamPutInternal(SDL_AudioStream *stream, const void *buf, int len, int *maxputbytes)
{
    int buflen = len;
    int workbuflen;
    uint8_t *workbuf;
    uint8_t *resamplebuf = NULL;
    int resamplebuflen = 0;
    int neededpaddingbytes;
    int paddingbytes;

    /* !!! FIXME: several converters can take advantage of SIMD, but only
       !!! FIXME:  if the data is aligned to 16 bytes. EnsureStreamBufferSize()
       !!! FIXME:  guarantees the buffer will align, but the
       !!! FIXME:  converters will iterate over the data backwards if
       !!! FIXME:  the output grows, and this means we won't align if buflen
       !!! FIXME:  isn't a multiple of 16. In these cases, we should chop off
       !!! FIXME:  a few samples at the end and convert them separately. */

    /* no padding prepended on first run. */
    neededpaddingbytes = stream->resampler_padding_samples * sizeof (float);
    paddingbytes = stream->first_run ? 0 : neededpaddingbytes;
    stream->first_run = false;

    /* Make sure the work buffer can hold all the data we need at once... */
    workbuflen = buflen;
    if (stream->cvt_before_resampling.needed) {
        workbuflen *= stream->cvt_before_resampling.len_mult;
    }

    if (stream->dst_rate != stream->src_rate) {
        /* resamples can't happen in place, so make space for second buf. */
        const int framesize = stream->pre_resample_channels * sizeof (float);
        const int frames = workbuflen / framesize;
        resamplebuflen = ((int) ceil(frames * stream->rate_incr)) * framesize;
        #if DEBUG_AUDIOSTREAM
        printf("AUDIOSTREAM: will resample %d bytes to %d (ratio=%.6f)\n", workbuflen, resamplebuflen, stream->rate_incr);
        #endif
        workbuflen += resamplebuflen;
    }

    if (stream->cvt_after_resampling.needed) {
        /* !!! FIXME: buffer might be big enough already? */
        workbuflen *= stream->cvt_after_resampling.len_mult;
    }

    workbuflen += neededpaddingbytes;

    #if DEBUG_AUDIOSTREAM
    printf("AUDIOSTREAM: Putting %d bytes of preconverted audio, need %d byte work buffer\n", buflen, workbuflen);
    #endif

    workbuf = EnsureStreamBufferSize(stream, workbuflen);
    if (!workbuf) {
        return -1;  /* probably out of memory. */
    }

    resamplebuf = workbuf;  /* default if not resampling. */

    memcpy(workbuf + paddingbytes, buf, buflen);

    if (stream->cvt_before_resampling.needed) {
        stream->cvt_before_resampling.buf = workbuf + paddingbytes;
        stream->cvt_before_resampling.len = buflen;
        if (SDL_ConvertAudio_EX(&stream->cvt_before_resampling) == -1) {
            return -1;   /* uhoh! */
        }
        buflen = stream->cvt_before_resampling.len_cvt;

        #if DEBUG_AUDIOSTREAM
        printf("AUDIOSTREAM: After initial conversion we have %d bytes\n", buflen);
        #endif
    }

    if (stream->dst_rate != stream->src_rate) {
        /* save off some samples at the end; they are used for padding now so
           the resampler is coherent and then used at the start of the next
           put operation. Prepend last put operation's padding, too. */

        /* prepend prior put's padding. :P */
        if (paddingbytes) {
            memcpy(workbuf, stream->resampler_padding, paddingbytes);
            buflen += paddingbytes;
        }

        /* save off the data at the end for the next run. */
        memcpy(stream->resampler_padding, workbuf + (buflen - neededpaddingbytes), neededpaddingbytes);

        resamplebuf = workbuf + buflen;  /* skip to second piece of workbuf. */
        assert(buflen >= neededpaddingbytes);
        if (buflen > neededpaddingbytes) {
            buflen = stream->resampler_func(stream, workbuf, buflen - neededpaddingbytes, resamplebuf, resamplebuflen);
        } else {
            buflen = 0;
        }

        #if DEBUG_AUDIOSTREAM
        printf("AUDIOSTREAM: After resampling we have %d bytes\n", buflen);
        #endif
    }

    if (stream->cvt_after_resampling.needed && (buflen > 0)) {
        stream->cvt_after_resampling.buf = resamplebuf;
        stream->cvt_after_resampling.len = buflen;
        if (SDL_ConvertAudio_EX(&stream->cvt_after_resampling) == -1) {
            return -1;   /* uhoh! */
        }
        buflen = stream->cvt_after_resampling.len_cvt;

        #if DEBUG_AUDIOSTREAM
        printf("AUDIOSTREAM: After final conversion we have %d bytes\n", buflen);
        #endif
    }

    #if DEBUG_AUDIOSTREAM
    printf("AUDIOSTREAM: Final output is %d bytes\n", buflen);
    #endif

    if (maxputbytes) {
        const int maxbytes = *maxputbytes;
        if (buflen > maxbytes)
            buflen = maxbytes;
        *maxputbytes -= buflen;
    }

    /* resamplebuf holds the final output, even if we didn't resample. */
    return buflen ? SDL_WriteToDataQueue(stream->queue, resamplebuf, buflen) : 0;
}

int
SDL_AudioStreamPutEX(SDL_AudioStream *stream, const void *buf, int len)
{
    /* !!! FIXME: several converters can take advantage of SIMD, but only
       !!! FIXME:  if the data is aligned to 16 bytes. EnsureStreamBufferSize()
       !!! FIXME:  guarantees the buffer will align, but the
       !!! FIXME:  converters will iterate over the data backwards if
       !!! FIXME:  the output grows, and this means we won't align if buflen
       !!! FIXME:  isn't a multiple of 16. In these cases, we should chop off
       !!! FIXME:  a few samples at the end and convert them separately. */

    #if DEBUG_AUDIOSTREAM
    printf("AUDIOSTREAM: wants to put %d preconverted bytes\n", buflen);
    #endif

    if (!stream) {
        return SDL_PrintError("stream");
    } else if (!buf) {
        return SDL_PrintError("buf");
    } else if (len == 0) {
        return 0;  /* nothing to do. */
    } else if ((len % stream->src_sample_frame_size) != 0) {
        return SDL_PrintError("Can't add partial sample frames");
    }

    if (!stream->cvt_before_resampling.needed &&
        (stream->dst_rate == stream->src_rate) &&
        !stream->cvt_after_resampling.needed) {
        #if DEBUG_AUDIOSTREAM
        printf("AUDIOSTREAM: no conversion needed at all, queueing %d bytes.\n", len);
        #endif
        return SDL_WriteToDataQueue(stream->queue, buf, len);
    }

    while (len > 0) {
        int amount;

        /* If we don't have a staging buffer or we're given enough data that
           we don't need to store it for later, skip the staging process.
         */
        if (!stream->staging_buffer_filled && len >= stream->staging_buffer_size) {
            return SDL_AudioStreamPutInternal(stream, buf, len, NULL);
        }

        /* If there's not enough data to fill the staging buffer, just save it */
        if ((stream->staging_buffer_filled + len) < stream->staging_buffer_size) {
            memcpy(stream->staging_buffer + stream->staging_buffer_filled, buf, len);
            stream->staging_buffer_filled += len;
            return 0;
        }
 
        /* Fill the staging buffer, process it, and continue */
        amount = (stream->staging_buffer_size - stream->staging_buffer_filled);
        assert(amount > 0);
        memcpy(stream->staging_buffer + stream->staging_buffer_filled, buf, amount);
        stream->staging_buffer_filled = 0;
        if (SDL_AudioStreamPutInternal(stream, stream->staging_buffer, stream->staging_buffer_size, NULL) < 0) {
            return -1;
        }
        buf = (void *)((uint8_t *)buf + amount);
        len -= amount;
    }
    return 0;
}

int SDL_AudioStreamFlushEX(SDL_AudioStream *stream)
{
    if (!stream) {
        return SDL_PrintError("stream");
    }

    #if DEBUG_AUDIOSTREAM
    printf("AUDIOSTREAM: flushing! staging_buffer_filled=%d bytes\n", stream->staging_buffer_filled);
    #endif

    /* shouldn't use a staging buffer if we're not resampling. */
    assert((stream->dst_rate != stream->src_rate) || (stream->staging_buffer_filled == 0));

    if (stream->staging_buffer_filled > 0) {
        /* push the staging buffer + silence. We need to flush out not just
           the staging buffer, but the piece that the stream was saving off
           for right-side resampler padding. */
        const bool first_run = stream->first_run;
        const int filled = stream->staging_buffer_filled;
        int actual_input_frames = filled / stream->src_sample_frame_size;
        if (!first_run)
            actual_input_frames += stream->resampler_padding_samples / stream->pre_resample_channels;

        if (actual_input_frames > 0) {  /* don't bother if nothing to flush. */
            /* This is how many bytes we're expecting without silence appended. */
            int flush_remaining = ((int) ceil(actual_input_frames * stream->rate_incr)) * stream->dst_sample_frame_size;

            #if DEBUG_AUDIOSTREAM
            printf("AUDIOSTREAM: flushing with padding to get max %d bytes!\n", flush_remaining);
            #endif

            memset(stream->staging_buffer + filled, '\0', stream->staging_buffer_size - filled);
            if (SDL_AudioStreamPutInternal(stream, stream->staging_buffer, stream->staging_buffer_size, &flush_remaining) < 0) {
                return -1;
            }

            /* we have flushed out (or initially filled) the pending right-side
               resampler padding, but we need to push more silence to guarantee
               the staging buffer is fully flushed out, too. */
            memset(stream->staging_buffer, '\0', filled);
            if (SDL_AudioStreamPutInternal(stream, stream->staging_buffer, stream->staging_buffer_size, &flush_remaining) < 0) {
                return -1;
            }
        }
    }

    stream->staging_buffer_filled = 0;
    stream->first_run = true;

    return 0;
}

/* get converted/resampled data from the stream */
int SDL_AudioStreamGetEX(SDL_AudioStream *stream, void *buf, int len)
{
    #if DEBUG_AUDIOSTREAM
    printf("AUDIOSTREAM: want to get %d converted bytes\n", len);
    #endif

    if (!stream) {
        return SDL_PrintError("stream");
    } else if (!buf) {
        return SDL_PrintError("buf");
    } else if (len <= 0) {
        return 0;  /* nothing to do. */
    } else if ((len % stream->dst_sample_frame_size) != 0) {
        return SDL_PrintError("Can't request partial sample frames");
    }

    return (int) SDL_ReadFromDataQueue(stream->queue, buf, len);
}

/* number of converted/resampled bytes available */
int
SDL_AudioStreamAvailableEX(SDL_AudioStream *stream)
{
    return stream ? (int) SDL_CountDataQueue(stream->queue) : 0;
}

void
SDL_AudioStreamClearEX(SDL_AudioStream *stream)
{
    if (!stream) {
        SDL_PrintError("stream");
    } else {
        SDL_ClearDataQueue(stream->queue, stream->packetlen * 2);
        if (stream->reset_resampler_func) {
            stream->reset_resampler_func(stream);
        }
        stream->first_run = true;
        stream->staging_buffer_filled = 0;
    }
}

/* dispose of a stream */
void
SDL_FreeAudioStreamEX(SDL_AudioStream *stream)
{
    if (stream) {
        if (stream->cleanup_resampler_func) {
            stream->cleanup_resampler_func(stream);
        }
        SDL_FreeDataQueue(stream->queue);
        free(stream->staging_buffer);
        free(stream->work_buffer_base);
        free(stream->resampler_padding);
        free(stream);
    }
}
