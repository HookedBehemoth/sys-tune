#include "elm_alpha_button.hpp"

AlphaButton::AlphaButton(const AlphaSymbol &symbol, ButtonCallback cb)
    : Element(), m_symbol(symbol), m_color(tsl::style::color::ColorText), m_cb(cb) {}

void AlphaButton::draw(tsl::gfx::Renderer *renderer) {
    const u8 *ptr      = this->m_symbol.data;
    const s32 x_offset = this->getX() + this->getWidth() / 2 - this->m_symbol.width;
    const s32 y_offset = this->getY() + this->getHeight() / 2 - this->m_symbol.height / 2;
    tsl::Color color   = this->m_color;
    for (s32 y1 = 0; y1 < this->m_symbol.height; y1++) {
        for (s32 x1 = 0; x1 < this->m_symbol.width; x1++) {
            color.a = static_cast<u16>(ptr[0] >> 4);
            renderer->setPixelBlendSrc(x_offset + x1 * 2, y_offset + y1, renderer->a(color));
            color.a = static_cast<u16>(ptr[0] & 0xf);
            renderer->setPixelBlendSrc(x_offset + x1 * 2 + 1, y_offset + y1, renderer->a(color));
            ptr++;
        }
    }
}

void AlphaButton::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {}
bool AlphaButton::onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) {
    if (event == tsl::elm::TouchEvent::Touch) {
        this->m_touched = this->inBounds(currX, currY);
    } else if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
        this->m_touched = false;

        if (Element::getInputMode() == tsl::InputMode::Touch) {
            this->m_cb(this);
            return true;
        }
    }

    return false;
}

bool AlphaButton::onClick(u64 keys) {
    if (keys & KEY_A && this->m_cb) {
        this->m_cb(this);
        return true;
    }
    return false;
}

tsl::elm::Element *AlphaButton::requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) {
    return this;
}

void AlphaButton::setColor(tsl::Color color) {
    this->m_color = color;
}

void AlphaButton::setSymbol(const AlphaSymbol &symbol) {
    this->m_symbol = symbol;
}
