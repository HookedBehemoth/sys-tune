#include "status_bar.hpp"

#include "symbol.hpp"

#define REPEAT_X

StatusBar::StatusBar(const char *current_track, const char *progress_text, const char *total_text)
    : m_state(), m_current_track(current_track), m_progress_text(progress_text), m_total_text(total_text), m_percentage(0.0), m_text_width(-1), m_truncated(false), m_scroll_offset(0), m_counter(0) {
    if (R_FAILED(tuneGetRepeatMode(&this->m_repeat)))
        this->m_repeat = TuneRepeatMode_Off;
    if (R_FAILED(tuneGetShuffleMode(&this->m_shuffle)))
        this->m_shuffle = TuneShuffleMode_Off;
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

    renderer->drawCircle(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight + 1, 3, true, 0xf00f);
    renderer->drawRect(this->getX() + 15, this->getY() + tsl::style::ListItemDefaultHeight - 2, bar_length * this->m_percentage, 7, 0xf00f);
    renderer->drawCircle(this->getX() + 15 + bar_length * this->m_percentage, this->getY() + tsl::style::ListItemDefaultHeight + 1, 3, true, 0xf00f);

    /* Progress */
    renderer->drawString(this->m_progress_text, false, this->getX() + 15, this->getY() + CenterOfLine(1) + 8, 20, 0xffff);

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
    renderer->drawString(this->m_total_text, false, this->getX() + this->getWidth() - 75, this->getY() + CenterOfLine(1) + 8, 20, 0xffff);

    /* Prev button */
    symbol::prev::symbol.draw(GetPrevX(), GetPrevY(), renderer, tsl::style::color::ColorText);

    /* Current playback glyph */
    this->GetPlaybackSymbol().draw(GetPlayStateX(), GetPlayStateY(), renderer, tsl::style::color::ColorText);

    /* Next button */
    symbol::next::symbol.draw(GetNextX(), GetNextY(), renderer, tsl::style::color::ColorText);
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

            if (handled > 0) {
                this->m_clickAnimationProgress = 0;
                return true;
            }
        }
    }

    return false;
}

void StatusBar::update(AudioOutState state, const char *current_track, double percentage) {
    this->m_state = state;
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

void StatusBar::CycleRepeat() {
    this->m_repeat = TuneRepeatMode((this->m_repeat + 1) % 3);
    tuneSetRepeatMode(this->m_repeat);
}

void StatusBar::CycleShuffle() {
    this->m_shuffle = TuneShuffleMode((this->m_shuffle + 1) % 2);
    tuneSetShuffleMode(this->m_shuffle);
}

void StatusBar::CyclePlay() {
    if (this->m_state == AudioOutState_Stopped) {
        tunePlay();
    } else {
        tunePause();
    }
}

void StatusBar::Prev() {
    tunePrev();
}

void StatusBar::Next() {
    tuneNext();
}

const AlphaSymbol &StatusBar::GetPlaybackSymbol() {
    switch (this->m_state) {
        case AudioOutState_Started:
            return symbol::pause::symbol;
        case AudioOutState_Stopped:
            return symbol::play::symbol;
        default:
            return symbol::stop::symbol;
    }
}
