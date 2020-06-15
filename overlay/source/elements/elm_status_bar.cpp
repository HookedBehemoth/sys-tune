#include "elm_status_bar.hpp"

#include "../symbol.hpp"

namespace {

    char path_buffer[FS_MAX_PATH] = "";
    char current_buffer[0x20] = "";
    char total_buffer[0x20] = "";

    void NullLastDot(char *str) {
        char *end = str + strlen(str) - 1;
        while (str != end) {
            if (*end == '.') {
                *end = '\0';
                return;
            }
            end--;
        }
    }

}

StatusBar::StatusBar() {
    if (R_FAILED(tuneGetRepeatMode(&this->m_repeat)))
        this->m_repeat = TuneRepeatMode_Off;
    if (R_FAILED(tuneGetShuffleMode(&this->m_shuffle)))
        this->m_shuffle = TuneShuffleMode_Off;
    if (R_FAILED(tuneGetCurrentQueueItem(path_buffer, FS_MAX_PATH, &this->m_stats))) {
        path_buffer[0] = '\0';
        this->m_stats = {};
    }
}

tsl::elm::Element *StatusBar::requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) {
    return this;
}

void StatusBar::draw(tsl::gfx::Renderer *renderer) {
    if (this->m_touched && Element::getInputMode() == tsl::InputMode::Touch) {
        renderer->drawRect(ELEMENT_BOUNDS(this), a(tsl::style::color::ColorClickAnimation));
    }

    if (this->m_text_width == 0) {
        /* Get base width. */
        auto [width, height] = renderer->drawString(this->m_current_track.data(), false, 0, 0, 26, tsl::style::color::ColorTransparent);
        this->m_truncated = static_cast<s32>(width) > (this->getWidth() - 30);
        if (this->m_truncated) {
            /* Get width with spacing. */
            this->m_scroll_text = std::string(m_current_track).append("       ");
            auto [ex_width, ex_height] = renderer->drawString(this->m_scroll_text.c_str(), false, 0, 0, 26, tsl::style::color::ColorTransparent);
            this->m_scroll_text.append(m_current_track);
            this->m_text_width = ex_width;
        } else {
            this->m_text_width = width;
        }
    }

    /* Current track. */
    if (this->m_truncated) {
        renderer->drawString(this->m_scroll_text.c_str(), false, this->getX() + 15 - this->m_scroll_offset, this->getY() + 40, 26, tsl::style::color::ColorText);
        if (this->m_counter == 120) {
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
        renderer->drawString(this->m_current_track.data(), false, this->getX() + 15, this->getY() + 40, 26, tsl::style::color::ColorText);
    }

    /* Seek bar. */
    u32 bar_length = this->getWidth() - 30;
    renderer->drawRect(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight, bar_length, 3, 0xffff);

    if (this->m_percentage > 0) {
        renderer->drawCircle(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight + 1, 3, true, 0xf00f);
        renderer->drawRect(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight - 2, bar_length * this->m_percentage, 7, 0xf00f);
        renderer->drawCircle(this->getX() + 15 + bar_length * this->m_percentage, this->getY() + tsl::style::ListItemDefaultHeight + 1, 3, true, 0xf00f);
    }

    /* Progress */
    renderer->drawString(current_buffer, false, this->getX() + 15, this->getY() + 100 + 8, 20, 0xffff);
}

void StatusBar::update() {
    if (R_FAILED(tuneGetStatus(&this->m_playing)))
        this->m_playing = false;

    if (R_SUCCEEDED(tuneGetCurrentQueueItem(path_buffer, FS_MAX_PATH, &this->m_stats))) {
        /* Only show file name. Ignore path to file and extension. */
        size_t length = std::strlen(path_buffer);
        NullLastDot(path_buffer);
        for (size_t i = length; i >= 0; i--) {
            if (path_buffer[i] == '/') {
                if (this->m_current_track != path_buffer + i + 1) {
                    this->m_current_track = path_buffer + i + 1;
                    this->m_text_width = 0;
                    this->m_scroll_offset = 0;
                    this->m_counter = 0;
                }
                break;
            }
        }
    } else {
        this->m_current_track = "Stopped!";
        this->m_stats = {};
        /* Reset scrolling text */
        this->m_text_width = 0;
        this->m_scroll_offset = 0;
        this->m_counter = 0;
    }
    /* Progress text and bar */
    u32 current = this->m_stats.current_frame / this->m_stats.sample_rate;
    u32 total = this->m_stats.total_frames / this->m_stats.sample_rate;
    this->m_percentage = std::clamp(float(this->m_stats.current_frame) / float(this->m_stats.total_frames), 0.0f, 1.0f);

    std::snprintf(current_buffer, sizeof(current_buffer), "%d:%02d", current / 60, current % 60);
    std::snprintf(total_buffer, sizeof(total_buffer), "%d:%02d", total / 60, total % 60);
}
