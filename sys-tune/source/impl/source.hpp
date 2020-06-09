#pragma once

#include <nxExt.h>

class Source {
  private:
    FsFile m_file = {};

  public:
    s64 m_offset = 0;
    s64 m_size   = 0;

  protected:
    LockableMutex m_mutex;
    FILE *log = nullptr;

  public:
    Source(FsFile &&file);
    virtual ~Source();

    size_t Read(void *buffer, size_t read_size);
    bool Seek(int offset, int whence);

    virtual bool IsOpen()                                 = 0;
    virtual size_t Decode(size_t sample_count, s16 *data) = 0;
    virtual std::pair<u32, u32> Tell()                    = 0;
    virtual bool Seek(u64 target)                         = 0;

    bool Done();

    virtual int GetSampleRate()   = 0;
    virtual int GetChannelCount() = 0;
};

Source *OpenFile(const char *path);
