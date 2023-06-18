#pragma once

#include "tune.h"
#include "elm_status_bar.hpp"

#include <tesla.hpp>

class MainGui final : public tsl::Gui {
  private:
    StatusBar *m_status_bar;

  public:
    MainGui();

    tsl::elm::Element *createUI() final;

    void update() final;
};
