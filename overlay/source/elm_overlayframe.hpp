#pragma once

#include <tesla.hpp>
#include <memory>
#include <optional>

/**
 * @brief The base frame which can contain another view
 *
 */
class SysTuneOverlayFrame : public tsl::elm::Element {
public:
    /**
     * @brief Constructor
     *
     * @param title Name of the Overlay drawn bolt at the top
     * @param subtitle Subtitle drawn bellow the title e.g version number
     */
    SysTuneOverlayFrame() : tsl::elm::Element() {}
    virtual ~SysTuneOverlayFrame() {}

    virtual void draw(tsl::gfx::Renderer *renderer) override {
        renderer->fillScreen(a(tsl::style::color::ColorFrameBackground));
        renderer->drawRect(tsl::cfg::FramebufferWidth - 1, 0, 1, tsl::cfg::FramebufferHeight, a(0xF222));

        renderer->drawString("ovl-tune \u266B", false, 20, 50, 30, a(tsl::style::color::ColorText));
        renderer->drawString(VERSION, false, 20, 70, 15, a(tsl::style::color::ColorDescription));

        renderer->drawRect(15, tsl::cfg::FramebufferHeight - 73, tsl::cfg::FramebufferWidth - 30, 1, a(tsl::style::color::ColorText));

        renderer->drawString(m_description, false, 30, 693, 23, a(tsl::style::color::ColorText));

        if (m_contentElement != nullptr)
            m_contentElement->frame(renderer);

        if (m_toast) {
            s32 width = tsl::cfg::FramebufferWidth - 20;
            s32 height = 110;
            s32 startX = 10;
            s32 startY = tsl::cfg::FramebufferHeight - height;
            
            u32 fadeDuration = 10;
            if (m_toast->Current < fadeDuration) {
                s32 offset = height - (height / fadeDuration) * m_toast->Current;
                startY += offset;
            }
            if (m_toast->Duration - m_toast->Current < fadeDuration) {
                s32 offset = height - (height / fadeDuration) * (m_toast->Duration - m_toast->Current);
                startY += offset;
            }

            renderer->drawRect(startX, startY, width, height, a(tsl::style::color::ColorText));
            renderer->drawRect(startX + 3, startY + 3, width - 6, height - 6, a(tsl::style::color::ColorFrameBackground));
            renderer->drawString(m_toast->Header.c_str(), false, startX + 10, startY + 40, 26, a(tsl::style::color::ColorText));
            renderer->drawString(m_toast->Content.c_str(), false, startX + 10, startY + 80, 23, a(tsl::style::color::ColorText), width - 20);

            ++m_toast->Current;
            if (m_toast->Duration <= m_toast->Current) m_toast = std::nullopt;
        }
    }

    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
        setBoundaries(parentX, parentY, parentWidth, parentHeight);

        if (m_contentElement != nullptr) {
            m_contentElement->setBoundaries(parentX + 35, parentY + 125, parentWidth - 85, parentHeight - 73 - 125);
            m_contentElement->invalidate();
        }
    }

    virtual Element* requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) override {
        if (m_contentElement != nullptr)
            return m_contentElement->requestFocus(oldFocus, direction);
        else
            return nullptr;
    }

    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
        // Discard touches outside bounds
        if (!m_contentElement->inBounds(currX, currY))
            return false;

        if (m_contentElement != nullptr)
            return m_contentElement->onTouch(event, currX, currY, prevX, prevY, initialX, initialY);
        else return false;
    }

    /**
     * @brief Sets the content of the frame
     *
     * @param content Element
     */
    void setContent(tsl::elm::Element *content) {
        m_contentElement = std::unique_ptr<tsl::elm::Element>(content);

        if (content != nullptr) {
            m_contentElement->setParent(this);
            this->invalidate();
        }
    }

    void setDescription(const char *description) {
        m_description = description;
    }

    struct Toast {
        std::string Header;
        std::string Content;
        u32 Duration;
        u32 Current;
    };

    void setToast(std::string header, std::string content) {
        m_toast = Toast { header, content, 150, 0 };
    }

private:
    std::unique_ptr<tsl::elm::Element> m_contentElement;
    const char *m_description = "\uE0E1  Back     \uE0E0  OK";
    
    std::optional<Toast> m_toast;
};
