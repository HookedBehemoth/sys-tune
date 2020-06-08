#pragma once

#include "tune.h"
#include "elm_status_bar.hpp"

#include <tesla.hpp>

class MainGui final : public tsl::Gui {
  private:
    StatusBar *m_status_bar;
    tsl::elm::TrackBar *m_volume_slider;

  public:
    MainGui();

    virtual tsl::elm::Element *createUI() final;

    virtual void update() final;
};
