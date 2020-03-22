#pragma once
#include <tesla.hpp>

/**
 * @brief A selectable list item that calls a function when the A button gets pressed.
 */
class SelectListItem : public tsl::elm::ListItem {
  private:
    std::function<void(const std::string &text)> m_f;

  public:
    SelectListItem(const std::string &text, std::function<void(const std::string &text)> f)
        : ListItem(text), m_f(f) {}

    virtual ~SelectListItem() {}

    virtual bool onClick(u64 keys) {
        if (keys & KEY_A) {
            m_f(this->m_text);
            return true;
        }

        return false;
    }

    void setCallback(std::function<void(const std::string &text)> f) {
        this->m_f = f;
    }
};