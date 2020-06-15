#pragma once

#include "../symbol.hpp"

#include <tesla.hpp>

class AlphaButton : public tsl::elm::Element {
  public:
    using ButtonCallback = std::function<void(AlphaButton *)>;

  private:
    AlphaSymbol m_symbol;
    tsl::Color m_color;
    bool m_touched = false;
    ButtonCallback m_cb;

  public:
    AlphaButton(const AlphaSymbol &symbol, ButtonCallback cb = nullptr);
    virtual void draw(tsl::gfx::Renderer *renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;
    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override;
    virtual bool onClick(u64 keys) override;
    virtual tsl::elm::Element *requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) override;

    void setColor(tsl::Color color);
    void setSymbol(const AlphaSymbol &symbol);
};
