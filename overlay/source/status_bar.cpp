#include "status_bar.hpp"

#include "symbol.hpp"

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

StatusBar::StatusBar()
    : m_playing(false), m_percentage(0.0), m_text_width(), m_truncated(false), m_scroll_offset(0), m_counter(0) {
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
    renderer->drawString(current_buffer, false, this->getX() + 15, this->getY() + CenterOfLine(1) + 8, 20, 0xffff);

    /* Repeat indicator */
    auto repeat_color = this->m_repeat ? tsl::style::color::ColorHighlight : tsl::style::color::ColorHeaderBar;
    if (this->m_repeat == TuneRepeatMode_One) {
        symbol::repeat::one::symbol.draw(GetRepeatX(), GetRepeatY(), renderer, repeat_color);
    } else {
        symbol::repeat::all::symbol.draw(GetRepeatX(), GetRepeatY(), renderer, repeat_color);
    }

    /* Shuffle indicator */
    auto shuffle_color = this->m_shuffle ? tsl::style::color::ColorHighlight : tsl::style::color::ColorHeaderBar;
    symbol::shuffle::symbol.draw(GetShuffleX(), GetShuffleY(), renderer, shuffle_color);

    /* Song length */
    renderer->drawString(total_buffer, false, this->getX() + this->getWidth() - 75, this->getY() + CenterOfLine(1) + 8, 20, 0xffff);

    /* Backward button */
    symbol::backward::symbol.draw(GetBackwardX(), GetBackwardY(), renderer, tsl::style::color::ColorText);

    /* Prev button */
    symbol::prev::symbol.draw(GetPrevX(), GetPrevY(), renderer, tsl::style::color::ColorText);

    /* Current playback glyph */
    this->GetPlaybackSymbol().draw(GetPlayStateX(), GetPlayStateY(), renderer, tsl::style::color::ColorText);

    /* Next button */
    symbol::next::symbol.draw(GetNextX(), GetNextY(), renderer, tsl::style::color::ColorText);

    /* Forward button */
    symbol::forward::symbol.draw(GetForwardX(), GetForwardY(), renderer, tsl::style::color::ColorText);
}

void StatusBar::layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) {
    this->setBoundaries(this->getX(), this->getY(), this->getWidth(), tsl::style::ListItemDefaultHeight * 3);
}

bool StatusBar::onClick(u64 keys) {
    u8 handled = 0;
    if (keys & KEY_A) {
        this->CyclePlay();
        handled++;
    }
    if (keys & KEY_X) {
        this->CycleRepeat();
        handled++;
    }
    if (keys & KEY_Y) {
        this->CycleShuffle();
        handled++;
    }
    if (keys & KEY_RIGHT) {
        this->Next();
        handled++;
    }
    if (keys & KEY_LEFT) {
        this->Prev();
        handled++;
    }
    if (keys & KEY_ZL) {
        this->Backward();
        handled++;
    }
    if (keys & KEY_ZR) {
        this->Forward();
        handled++;
    }
    return handled;
}

#define TOUCHED(button) (currX > (Get##button##X() - 30) && currX < (Get##button##X() + 30) && prevY > (Get##button##Y() - 30) && prevY < (Get##button##Y() + 30))

bool StatusBar::onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) {
    if (event == tsl::elm::TouchEvent::Touch)
        this->m_touched = this->inBounds(currX, currY);

    if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
        this->m_touched = false;

        if (Element::getInputMode() == tsl::InputMode::Touch) {
            u16 handled = 0;
            if (TOUCHED(Repeat)) {
                this->CycleRepeat();
                handled++;
            }
            if (TOUCHED(Shuffle)) {
                this->CycleShuffle();
                handled++;
            }
            if (TOUCHED(PlayState)) {
                this->CyclePlay();
                handled++;
            }
            if (TOUCHED(Prev)) {
                this->Prev();
                handled++;
            }
            if (TOUCHED(Next)) {
                this->Next();
                handled++;
            }
            if (TOUCHED(Forward)) {
                this->Forward();
                handled++;
            }
            if (TOUCHED(Backward)) {
                this->Backward();
                handled++;
            }

            if (handled > 0) {
                this->m_clickAnimationProgress = 0;
                return true;
            }
        }
    }

    return false;
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
    this->m_percentage = double(this->m_stats.current_frame) / double(this->m_stats.total_frames);

    std::snprintf(current_buffer, sizeof(current_buffer), "%d:%02d", current / 60, current % 60);
    std::snprintf(total_buffer, sizeof(total_buffer), "%d:%02d", total / 60, total % 60);
}

void StatusBar::CycleRepeat() {
    this->m_repeat = TuneRepeatMode((this->m_repeat + 1) % 3);
    tuneSetRepeatMode(this->m_repeat);
}

void StatusBar::CycleShuffle() {
    this->m_shuffle = TuneShuffleMode((this->m_shuffle + 1) % 2);
    tuneSetShuffleMode(this->m_shuffle);
}

void StatusBar::CyclePlay() {
    if (this->m_playing) {
        tunePause();
    } else {
        tunePlay();
    }
}

void StatusBar::Prev() {
    tunePrev();
}

void StatusBar::Next() {
    tuneNext();
}

void StatusBar::Forward() {
    u32 next = std::min(this->m_stats.current_frame + (this->m_stats.total_frames / 10), this->m_stats.total_frames);
    tuneSeek(next);
}

void StatusBar::Backward() {
    u32 next = std::max(s64(this->m_stats.current_frame) - s64(this->m_stats.total_frames / 10), s64(0));
    tuneSeek(next);
}

const AlphaSymbol &StatusBar::GetPlaybackSymbol() {
    return this->m_playing ? symbol::pause::symbol : symbol::play::symbol;
}
