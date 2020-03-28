#pragma once

#include "../../ipc/music.h"

#include <tesla.hpp>

class StatusBar : public tsl::elm::Element {
  private:
    MusicPlayerStatus m_status;
    MusicLoopStatus m_loop;
    std::string_view m_current_track;
    std::string m_scroll_text;
    const char *m_progress_text;
    const char *m_total_text;
    double m_percentage;
    u32 m_text_width;
    bool m_truncated;
    u32 m_scroll_offset;
    u8 m_counter;

  public:
    StatusBar(const char *current_track, const char *progress_text, const char *total_text);

    virtual tsl::elm::Element *requestFocus(tsl::elm::Element *oldFocus, tsl::FocusDirection direction) override;
    virtual void draw(tsl::gfx::Renderer *renderer) override;
    virtual void layout(u16 parentX, u16 parentY, u16 parentWidth, u16 parentHeight) override;
    virtual bool onClick(u64 keys) override;

    void update(MusicPlayerStatus status, const char *current_track, double percentage);
};
