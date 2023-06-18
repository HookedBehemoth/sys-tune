#pragma once

#include <nxExt.h>
#include <memory>

// number of kb to allocate for mp3 chunk
#define MP3_CHUNK_SIZE_KB 96

class Source {
  private:
    FsFile m_file = {};
    s64 m_offset = 0;
    s64 m_size = 0;

  protected:
    LockableMutex m_mutex;

  public:
    Source(FsFile &&file);
    virtual ~Source();

    size_t Read(void *buffer, size_t read_size);
    bool Seek(int offset, bool set);

    virtual bool IsOpen() = 0;
    virtual size_t Decode(size_t sample_count, s16 *data) = 0;
    virtual std::pair<u32, u32> Tell() = 0;
    virtual bool Seek(u64 target) = 0;

    bool Done();

    virtual int GetSampleRate() = 0;
    virtual int GetChannelCount() = 0;
};

std::unique_ptr<Source> OpenFile(const char *path);
