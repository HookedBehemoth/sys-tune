#pragma once

#include "elements/elm_alpha_button.hpp"

#include <tesla.hpp>
#include <tune.h>

class MainGui final : public tsl::Gui {
  public:
    bool m_playing;
    TuneRepeatMode m_repeat;
    TuneShuffleMode m_shuffle;
    TuneCurrentStats m_stats;
    AlphaButton *repeat, *shuffle, *pause;

  public:
    virtual tsl::elm::Element *createUI() final;

    virtual void update() final;
};
