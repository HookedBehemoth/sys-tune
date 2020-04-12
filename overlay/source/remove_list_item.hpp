#pragma once

#include "symbol.hpp"

#include <tesla.hpp>

class RemoveListItem : public tsl::elm::ListItem {
  public:
    RemoveListItem(const std::string &text) : ListItem(text, "\uE098") {}

    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
        if (event == tsl::elm::TouchEvent::Touch)
            this->m_touched = currX > ELEMENT_LEFT_BOUND(this) && currX < ELEMENT_RIGHT_BOUND(this) && currY > ELEMENT_TOP_BOUND(this) && currY < ELEMENT_BOTTOM_BOUND(this);

        if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
            this->m_touched = false;

            if (Element::getInputMode() == tsl::InputMode::Touch) {
                bool handled = false;
                if (currX > ELEMENT_LEFT_BOUND(this) && currX < (ELEMENT_RIGHT_BOUND(this) - this->getHeight()) && currY > ELEMENT_TOP_BOUND(this) && currY < ELEMENT_BOTTOM_BOUND(this))
                    handled = this->onClick(KEY_A);

                if (currX > (ELEMENT_LEFT_BOUND(this) + this->getHeight()) && currX < ELEMENT_RIGHT_BOUND(this) && currY > ELEMENT_TOP_BOUND(this) && currY < ELEMENT_BOTTOM_BOUND(this))
                    handled = this->onClick(KEY_Y);

                this->m_clickAnimationProgress = 0;
                return handled;
            }
        }

        return false;
    }
};
