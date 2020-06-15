#pragma once

#include "../symbol.hpp"

#include <tesla.hpp>
#include <tune.h>

class StatusBar : public tsl::elm::Element {
  private:
    bool m_playing;
    TuneRepeatMode m_repeat;
    TuneShuffleMode m_shuffle;
    TuneCurrentStats m_stats;

    float m_percentage;

    std::string_view m_current_track;
    std::string m_scroll_text;
    u32 m_text_width;
    u32 m_scroll_offset;
    bool m_truncated;
    u8 m_counter;

    bool m_touched = false;

  public:
    StatusBar();

    virtual tsl::elm::Element *requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) final;
    virtual void draw(tsl::gfx::Renderer *renderer) final;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) final {}

    void update();
};
