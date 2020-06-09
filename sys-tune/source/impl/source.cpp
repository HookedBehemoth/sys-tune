#include "source.hpp"

#include "sdmc.hpp"

#include <cstring>
#include <strings.h>

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_OGG
#define DR_FLAC_NO_STDIO
#include "dr_flac.h"

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#define DRMP3_DATA_CHUNK_SIZE 0x4000 * 6
#include "dr_mp3.h"

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "dr_wav.h"

namespace {

    size_t ReadCallback(void *pUserData, void *pBufferOut, size_t bytesToRead) {
        auto data = static_cast<Source *>(pUserData);

        return data->Read(pBufferOut, bytesToRead);
    }

    drflac_bool32 FlacSeekCallback(void *pUserData, int offset, drflac_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, origin == drflac_seek_origin_start);
    }

    drmp3_bool32 Mp3SeekCallback(void *pUserData, int offset, drmp3_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, origin == drmp3_seek_origin_start);
    }

    drwav_bool32 WavSeekCallback(void *pUserData, int offset, drwav_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, origin == drwav_seek_origin_start);
    }

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
    constexpr const drflac_allocation_callbacks *flac_alloc_ptr = nullptr;
    constexpr const drmp3_allocation_callbacks *mp3_alloc_ptr   = nullptr;
    constexpr const drwav_allocation_callbacks *wav_alloc_ptr   = nullptr;
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

size_t Source::Read(void *buffer, size_t read_size) {
    size_t bytes_read = 0;
    if (R_SUCCEEDED(fsFileRead(&this->m_file, this->m_offset, buffer, read_size, 0, &bytes_read))) {
        this->m_offset += bytes_read;
        return bytes_read;
    } else {
        return 0;
    }
}

bool Source::Seek(int offset, bool set) {
    s64 absolute = offset;
    if (!set)
        absolute += this->m_offset;

    if (absolute < this->m_size) {
        this->m_offset = absolute;
        return true;
    } else {
        return false;
    }
}

bool Source::Done() {
    auto [current, total] = this->Tell();

    return current == total;
}

class FlacFile : public Source {
  private:
    drflac *m_flac;

  public:
    FlacFile(FsFile &&file) : Source(std::move(file)) {
        this->m_flac = drflac_open(ReadCallback, FlacSeekCallback, this, flac_alloc_ptr);
    }
    ~FlacFile() {
        if (this->m_flac != nullptr)
            drflac_close(this->m_flac);
    }

    bool IsOpen() {
        return this->m_flac != nullptr;
    }

    size_t Decode(size_t sample_count, s16 *data) {
        std::scoped_lock lk(this->m_mutex);

        return drflac_read_pcm_frames_s16(this->m_flac, sample_count, data);
    }

    std::pair<u32, u32> Tell() {
        std::scoped_lock lk(this->m_mutex);

        return {this->m_flac->currentPCMFrame, this->m_flac->totalPCMFrameCount};
    }

    bool Seek(u64 target) {
        std::scoped_lock lk(this->m_mutex);

        return drflac_seek_to_pcm_frame(this->m_flac, target);
    }

    int GetSampleRate() {
        return this->m_flac->sampleRate;
    }

    int GetChannelCount() {
        return this->m_flac->channels;
    }
};

class Mp3File : public Source {
  private:
    drmp3 m_mp3;
    bool initialized;
    u64 m_total_frame_count;

  public:
    Mp3File(FsFile &&file) : Source(std::move(file)) {
        if (drmp3_init(&this->m_mp3, ReadCallback, Mp3SeekCallback, this, mp3_alloc_ptr)) {
            this->m_total_frame_count = drmp3_get_pcm_frame_count(&this->m_mp3);
            this->initialized         = true;
        }
    }
    ~Mp3File() {
        if (initialized)
            drmp3_uninit(&this->m_mp3);
    }

    bool IsOpen() {
        return initialized;
    }

    size_t Decode(size_t sample_count, s16 *data) {
        std::scoped_lock lk(this->m_mutex);

        return drmp3_read_pcm_frames_s16(&this->m_mp3, sample_count, data);
    }

    std::pair<u32, u32> Tell() {
        std::scoped_lock lk(this->m_mutex);

        return {this->m_mp3.currentPCMFrame, this->m_total_frame_count};
    }

    bool Seek(u64 target) {
        std::scoped_lock lk(this->m_mutex);

        return drmp3_seek_to_pcm_frame(&this->m_mp3, target);
    }

    int GetSampleRate() {
        return this->m_mp3.sampleRate;
    }

    int GetChannelCount() {
        return this->m_mp3.channels;
    }
};

class WavFile : public Source {
  private:
    drwav m_wav;
    bool initialized;
    s32 m_bytes_per_pcm;

  public:
    WavFile(FsFile &&file) : Source(std::move(file)) {
        if (drwav_init(&this->m_wav, ReadCallback, WavSeekCallback, this, wav_alloc_ptr)) {
            this->m_bytes_per_pcm = drwav_get_bytes_per_pcm_frame(&this->m_wav);
            this->initialized     = true;
        }
    }
    ~WavFile() {
        if (initialized)
            drwav_uninit(&this->m_wav);
    }

    bool IsOpen() {
        return initialized;
    }

    size_t Decode(size_t sample_count, s16 *data) {
        std::scoped_lock lk(this->m_mutex);

        return drwav_read_pcm_frames_s16(&this->m_wav, sample_count, data);
    }

    std::pair<u32, u32> Tell() {
        std::scoped_lock lk(this->m_mutex);

        u64 byte_position = this->m_wav.dataChunkDataSize - this->m_wav.bytesRemaining;
        return {byte_position / this->m_bytes_per_pcm, this->m_wav.totalPCMFrameCount};
    }

    bool Seek(u64 target) {
        std::scoped_lock lk(this->m_mutex);

        return drwav_seek_to_pcm_frame(&this->m_wav, target);
    }

    int GetSampleRate() {
        return this->m_wav.sampleRate;
    }

    int GetChannelCount() {
        return this->m_wav.channels;
    }
};

Source *OpenFile(const char *path) {
    size_t length = std::strlen(path);
    if (length < 5)
        return nullptr;

    FsFile file;

    if (R_FAILED(sdmc::OpenFile(&file, path)))
        return nullptr;

    Source *source = nullptr;

    if (false) {}
#ifdef WANT_MP3
    else if (strcasecmp(path + length - 4, ".mp3") == 0) {
        source = new (std::nothrow) Mp3File(std::move(file));
    }
#endif
#ifdef WANT_FLAC
    else if (strcasecmp(path + length - 5, ".flac") == 0) {
        source = new (std::nothrow) FlacFile(std::move(file));
    }
#endif
#ifdef WANT_WAV
    else if (strcasecmp(path + length - 4, ".wav") == 0 || strcasecmp(path + length - 5, ".wave") == 0) {
        source = new (std::nothrow) WavFile(std::move(file));
    }
#endif

    if (source == nullptr)
        fsFileClose(&file);

    return source;
}
