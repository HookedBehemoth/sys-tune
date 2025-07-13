#include "source.hpp"

#include "sdmc/sdmc.hpp"

#include <cstring>

// NOTE: when updating dr_libs, check for TUNE-FIX comment for patches.
#ifdef WANT_FLAC
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_OGG
#define DR_FLAC_NO_STDIO
#include "dr_flac.h"
#endif

#ifdef WANT_MP3
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr_mp3.h"
#endif

#ifdef WANT_WAV
#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "dr_wav.h"
#endif

namespace {

    enum SeekOrigin {
        SeekOrigin_SET,
        SeekOrigin_CUR,
        SeekOrigin_END
    };

    size_t ReadCallback(void *pUserData, void *pBufferOut, size_t bytesToRead) {
        auto data = static_cast<Source *>(pUserData);

        return data->ReadFile(pBufferOut, bytesToRead);
    }

#ifdef WANT_FLAC
    drflac_bool32 FlacSeekCallback(void *pUserData, int offset, drflac_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->SeekFile(offset, origin);
    }

    drflac_bool32 FlacTellCallback(void *pUserData, drflac_int64* pCursor) {
        auto data = static_cast<Source *>(pUserData);

        *pCursor = data->TellFile();
        return true;
    }
#endif

#ifdef WANT_MP3
    drmp3_bool32 Mp3SeekCallback(void *pUserData, int offset, drmp3_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->SeekFile(offset, origin);
    }

    drmp3_bool32 Mp3TellCallback(void *pUserData, drmp3_int64* pCursor) {
        auto data = static_cast<Source *>(pUserData);

        *pCursor = data->TellFile();
        return true;
    }

    void Mp3MetaCallback(void *pUserData, const drmp3_metadata* pMetadata) {
        // stubbed for now, will handle later to load album artwork.
    }
#endif

#ifdef WANT_WAV
    drwav_bool32 WavSeekCallback(void *pUserData, int offset, drwav_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->SeekFile(offset, origin);
    }

    drwav_bool32 WavTellCallback(void *pUserData, drwav_int64* pCursor) {
        auto data = static_cast<Source *>(pUserData);

        *pCursor = data->TellFile();
        return true;
    }
#endif

#ifdef DEBUG
    void *log_malloc(size_t sz, void *) {
        std::printf("malloc: 0x%lX\n", sz);
        return malloc(sz);
    }
    void *log_realloc(void *p, size_t sz, void *) {
        std::printf("realloc: %p, 0x%lX\n", p, sz);
        return realloc(p, sz);
    }
    void log_free(void *p, void *) {
        std::printf("free: %p\n", p);
        free(p);
    }
    constexpr const drflac_allocation_callbacks flac_alloc = {
        .onMalloc  = log_malloc,
        .onRealloc = log_realloc,
        .onFree    = log_free,
    };
    constexpr const drmp3_allocation_callbacks mp3_alloc = {
        .onMalloc  = log_malloc,
        .onRealloc = log_realloc,
        .onFree    = log_free,
    };
    constexpr const drwav_allocation_callbacks wav_alloc = {
        .onMalloc  = log_malloc,
        .onRealloc = log_realloc,
        .onFree    = log_free,
    };
    constexpr const drflac_allocation_callbacks *flac_alloc_ptr = &flac_alloc;
    constexpr const drmp3_allocation_callbacks *mp3_alloc_ptr   = &mp3_alloc;
    constexpr const drwav_allocation_callbacks *wav_alloc_ptr   = &wav_alloc;
#else
#ifdef WANT_FLAC
    constexpr const drflac_allocation_callbacks *flac_alloc_ptr = nullptr;
#endif
#ifdef WANT_MP3
    constexpr const drmp3_allocation_callbacks *mp3_alloc_ptr   = nullptr;
#endif
#ifdef WANT_WAV
    constexpr const drwav_allocation_callbacks *wav_alloc_ptr   = nullptr;
#endif
#endif

}

Source::Source(FsFile &&file) : m_file(file), m_offset(0), m_size(0) {
    file = {};
    if (R_FAILED(fsFileGetSize(&this->m_file, &this->m_size)))
        this->m_size = 0;
}

Source::~Source() {
    fsFileClose(&this->m_file);
    this->m_offset = 0;
    this->m_size   = 0;
}

bool Source::SetupResampler(u32 output_channels, u32 output_sample_rate) {
    m_sdl_stream = UniqueAudioStream{
        SDL_NewAudioStreamEX(
        AUDIO_S16, GetChannelCount(), GetSampleRate(),
        AUDIO_S16, output_channels, output_sample_rate)
    };

    return m_sdl_stream != nullptr;
}

s64 Source::Resample(u8* out, std::size_t size) {
    if (!out || !size) {
        return -1;
    }

    s64 data_read = 0;
    while (size > 0) {
        if (auto sz = SDL_AudioStreamGetEX(m_sdl_stream.get(), out, size); sz != 0) {
            size -= sz;
            out += sz;
            data_read += sz;
        } else {
            const auto dec_got = Decode(m_resample_buffer.size(), m_resample_buffer.data());
            if (dec_got == 0) {
                return data_read;
            }
            if (0 != SDL_AudioStreamPutEX(m_sdl_stream.get(), m_resample_buffer.data(), dec_got)) {
                return -1;
            }
        }
    }

    return data_read;
}

size_t Source::ReadFile(void *buffer, size_t read_size) {
    size_t bytes_read = 0;
    if (R_SUCCEEDED(fsFileRead(&this->m_file, this->m_offset, buffer, read_size, 0, &bytes_read))) {
        this->m_offset += bytes_read;
        return bytes_read;
    } else {
        return 0;
    }
}

bool Source::SeekFile(s64 offset, int origin) {
    s64 new_offset;
    switch (origin) {
        case SeekOrigin_SET:
            new_offset = offset;
            break;
        case SeekOrigin_CUR:
            new_offset = this->m_offset + offset;
            break;
        case SeekOrigin_END:
            new_offset = this->m_size + offset;
            break;
        default:
            return false;
    }

    if (new_offset <= this->m_size) {
        this->m_offset = new_offset;
        return true;
    } else {
        return false;
    }
}

s64 Source::TellFile() {
    return this->m_offset;
}

bool Source::Done() {
    auto [current, total] = this->Tell();

    return current == total;
}

#ifdef WANT_FLAC
class FlacFile final : public Source {
  private:
    drflac *m_flac;

  public:
    FlacFile(FsFile &&file) : Source(std::move(file)) {
        this->m_flac = drflac_open(ReadCallback, FlacSeekCallback, FlacTellCallback, this, flac_alloc_ptr);
    }
    ~FlacFile() {
        if (this->m_flac != nullptr)
            drflac_close(this->m_flac);
    }

    bool IsOpen() override {
        return this->m_flac != nullptr;
    }

    size_t Decode(size_t sample_count, s16 *data) override {
        std::scoped_lock lk(this->m_mutex);

        return GetChannelCount() * sizeof(s16) * drflac_read_pcm_frames_s16(this->m_flac, sample_count / GetChannelCount(), data);
    }

    std::pair<u32, u32> Tell() override {
        std::scoped_lock lk(this->m_mutex);

        return {this->m_flac->currentPCMFrame, this->m_flac->totalPCMFrameCount};
    }

    bool Seek(u64 target) override {
        std::scoped_lock lk(this->m_mutex);

        return drflac_seek_to_pcm_frame(this->m_flac, target);
    }

    int GetSampleRate() override {
        return this->m_flac->sampleRate;
    }

    int GetChannelCount() override {
        return this->m_flac->channels;
    }
};
#endif

#ifdef WANT_MP3
class Mp3File final : public Source {
  private:
    drmp3 m_mp3;
    bool initialized;
    u64 m_total_frame_count;

  public:
    Mp3File(FsFile &&file) : Source(std::move(file)) {
        if (drmp3_init(&this->m_mp3, ReadCallback, Mp3SeekCallback, Mp3TellCallback, Mp3MetaCallback, this, mp3_alloc_ptr)) {
            this->m_total_frame_count = drmp3_get_pcm_frame_count(&this->m_mp3);
            this->initialized         = true;
        }
    }
    ~Mp3File() {
        if (initialized)
            drmp3_uninit(&this->m_mp3);
    }

    bool IsOpen() override {
        return initialized;
    }

    size_t Decode(size_t sample_count, s16 *data) override {
        std::scoped_lock lk(this->m_mutex);

        return GetChannelCount() * sizeof(s16) * drmp3_read_pcm_frames_s16(&this->m_mp3, sample_count / GetChannelCount(), data);
    }

    std::pair<u32, u32> Tell() override {
        std::scoped_lock lk(this->m_mutex);

        return {this->m_mp3.currentPCMFrame, this->m_total_frame_count};
    }

    bool Seek(u64 target) override {
        std::scoped_lock lk(this->m_mutex);

        return drmp3_seek_to_pcm_frame(&this->m_mp3, target);
    }

    int GetSampleRate() override {
        return this->m_mp3.sampleRate;
    }

    int GetChannelCount() override {
        return this->m_mp3.channels;
    }
};
#endif

#ifdef WANT_WAV
class WavFile final : public Source {
  private:
    drwav m_wav;
    bool initialized;
    s32 m_bytes_per_pcm;

  public:
    WavFile(FsFile &&file) : Source(std::move(file)) {
        if (drwav_init(&this->m_wav, ReadCallback, WavSeekCallback, WavTellCallback, this, wav_alloc_ptr)) {
            this->m_bytes_per_pcm = drwav_get_bytes_per_pcm_frame(&this->m_wav);
            this->initialized     = true;
        }
    }
    ~WavFile() {
        if (initialized)
            drwav_uninit(&this->m_wav);
    }

    bool IsOpen() override {
        return initialized;
    }

    size_t Decode(size_t sample_count, s16 *data) override {
        std::scoped_lock lk(this->m_mutex);

        return GetChannelCount() * sizeof(s16) * drwav_read_pcm_frames_s16(&this->m_wav, sample_count / GetChannelCount(), data);
    }

    std::pair<u32, u32> Tell() override {
        std::scoped_lock lk(this->m_mutex);

        u64 byte_position = this->m_wav.dataChunkDataSize - this->m_wav.bytesRemaining;
        return {byte_position / this->m_bytes_per_pcm, this->m_wav.totalPCMFrameCount};
    }

    bool Seek(u64 target) override {
        std::scoped_lock lk(this->m_mutex);

        return drwav_seek_to_pcm_frame(&this->m_wav, target);
    }

    int GetSampleRate() override {
        return this->m_wav.sampleRate;
    }

    int GetChannelCount() override {
        return this->m_wav.channels;
    }
};
#endif

std::unique_ptr<Source> OpenFile(const char *path) {
    const auto type = GetSourceType(path);
    if (type == SourceType::NONE)
        return nullptr;

    FsFile file;

    if (R_FAILED(sdmc::OpenFile(&file, path)))
        return nullptr;

    if (false) {}
#ifdef WANT_MP3
    else if (type == SourceType::MP3) {
        return std::make_unique<Mp3File>(std::move(file));
    }
#endif
#ifdef WANT_FLAC
    else if (type == SourceType::FLAC) {
        return std::make_unique<FlacFile>(std::move(file));
    }
#endif
#ifdef WANT_WAV
    else if (type == SourceType::WAV) {
        return std::make_unique<WavFile>(std::move(file));
    }
#endif
    else {
        fsFileClose(&file);
        return nullptr;
    }
}

SourceType GetSourceType(const char* path) {
    const auto ext = std::strrchr(path, '.');
    if (!ext) {
        return SourceType::NONE;
    }

    if (!strcasecmp(ext, ".mp3")) {
        return SourceType::MP3;
    } else if (!strcasecmp(ext, ".flac")) {
        return SourceType::FLAC;
    } else if (!strcasecmp(ext, ".wav") || !strcasecmp(ext, ".wave")) {
        return SourceType::WAV;
    }

    return SourceType::NONE;
}
