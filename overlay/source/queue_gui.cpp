#include "queue_gui.hpp"

#include "../../ipc/tune.h"
#include "remove_list_item.hpp"

namespace {

    constexpr const u32 queue_count = 30;
    constexpr const size_t queue_size = 30 * FS_MAX_PATH;
    char queue_buffer[queue_size];

}

QueueGui::QueueGui() {
    m_list = new tsl::elm::List();

    u32 count;
    if (R_SUCCEEDED(tuneGetCurrentPlaylist(&count, queue_buffer, queue_size))) {
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
            auto *item = new RemoveListItem(str);
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
                    }

                    counter++;
                }
                return counter;
            });
            m_list->addItem(item);
            ptr += FS_MAX_PATH;
        }
    }
}

tsl::elm::Element *QueueGui::createUI() {
    auto rootFrame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);

    rootFrame->setContent(this->m_list);

    return rootFrame;
}

void QueueGui::update() {
}

bool QueueGui::handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) {
    return false;
}
