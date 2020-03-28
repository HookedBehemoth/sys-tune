#include "status_bar.hpp"

namespace {

    constexpr const char *const status_descriptions[] = {
        "\u25A0",
        "\u25B6",
        " \u0406\u0406",
        "\u25B6\u0406",
        "\uE098",
    };

    const char *GetGlyph(MusicPlayerStatus status) {
        if (status < 0 || status > 4)
            return "???";
        return status_descriptions[status];
    }

}

StatusBar::StatusBar(const char *current_track, const char *progress_text, const char *total_text)
    : m_status(), m_current_track(current_track), m_progress_text(progress_text), m_total_text(total_text), m_percentage(0.0), m_text_width(-1), m_truncated(false), m_scroll_offset(0), m_counter(0) {
    if (R_FAILED(musicGetLoop(&this->m_loop)))
        this->m_loop = MusicLoopStatus_Off;
}

tsl::elm::Element *StatusBar::requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) {
    return this;
}

void StatusBar::draw(tsl::gfx::Renderer *renderer) {
    if (this->m_text_width == 0) {
        /* Get base width. */
        auto [width, height] = renderer->drawString(this->m_current_track.data(), false, 0, 0, 26, tsl::style::color::ColorTransparent);
        this->m_truncated = width > (this->getWidth() - 30);
        if (this->m_truncated) {
            /* Get width with spacing. */
            this->m_scroll_text = std::string(m_current_track).append("       ").append(m_current_track);
            this->m_text_width = width + 100;
        } else {
            this->m_text_width = width;
        }
    }
    /* Current track. */
    if (this->m_truncated) {
        renderer->drawString(this->m_scroll_text.c_str(), false, this->getX() + 15 - this->m_scroll_offset, this->getY() + 40, 26, tsl::style::color::ColorText);
        if (this->m_counter > 60) {
            if (this->m_scroll_offset == this->m_text_width) {
                this->m_scroll_offset = 0;
                this->m_counter = 0;
            } else {
                this->m_scroll_offset++;
            }
        } else {
            this->m_counter++;
        }
    } else {
        renderer->drawString(this->m_current_track.data(), false, this->getX() + 20, this->getY() + 40, 26, tsl::style::color::ColorText);
    }
    /* Seek bar. */
    u32 bar_length = this->getWidth() - 30;
    renderer->drawRect(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight + 2, bar_length, 3, 0xffff);

    renderer->drawCircle(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight + 3, 3, true, 0xf00f);
    renderer->drawRect(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight, bar_length * this->m_percentage, 7, 0xf00f);
    renderer->drawCircle(this->getX() + 15 + bar_length * this->m_percentage, this->getY() + tsl::style::ListItemDefaultHeight + 3, 3, true, 0xf00f);
    /* Progress and song length */
    renderer->drawString(this->m_progress_text, false, this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight + 35, 20, 0xffff);
    renderer->drawString(GetGlyph(this->m_status), false, this->getX() + (this->getWidth() / 2) - 10, this->getY() + tsl::style::ListItemDefaultHeight + 35, 20, tsl::style::color::ColorText);
    renderer->drawString(this->m_total_text, false, this->getX() + this->getWidth() - 75, this->getY() + tsl::style::ListItemDefaultHeight + 35, 20, 0xffff);
    /* Loop indicator */
    auto loop_color = this->m_loop ? 0xfcc0 : 0xfccc;
    renderer->drawString("\uE08E", true, 407, 175, 30, loop_color);
    if (this->m_loop == MusicLoopStatus_Single)
        renderer->drawString("1", true, 419, 167, 12, 0xfff0);
}

void StatusBar::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight + 50);
}

bool StatusBar::onClick(u64 keys) {
    u8 counter = 0;
    if (keys & KEY_A) {
        if (this->m_status == MusicPlayerStatus_Playing) {
            musicSetStatus(MusicPlayerStatus_Paused);
        } else {
            musicSetStatus(MusicPlayerStatus_Playing);
        }
        counter++;
    }
    if (keys & KEY_Y) {
        musicSetStatus(MusicPlayerStatus_Stopped);
        counter++;
    }
    if (keys & KEY_X) {
        this->m_loop = MusicLoopStatus((this->m_loop + 1) % 3);
        musicSetLoop(this->m_loop);
    }
    if (keys & KEY_RIGHT) {
        musicSetStatus(MusicPlayerStatus_Next);
        counter++;
    }
    if (keys & KEY_LEFT) {
        /* TODO: prev */
        counter++;
    }
    return counter & 1;
}

void StatusBar::update(MusicPlayerStatus status, const char *current_track, double percentage) {
    this->m_status = status;
    if (current_track == nullptr) {
        this->m_current_track = "Not playing anything.";
        this->m_text_width = 0;
        this->m_scroll_offset = 0;
        this->m_counter = 0;
    } else if (this->m_current_track != current_track) {
        this->m_current_track = current_track;
        this->m_text_width = 0;
        this->m_scroll_offset = 0;
        this->m_counter = 0;
    }
    this->m_percentage = percentage;
}
