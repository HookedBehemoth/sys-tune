#pragma once

#include "../../ipc/music.h"
#include "music_ovl_frame.hpp"

#include <tesla.hpp>

class ControlGui : public tsl::Gui {
  private:
    MusicOverlayFrame *m_frame;
    tsl::elm::List m_list;
    MusicPlayerStatus m_status;
    MusicLoopStatus m_loop;
    const char *m_current;
    const char *m_status_desc;
    char m_progress_text[0x20];
    double m_percentage;
    double m_volume;
    u32 m_counter;

  public:
    ControlGui();

    virtual tsl::elm::Element *createUI() override;

    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition, JoystickPosition, JoystickPosition) override;

  private:
    void FetchStatus();
    void FetchCurrentTune();
    void FetchQueue();
    void FetchVolume();
};
