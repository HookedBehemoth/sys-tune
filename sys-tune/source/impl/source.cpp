#include "source.hpp"

#include "nx_hwopus.hpp"
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

    size_t DrReadCallback(void *pUserData, void *pBufferOut, size_t bytesToRead) {
        auto data = static_cast<Source *>(pUserData);

        return data->Read(pBufferOut, bytesToRead);
    }

    drflac_bool32 FlacSeekCallback(void *pUserData, int offset, drflac_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, origin);
    }

    drmp3_bool32 Mp3SeekCallback(void *pUserData, int offset, drmp3_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, origin);
    }

    drwav_bool32 WavSeekCallback(void *pUserData, int offset, drwav_seek_origin origin) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, origin);
    }

    int OpusReadCallback(void *pUserData, unsigned char *pBufferOut, int bytesToRead) {
        auto data = static_cast<Source *>(pUserData);

        return data->Read(pBufferOut, bytesToRead);
    }

    int OpusSeekCallback(void *pUserData, opus_int64 offset, int whence) {
        auto data = static_cast<Source *>(pUserData);

        return data->Seek(offset, whence);
    }

    opus_int64 OpusTellCallback(void *pUserData) {
        auto data = static_cast<Source *>(pUserData);

        return data->m_offset;
    }

    constexpr const OpusFileCallbacks opus_ioctl{
        .read  = OpusReadCallback,
        .seek  = OpusSeekCallback,
        .tell  = OpusTellCallback,
        .close = nullptr,
    };

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

Source::Source(FsFile &&file) : m_file(file) {
    log = fopen("sdmc:/log", "w+a");
    file = {};
    if (R_FAILED(fsFileGetSize(&this->m_file, &this->m_size)))
        this->m_size = 0;
}

Source::~Source() {
    if (log)
        fclose(log);
    fsFileClose(&this->m_file);
    this->m_offset = 0;
    this->m_size   = 0;
}

size_t Source::Read(void *buffer, size_t read_size) {
    size_t bytes_read = 0;
    Result rc = fsFileRead(&this->m_file, this->m_offset, buffer, read_size, 0, &bytes_read);
    if (R_SUCCEEDED(rc)) {
        this->m_offset += bytes_read;
        return bytes_read;
    } else {
        fprintf(log, "read: 0x%x\n", rc);
        return 0;
    }
}

bool Source::Seek(int offset, int whence) {
    s64 absolute = 0;

    switch (whence) {
        case SEEK_SET:
            absolute = offset;
            break;
        case SEEK_CUR:
            absolute = this->m_offset + offset;
            break;
        case SEEK_END:
            absolute = this->m_size - (offset + 1);
            break;
        default:
            fprintf(log, "whence: %d\n", whence);
            return false;
    }

    if (absolute >= 0 && absolute < this->m_size) {
        this->m_offset = absolute;
        return true;
    } else {
        fprintf(log, "seek: %ld/%ld\n", absolute, this->m_size);
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
        this->m_flac = drflac_open(DrReadCallback, FlacSeekCallback, this, flac_alloc_ptr);
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
        if (drmp3_init(&this->m_mp3, DrReadCallback, Mp3SeekCallback, this, mp3_alloc_ptr)) {
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
        if (drwav_init(&this->m_wav, DrReadCallback, WavSeekCallback, this, wav_alloc_ptr)) {
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

class OpusFile : public Source {
  private:
    OggOpusFile *m_opus;
    HwopusDecoder decoder;
    bool has_memory;
    Result decoder_rc;
    int m_channel_count;

  public:
    OpusFile(FsFile &&file) : Source(std::move(file)) {
        int err = 0;
        this->m_opus = op_open_callbacks(this, &opus_ioctl, nullptr, 0, &err);
        if (this->m_opus != nullptr) {
            op_set_decode_callback(this->m_opus, hwopus::OpusDecodeCallback, &this->decoder);
            this->m_channel_count = op_channel_count(this->m_opus, -1);
        } else {
            fprintf(log, "open: %d\n", err);
        }
        has_memory = hwopus::Allocate();
        smInitialize();
        decoder_rc = hwopusDecoderInitialize(&this->decoder, this->GetSampleRate(), this->GetChannelCount());
        smExit();
        fprintf(this->log, "opus: %p, mem: %d, hw: 0x%x\n", m_opus, has_memory, decoder_rc);
    }

    ~OpusFile() {
        if (this->m_opus != nullptr)
            op_free(this->m_opus);
        if (has_memory)
            hwopus::Free();
        if (R_SUCCEEDED(decoder_rc))
            hwopusDecoderExit(&this->decoder);
    }

    bool IsOpen() {
        return (this->m_opus != nullptr) && has_memory && (R_SUCCEEDED(decoder_rc));
    }

    size_t Decode(size_t sample_count, s16 *data) {
        std::scoped_lock lk(this->m_mutex);

        size_t buffer_size = sample_count * this->m_channel_count;
        int opret = op_read(this->m_opus, data, buffer_size, nullptr);

        if (opret < 0) {
            fprintf(log, "op_read: %d\n", opret);
            opret = 0;
        }

        return opret;
    }

    std::pair<u32, u32> Tell() {
        std::scoped_lock lk(this->m_mutex);

        return {op_pcm_tell(this->m_opus), op_pcm_total(this->m_opus, -1)};
    }

    bool Seek(u64 target) {
        std::scoped_lock lk(this->m_mutex);

        return op_pcm_seek(this->m_opus, target);
    }

    int GetSampleRate() {
        return 48'000;
    }

    int GetChannelCount() {
        return this->m_channel_count;
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
#ifdef WANT_OPUS
    else if (strcasecmp(path + length - 5, ".opus") == 0) {
        source = new (std::nothrow) OpusFile(std::move(file));
    }
#endif

    if (source == nullptr)
        fsFileClose(&file);

    return source;
}
