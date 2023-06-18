#pragma once

#include "tesla.hpp"


class ElmVolume final : public tsl::elm::StepTrackBar {
public:
    ElmVolume(const char icon[3], const std::string& name, size_t numSteps)
        : StepTrackBar{icon, numSteps}, m_name{name} { }

    virtual ~ElmVolume() {}

    virtual void draw(tsl::gfx::Renderer *renderer) override {
        const u16 trackBarWidth = this->getWidth() - 95;
        const u16 stepWidth = trackBarWidth / (this->m_numSteps - 1);

        for (u8 i = 0; i < this->m_numSteps; i++) {
            renderer->drawRect(this->getX() + 60 + stepWidth * i, this->getY() + 50, 1, 10, a(tsl::style::color::ColorFrame));
        }

        const auto [descWidth, descHeight] = renderer->drawString(this->m_name.c_str(), false, 0, 0, 15, tsl::style::color::ColorTransparent);
        renderer->drawString(this->m_name.c_str(), false, ((this->getX() + 60) + (this->getWidth() - 95) / 2) - (descWidth / 2), this->getY() + 20, 15, a(tsl::style::color::ColorDescription));

        StepTrackBar::draw(renderer);
    }

private:
    const std::string m_name;
};
