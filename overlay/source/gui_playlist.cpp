#include "gui_playlist.hpp"

#include "elm_overlayframe.hpp"
#include "tune.h"

namespace {

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

    class ButtonListItem : public tsl::elm::ListItem {
      public:
        template <typename Text, typename Value>
        ButtonListItem(Text &text, Value &value) : ListItem(std::forward<Text>(text), std::forward<Value>(value)) {}

        virtual bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
            if (event == tsl::elm::TouchEvent::Touch)
                this->m_touched = this->inBounds(currX, currY);

            if (event == tsl::elm::TouchEvent::Release && this->m_touched) {
                this->m_touched = false;

                if (Element::getInputMode() == tsl::InputMode::Touch) {
                    bool handled = false;
                    if (currX > this->getLeftBound() && currX < (this->getRightBound() - this->getHeight()) && currY > this->getTopBound() && currY < this->getBottomBound())
                        handled = this->onClick(HidNpadButton_A);

                    if (currX > (this->getRightBound() - this->getHeight()) && currX < this->getRightBound() && currY > this->getTopBound() && currY < this->getBottomBound())
                        handled = this->onClick(HidNpadButton_Y);

                    this->m_clickAnimationProgress = 0;
                    return handled;
                }
            }

            return false;
        }
    };

}

PlaylistGui::PlaylistGui() {
    m_list = new tsl::elm::List();

    u32 count;
    Result rc = tuneGetPlaylistSize(&count);
    if (R_FAILED(rc)) {
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

    char path[FS_MAX_PATH];
    for (u32 i = 0; i < count; i++) {
        rc = tuneGetPlaylistItem(i, path, FS_MAX_PATH);
        if (R_FAILED(rc))
            break;

        char *str = path;
        size_t length   = std::strlen(str);
        NullLastDot(str);
        for (size_t i = length; i >= 0; i--) {
            if (str[i] == '/') {
                str = str + i + 1;
                break;
            }
        }
        auto *item = new ButtonListItem(str, "\uE098");
        item->setClickListener([this, item](u64 keys) -> bool {
            u32 index  = this->m_list->getIndexInList(item);
            u8 counter = 0;
            if (keys & HidNpadButton_A) {
                tuneSelect(index);
                counter++;
            }
            if (keys & HidNpadButton_Y) {
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
    }
}

tsl::elm::Element *PlaylistGui::createUI() {
    auto rootFrame = new SysTuneOverlayFrame();

    rootFrame->setContent(this->m_list);
    rootFrame->setDescription("\uE0E1  Back     \uE0E0  OK     \uE0E3  Remove");

    return rootFrame;
}
