#pragma once

#include "../../ipc/tune.h"
#include "symbol.hpp"

#include <tesla.hpp>

class StatusBar : public tsl::elm::Element {
  private:
    AudioOutState m_state;
    TuneRepeatMode m_repeat;
    TuneShuffleMode m_shuffle;
    std::string_view m_current_track;
    std::string m_scroll_text;

    const char *m_progress_text;
    const char *m_total_text;
    double m_percentage;

    u32 m_text_width;
    bool m_truncated;
    u32 m_scroll_offset;
    u8 m_counter;

    bool m_touched = false;

  public:
    StatusBar(const char *current_track, const char *progress_text, const char *total_text);

    virtual tsl::elm::Element *requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) override;
    virtual void draw(tsl::gfx::Renderer *renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;
    virtual bool onClick(u64 keys) override;
    virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override;

    void update(AudioOutState state, const char *current_track, double percentage);
    void CycleRepeat();
    void CyclePlay();
    void CycleShuffle();
    void Prev();
    void Next();

  private:
    inline ALWAYS_INLINE constexpr s32 CenterOfLine(u8 line) {
        return (tsl::style::ListItemDefaultHeight * line) + (tsl::style::ListItemDefaultHeight / 2);
    }
    inline ALWAYS_INLINE s32 GetRepeatX() {
        return this->getX() + (this->getWidth() / 2) - 70;
    }
    inline ALWAYS_INLINE s32 GetRepeatY() {
        return this->getY() + CenterOfLine(1);
    }
    inline ALWAYS_INLINE s32 GetShuffleX() {
        return this->getX() + (this->getWidth() / 2) + 70;
    }
    inline ALWAYS_INLINE s32 GetShuffleY() {
        return this->getY() + CenterOfLine(1);
    }
    inline ALWAYS_INLINE s32 GetPlayStateX() {
        return this->getX() + (this->getWidth() / 2);
    }
    inline ALWAYS_INLINE s32 GetPlayStateY() {
        return this->getY() + CenterOfLine(2);
    }
    inline ALWAYS_INLINE s32 GetPrevX() {
        return this->getX() + ((this->getWidth() / 4) * 1);
    }
    inline ALWAYS_INLINE s32 GetPrevY() {
        return this->getY() + CenterOfLine(2);
    }
    inline ALWAYS_INLINE s32 GetNextX() {
        return this->getX() + ((this->getWidth() / 4) * 3);
    }
    inline ALWAYS_INLINE s32 GetNextY() {
        return this->getY() + CenterOfLine(2);
    }
    const AlphaSymbol &GetPlaybackSymbol();
};
