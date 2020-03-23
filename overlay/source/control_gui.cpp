#include "control_gui.hpp"

#include "browser_gui.hpp"
#include "music_ovl_frame.hpp"

namespace {

    constexpr const char *const status_descriptions[] = {
        "\u25A0",
        "\u25B6",
        " \u0406\u0406",
        "\u25B6\u0406",
        "\uE098",
    };

    constexpr const char *const paused_desc = "\uE0E0  Play   \uE0E2  Stop  \uE0E3  Select  \uE0E5  Next";
    constexpr const char *const playing_desc = "\uE0E0 Pause \uE0E2  Stop  \uE0E3  Select  \uE0E5  Next";

    char path_buffer[FS_MAX_PATH];

    constexpr u32 queue_size = 6;
    constexpr const size_t queue_buffer_size = FS_MAX_PATH * queue_size;
    char queue_buffer[queue_buffer_size];

}

ControlGui::ControlGui()
    : m_list(queue_size), m_status(MusicPlayerStatus_Stopped), m_current(), m_status_desc(), m_bottom_text(), m_progress_text(" 0:00/ 0:00"), m_percentage(), m_volume(), m_counter() {
    if (R_FAILED(musicGetVolume(&this->m_volume)))
        this->m_volume = 0;
    if (R_FAILED(musicGetLoop(&this->m_loop)))
        this->m_loop = MusicLoopStatus_Off;
}

tsl::elm::Element *ControlGui::createUI() {
    auto rootFrame = new MusicOverlayFrame("Audioplayer \u266B", "v1.0.0", &this->m_bottom_text);
    this->m_list.setBoundaries(0, 200, tsl::cfg::FramebufferWidth, 470);

    auto *custom = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *drawer, u16 x, u16 y, u16 w, u16 h) {
        /* Volume indicator. */
        drawer->drawString("\uE13C", false, 280, 55, 30, 0xffff);
        drawer->drawRect(315, 42, 100, 2, 0xffff);
        drawer->drawRect(315, 40, 100 * this->m_volume, 6, 0xfff0);
        /* Current track and progress bar. */
        if (this->m_current) {
            drawer->drawString(this->m_current, false, 15, 115, 24, 0xffff, 418);
            if (this->m_status_desc) {
                drawer->drawString(this->m_status_desc, false, 15, 170, 20, 0xffff);
            }
            /* Progress bar */
            u32 bar_length = tsl::cfg::FramebufferWidth - 220;
            drawer->drawRect(50, 162, bar_length, 2, 0xffff);
            drawer->drawRect(50, 160, bar_length * this->m_percentage, 6, 0xf00f);
            /* Song length */
            drawer->drawString(this->m_progress_text, false, 290, 170, 20, 0xffff);
        }
        /* Loop indicator */
        auto loop_color = this->m_loop ? 0xfcc0 : 0xfccc;
        drawer->drawString("\uE08E", true, 410, 175, 30, loop_color);
        if (this->m_loop == MusicLoopStatus_Single)
            drawer->drawString("1", true, 419, 167, 12, 0xfff0);
        /* Query list */
        m_list.draw(drawer);
    });

    rootFrame->setContent(custom);

    return rootFrame;
}

void ControlGui::update() {
    if ((this->m_counter % 15) == 0) {
        this->FetchStatus();
        this->FetchCurrentTune();
        this->FetchQueue();
        this->m_counter = 0;
    }
    this->m_counter++;
}

bool ControlGui::handleInput(u64 keysDown, u64 keysHeld, touchPosition, JoystickPosition, JoystickPosition) {
    if (keysDown & KEY_A) {
        MusicPlayerStatus next_status;
        if (this->m_status == MusicPlayerStatus_Playing) {
            next_status = MusicPlayerStatus_Paused;
        } else {
            next_status = MusicPlayerStatus_Playing;
        }
        musicSetStatus(next_status);
        return true;
    }

    if (keysDown & KEY_X) {
        musicSetStatus(MusicPlayerStatus_Stopped);
        return true;
    }

    if (keysDown & KEY_Y) {
        tsl::changeTo<BrowserGui>();
        return true;
    }

    static u32 r_held = 0;
    if (keysHeld & KEY_RIGHT && this->m_volume < 1) {
        r_held++;
        if (r_held >= 10) {
            this->m_volume += 0.05;
            musicSetVolume(this->m_volume);
            r_held = 0;
        }
        return true;
    }

    static u32 l_held = 0;
    if (keysHeld & KEY_LEFT && this->m_volume > 0) {
        l_held++;
        if (l_held >= 10) {
            this->m_volume -= 0.05;
            musicSetVolume(this->m_volume);
            l_held = 0;
        }
        return true;
    }

    if (keysDown & KEY_DUP) {
        this->m_loop = MusicLoopStatus((this->m_loop + 1) % 3);
        musicSetLoop(this->m_loop);
        return true;
    }

    if (keysDown & KEY_DDOWN) {
        musicShuffle();
        return true;
    }

    if (keysDown & KEY_R || keysDown & KEY_ZR) {
        musicSetStatus(MusicPlayerStatus_Next);
        return true;
    }

    return false;
}

void ControlGui::FetchStatus() {
    this->m_status_desc = nullptr;
    if (R_SUCCEEDED(musicGetStatus(&this->m_status)) && this->m_status < 5) {
        if (this->m_status == MusicPlayerStatus_Playing) {
            this->m_bottom_text = playing_desc;
        } else {
            this->m_bottom_text = paused_desc;
        }
        this->m_status_desc = status_descriptions[this->m_status];
    }
}

#define MIN(val) (int)val / 60
#define SEC(val) (int)val % 60

void ControlGui::FetchCurrentTune() {
    this->m_current = nullptr;
    MusicCurrentTune current_tune;
    if (R_SUCCEEDED(musicGetCurrent(path_buffer, FS_MAX_PATH, &current_tune))) {
        this->m_volume = current_tune.volume;
        /* Progress text and bar */
        double total = current_tune.tpf * current_tune.total_frame_count;
        double progress = current_tune.tpf * current_tune.progress_frame_count;
        std::snprintf(this->m_progress_text, 0x20, "%2d:%02d/%2d:%02d", MIN(progress), SEC(progress), MIN(total), SEC(total));
        this->m_percentage = progress / total;
        /* Only show file name. Ignore path to file and extension. */
        size_t length = std::strlen(path_buffer);
        path_buffer[length - 4] = '\0';
        for (size_t i = length; i >= 0; i--) {
            if (path_buffer[i] == '/') {
                this->m_current = path_buffer + i + 1;
                return;
            }
        }
        this->m_current = path_buffer;
    }
}

void ControlGui::FetchQueue() {
    this->m_list.clear();
    u32 count;
    if (R_SUCCEEDED(musicListTunes(&count, queue_buffer, queue_buffer_size))) {
        if (count > 0 && count <= queue_size) {
            /* Only show file name. Ignore path to file and extension. */
            char *ptr = queue_buffer;
            for (u32 i = 0; i < count; i++) {
                const char *str = ptr;
                size_t length = std::strlen(ptr);
                ptr[length - 4] = '\0';
                for (size_t i = length; i >= 0; i--) {
                    if (ptr[i] == '/') {
                        str = ptr + i + 1;
                        break;
                    }
                }
                this->m_list.addItem(new tsl::elm::ListItem(str));
                ptr += FS_MAX_PATH;
            }
        } else {
            this->m_list.addItem(new tsl::elm::ListItem("Queue empty!"));
        }
    } else {
        this->m_list.addItem(new tsl::elm::ListItem("Failed to get queue!"));
    }
}
