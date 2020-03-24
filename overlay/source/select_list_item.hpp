#pragma once
#include <tesla.hpp>

/**
 * @brief A selectable list item that calls a function when the A button gets pressed.
 */
class SelectListItem : public tsl::elm::ListItem {
  private:
    std::function<void(const std::string &text)> m_f;
    u8 m_selectFactor;

  public:
    SelectListItem(const std::string &text, std::function<void(const std::string &text)> f)
        : ListItem(text), m_f(f), m_selectFactor() {}

    virtual ~SelectListItem() {}

    void drawSelection(tsl::gfx::Renderer *renderer) {
        renderer->drawRect(this->getX(), this->getY(), this->getWidth(), this->getHeight(), a({0x0, m_selectFactor, m_selectFactor, 0xf}));
        --m_selectFactor;
    }

    virtual void frame(tsl::gfx::Renderer *renderer) override {
        if (this->m_focused)
            this->drawHighlight(renderer);

        if (this->m_selectFactor)
            this->drawSelection(renderer);

        this->draw(renderer);
    }

    virtual bool onClick(u64 keys) {
        if (keys & KEY_A) {
            m_selectFactor = 0xf;
            m_f(this->m_text);
            return true;
        }

        return false;
    }

    void setCallback(std::function<void(const std::string &text)> f) {
        this->m_f = f;
    }
};
