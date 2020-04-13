#include "playlist_gui.hpp"

#include "../../ipc/tune.h"
#include "list_items.hpp"

namespace {

    constexpr const u32 queue_count = 30;
    constexpr const size_t queue_size = 30 * FS_MAX_PATH;
    char queue_buffer[queue_size];

}

PlaylistGui::PlaylistGui() {
    m_list = new tsl::elm::List();

    u32 count;
    Result rc = tuneGetCurrentPlaylist(&count, queue_buffer, queue_size);
    if (R_FAILED(rc)){
        char result_buffer[0x10];
        std::snprintf(result_buffer, 0x10, "2%03X-%04X", R_MODULE(rc), R_DESCRIPTION(rc));
        this->m_list->addItem(new tsl::elm::ListItem("something went wrong :/"));
        this->m_list->addItem(new tsl::elm::ListItem(result_buffer));
        return;
    }

    if (count == 0) {
        m_list->addItem(new tsl::elm::ListItem("Playlist empty."));
        return;
    }

    char *ptr = queue_buffer;
    for (u32 i = 0; i < std::min(count, queue_count); i++) {
        const char *str = ptr;
        size_t length = std::strlen(ptr);
        ptr[length - 4] = '\0';
        for (size_t i = length; i >= 0; i--) {
            if (ptr[i] == '/') {
                str = ptr + i + 1;
                break;
            }
        }
        auto *item = new ButtonListItem(str, "\uE098");
        item->setClickListener([this, item](u64 keys) -> bool {
            u32 index = this->m_list->getIndexInList(item);
            u8 counter = 0;
            if (keys & KEY_A) {
                tuneSelect(index);
                counter++;
            }
            if (keys & KEY_Y) {
                if (R_SUCCEEDED(tuneRemove(index))) {
                    this->removeFocus();
                    this->m_list->removeIndex(index);
                    auto *element = this->m_list->getItemAtIndex(index + 1);
                    if (element != nullptr) {
                        this->requestFocus(element, tsl::FocusDirection::Down);
                        this->m_list->setFocusedIndex(index + 1);
                    } else if (index > 0) {
                        element = this->m_list->getItemAtIndex(index - 1);
                        this->requestFocus(element, tsl::FocusDirection::Up);
                        this->m_list->setFocusedIndex(index - 1);
                    }
                }

                counter++;
            }
            return counter;
        });
        m_list->addItem(item);
        ptr += FS_MAX_PATH;
    }
}

tsl::elm::Element *PlaylistGui::createUI() {
    auto rootFrame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);

    rootFrame->setContent(this->m_list);

    return rootFrame;
}

void PlaylistGui::update() {
}

bool PlaylistGui::handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) {
    return false;
}
