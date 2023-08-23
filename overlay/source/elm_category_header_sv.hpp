#pragma once

#include <tesla.hpp>

class CategoryHeaderSV : public tsl::elm::Element {
public:
    CategoryHeaderSV(std::string_view title, bool hasSeparator = false) : m_text(title), m_hasSeparator(hasSeparator) {}
    virtual ~CategoryHeaderSV() {}

    virtual void draw(tsl::gfx::Renderer *renderer) override {
        renderer->drawRect(this->getX() - 2, this->getBottomBound() - 30, 5, 23, a(tsl::style::color::ColorHeaderBar));
        renderer->drawString(this->m_text.data(), false, this->getX() + 13, this->getBottomBound() - 12, 15, a(tsl::style::color::ColorText));

        if (this->m_hasSeparator)
            renderer->drawRect(this->getX(), this->getBottomBound(), this->getWidth(), 1, a(tsl::style::color::ColorFrame));
    }

    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override {
        // Check if the CategoryHeader is part of a list and if it's the first entry in it, half it's height
        if (auto *list = dynamic_cast<tsl::elm::List*>(this->getParent()); list != nullptr) {
            if (list->getIndexInList(this) == 0) {
                this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight / 2);
                return;
            }
        }

        this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight);
    }

    virtual bool onClick(u64 keys) {
        return false;
    }

private:
    std::string_view m_text;
    bool m_hasSeparator;
};
