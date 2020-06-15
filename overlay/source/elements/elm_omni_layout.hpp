#pragma once

#include <tesla.hpp>

struct DirectionReference {
    tsl::elm::Element *up    = nullptr;
    tsl::elm::Element *down  = nullptr;
    tsl::elm::Element *left  = nullptr;
    tsl::elm::Element *right = nullptr;
};

class OmniDirectionalLayout : public tsl::elm::Element {
  private:
    std::vector<std::pair<std::unique_ptr<tsl::elm::Element>, DirectionReference>> m_elements;

  public:
    void addElements(std::initializer_list<tsl::elm::Element *> elements);

    virtual tsl::elm::Element *requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) final;
    virtual void draw(tsl::gfx::Renderer *renderer) final;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) final;
    virtual bool onClick(u64 keys) final;
    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) final;
};
