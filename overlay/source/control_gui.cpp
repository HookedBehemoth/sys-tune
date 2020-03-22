#include "control_gui.hpp"

#include "../../ipc/music.h"
#include "browser_gui.hpp"
#include "music_ovl_frame.hpp"

namespace {

    constexpr const char *const status_descriptions[] = {
        "\u25A0",
        "\u25B6",
        " \u0406\u0406",
        "\u25B6\u25B6",
        "\uE098",
    };

    u32 count;
    MusicPlayerStatus status;
    char current_buffer[FS_MAX_PATH];
    const char *current = nullptr;
    const char *status_desc = nullptr;
    constexpr const size_t path_buffer_size = FS_MAX_PATH * 10;
    char path_buffer[path_buffer_size];

    double percentage = 0;
    char progress_text[0x20] = " 0:00/ 0:00";

    double volume = 0;

    const char *paused_desc = "\uE0E0  Play   \uE0E2  Stop  \uE0E3  Select  \uE0D4  Next";
    const char *playing_desc = "\uE0E0 Pause \uE0E2  Stop  \uE0E3  Select  \uE0D4  Next";
    const char *current_desc = paused_desc;

    void FetchStatus() {
        if (R_FAILED(musicGetStatus(&status)) || status >= 5) {
            status_desc = nullptr;
        } else {
            if (status == MusicPlayerStatus_Playing) {
                current_desc = playing_desc;
            } else {
                current_desc = paused_desc;
            }
            status_desc = status_descriptions[status];
        }
    }

    void FetchCurrent() {
        current_buffer[0] = '\0';
        if (R_FAILED(musicGetCurrent(current_buffer, FS_MAX_PATH)) || current_buffer[0] == '\0') {
            current = nullptr;
        } else {
            current = current_buffer;
        }
    }

    void FetchQueue(tsl::elm::List *list) {
        list->clear();
        u32 count;
        if (R_SUCCEEDED(musicGetList(&count, path_buffer, path_buffer_size))) {
            if (count != 0) {
                const char *ptr = path_buffer;
                for (u32 i = 0; i < count; i++) {
                    list->addItem(new tsl::elm::ListItem(ptr));
                    ptr += FS_MAX_PATH;
                }
            } else {
                list->addItem(new tsl::elm::ListItem("Queue empty!"));
            }
        } else {
            list->addItem(new tsl::elm::ListItem("Failed to get queue!"));
        }
    }

    void FetchVolume() {
        double tmp;
        if (R_SUCCEEDED(musicGetVolume(&tmp))) {
            volume = tmp;
        } else {
            volume = 0;
        }
    }

#define MIN(val) (int)val / 60
#define SEC(val) (int)val % 60

    void FetchProgress() {
        double length = 0;
        double progress = 0;
        if (R_SUCCEEDED(musicGetCurrentLength(&length)) && R_SUCCEEDED(musicGetCurrentProgress(&progress))) {
            std::snprintf(progress_text, 0x20, "%2d:%02d/%2d:%02d", MIN(progress), SEC(progress), MIN(length), SEC(length));
            percentage = progress / length;
        } else {
            percentage = 0.0;
        }
    }

}

ControlGui::ControlGui()
    : m_list(5) {}

tsl::elm::Element *ControlGui::createUI() {
    auto rootFrame = new MusicOverlayFrame("Audioplayer \u266B", "v1.0.0", &current_desc);
    this->m_list.setBoundaries(0, 250, tsl::cfg::FramebufferWidth, 470);

    auto *custom = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *drawer, u16 x, u16 y, u16 w, u16 h) {
        drawer->drawString("Control", false, 140, 115, 40, 0xffff);
        if (current) {
            drawer->drawString("Current track:", false, 15, 165, 20, 0xffff);
            drawer->drawString(current, false, 160, 165, 20, 0xffff);
            if (status_desc) {
                drawer->drawString(status_desc, false, 15, 220, 20, 0xffff);
            }
            /* Progress bar */
            u32 bar_length = tsl::cfg::FramebufferWidth - 180;
            drawer->drawRect(50, 212, bar_length, 2, 0xffff);
            drawer->drawRect(50, 210, bar_length * percentage, 6, 0xf00f);
            /* Song length */
            drawer->drawString(progress_text, false, 330, 220, 20, 0xffff);
        }
        /* Volume indicator. */
        drawer->drawString("\uE13C", false, 280, 55, 30, 0xffff);
        drawer->drawRect(315, 42, 100, 2, 0xffff);
        drawer->drawRect(315, 40, 100 * volume, 6, 0xfff0);
        /* Query list */
        m_list.draw(drawer);
    });

    rootFrame->setContent(custom);

    return rootFrame;
}

void ControlGui::update() {
    count++;
    if (count % 15) {
        FetchStatus();
        FetchCurrent();
        FetchQueue(&this->m_list);
        FetchProgress();
        FetchVolume();
        count = 0;
    }
}

bool ControlGui::handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) {
    if (keysDown & KEY_A) {
        MusicPlayerStatus next_status;
        if (status == MusicPlayerStatus_Playing) {
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

    if (keysDown & KEY_DUP) {
        musicSetVolume(volume + 0.05);
    }

    if (keysDown & KEY_DDOWN) {
        musicSetVolume(volume - 0.05);
    }

    if (keysDown & KEY_DRIGHT) {
        musicSetStatus(MusicPlayerStatus_Next);
        return true;
    }

    return false;
}
