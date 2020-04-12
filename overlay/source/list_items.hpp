#pragma once

#include "symbol.hpp"

#include <tesla.hpp>

class ButtonListItem : public tsl::elm::ListItem {
  public:
    template <typename Text, typename Value>
    ButtonListItem(const Text &text, const Value &value) : ListItem(text, value) {}

    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
        if (event == tsl::elm::TouchEvent::Touch)
            this->m_touched = this->inBounds(currX, currY);

        if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
            this->m_touched = false;

            if (Element::getInputMode() == tsl::InputMode::Touch) {
                bool handled = false;
                if (currX > this->getLeftBound() && currX < (this->getRightBound() - this->getHeight()) && currY > this->getTopBound() && currY < this->getBottomBound())
                    handled = this->onClick(KEY_A);

                if (currX > (this->getLeftBound() + this->getHeight()) && currX < this->getRightBound() && currY > this->getTopBound() && currY < this->getBottomBound())
                    handled = this->onClick(KEY_Y);

                this->m_clickAnimationProgress = 0;
                return handled;
            }
        }

        return false;
    }
};
