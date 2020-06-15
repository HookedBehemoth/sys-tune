#pragma once

#include <tesla.hpp>

class SeekBar : tsl::elm::Element {
  private:
    bool m_touched   = false;
    float m_progress = 0.0f;

  public:
    virtual void draw(tsl::gfx::Renderer *renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;
    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override;
    virtual bool onClick(u64 keys) override;
    virtual tsl::elm::Element *requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) override;

    void setProgress(float progress);
};
