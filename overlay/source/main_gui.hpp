#pragma once

#include "../../ipc/tune.h"
#include "status_bar.hpp"

#include <tesla.hpp>

class MainGui : public tsl::Gui {
  private:
    StatusBar *m_status_bar;
    tsl::elm::TrackBar *m_volume_slider;

  public:
    MainGui();

    virtual tsl::elm::Element *createUI() override;

    virtual void update() override;
};
