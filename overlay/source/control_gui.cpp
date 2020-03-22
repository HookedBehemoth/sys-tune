#include "control_gui.hpp"

#include "../../ipc/music.h"
#include "music_ovl_frame.hpp"

namespace {

    constexpr const char *const status_descriptions[] = {
        "\u25A0",
        "\u25B6",
        "\u2590\u2590",
        "\u25B6\u25B6",
        "\uE150",
    };

    u32 count;
    MusicPlayerStatus status;
    char current_buffer[0x40];
    const char *current = nullptr;
    const char *status_desc = nullptr;
    constexpr const size_t path_buffer_size = FS_MAX_PATH * 10;
    char path_buffer[path_buffer_size];

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
        if (R_FAILED(musicGetCurrent(current_buffer, 0x40)) || current_buffer[0] == '\0') {
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

}

ControlGui::ControlGui()
    : m_list(5) {}

tsl::elm::Element *ControlGui::createUI() {
    auto rootFrame = new MusicOverlayFrame("Audioplayer \u266B", "v1.0.0", &current_desc);
    this->m_list.setBoundaries(0, 250, tsl::cfg::LayerWidth, 470);

    auto *custom = new tsl::elm::CustomDrawer([&](tsl::gfx::Renderer *drawer, u16 x, u16 y, u16 w, u16 h) {
        drawer->drawString("Control", false, 140, 115, 40, 0xffff);
        if (current) {
            drawer->drawString("Current track:", false, 15, 165, 20, 0xffff);
            drawer->drawString(current, false, 160, 165, 20, 0xffff);
            if (status_desc) {
                drawer->drawString(status_desc, false, 15, 220, 20, 0xffff);
            }
            /* Progress bar */
            drawer->drawRect(50, 212, tsl::cfg::FramebufferWidth - 180, 2, 0xffff);
            drawer->drawRect(50, 210, 100, 6, 0xf00f);
            /* Song length */
            drawer->drawString("1:30/2:32", false, 330, 220, 20, 0xffff);
        }
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
        musicAddToQueue("/MyPatch.mp3");
        return true;
    }

    if (keysDown & KEY_DRIGHT) {
        musicSetStatus(MusicPlayerStatus_Next);
        return true;
    }

    return false;
}
