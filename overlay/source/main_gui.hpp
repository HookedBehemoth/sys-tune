#pragma once

#include "../../ipc/music.h"
#include "status_bar.hpp"

#include <tesla.hpp>

class MainGui : public tsl::Gui {
  private:
    char m_progress_text[0x10];
    char m_total_text[0x10];
    StatusBar *m_status_bar;
    tsl::elm::TrackBar *m_volume_slider;

  public:
    MainGui();

    virtual tsl::elm::Element *createUI() override;

    virtual void update() override;
};
