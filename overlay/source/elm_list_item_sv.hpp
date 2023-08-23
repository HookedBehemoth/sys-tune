#pragma once

#include <tesla.hpp>

/**
 * @brief A item that goes into a list
 *
 */
class ListItemSV : public tsl::elm::Element {
public:
    /**
     * @brief Constructor
     *
     * @param text Initial description text
     */
    ListItemSV(const std::string_view text)
        : Element(), m_text(text) {
    }
    virtual ~ListItemSV() {}

    virtual void draw(tsl::gfx::Renderer *renderer) override {
        if (this->m_touched && Element::getInputMode() == tsl::InputMode::Touch) {
            renderer->drawRect(ELEMENT_BOUNDS(this), a(tsl::style::color::ColorClickAnimation));
        }

        renderer->drawRect(this->getX(), this->getY(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
        renderer->drawRect(this->getX(), this->getTopBound(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));

        renderer->drawString(this->m_text.data(), false, this->getX() + 20, this->getY() + 45, 23, a(tsl::style::color::ColorText));
    }

    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
        this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight);
    }

    virtual bool onClick(u64 keys) override {
        if (keys & HidNpadButton_A)
            this->triggerClickAnimation();
        else if (keys & (HidNpadButton_AnyUp | HidNpadButton_AnyDown | HidNpadButton_AnyLeft | HidNpadButton_AnyRight))
            this->m_clickAnimationProgress = 0;

        return Element::onClick(keys);
    }


    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
        if (event == tsl::elm::TouchEvent::Touch)
            this->m_touched = this->inBounds(currX, currY);

        if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
            this->m_touched = false;

            if (Element::getInputMode() == tsl::InputMode::Touch) {
                bool handled = this->onClick(HidNpadButton_A);

                this->m_clickAnimationProgress = 0;
                return handled;
            }
        }


        return false;
    }

    virtual tsl::elm::Element* requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) override {
        return this;
    }

    /**
     * @brief Sets the left hand description text of the list item
     *
     * @param text Text
     */
    inline void setText(const std::string_view text) {
        this->m_text = text;
    }

protected:
    std::string_view m_text;

    bool m_touched = false;
};
