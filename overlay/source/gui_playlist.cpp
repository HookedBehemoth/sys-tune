#include "gui_playlist.hpp"

#include "elm_overlayframe.hpp"
#include "config/config.hpp"
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

    class ButtonListItem final : public tsl::elm::ListItem {
      public:
        template <typename Text, typename Value>
        ButtonListItem(Text &text, Value &value) : ListItem(std::forward<Text>(text), std::forward<Value>(value)) {}

        bool onTouch(tsl::elm::TouchEvent event, s32 currX, s32 currY, s32 prevX, s32 prevY, s32 initialX, s32 initialY) override {
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

    ButtonListItem* g_focus_item;

}

PlaylistGui::PlaylistGui() {
    g_focus_item = nullptr;
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

    char current_path[FS_MAX_PATH];
    TuneCurrentStats current_stats;
    rc = tuneGetCurrentQueueItem(current_path, sizeof(current_path), &current_stats);
    if (R_FAILED(rc)) {
        char result_buffer[0x10];
        std::snprintf(result_buffer, 0x10, "2%03X-%04X", R_MODULE(rc), R_DESCRIPTION(rc));
        this->m_list->addItem(new tsl::elm::ListItem("failed to get current item"));
        this->m_list->addItem(new tsl::elm::ListItem(result_buffer));
        return;
    }

    m_list->addItem(new tsl::elm::CategoryHeader("\uE0E2  To remove all      \uE0E7  Play on start up", true));

    char path[FS_MAX_PATH];
    for (u32 i = 0; i < count; i++) {
        rc = tuneGetPlaylistItem(i, path, sizeof(path));
        if (R_FAILED(rc))
            break;

        bool found = false;
        if (!g_focus_item && !strcasecmp(current_path, path)) {
            found = true;
        }

        char *str = path;
        size_t length   = std::strlen(str);
        NullLastDot(str);
        for (size_t i = length; i >= 0; i--) {
            if (str[i] == '/') {
                str = str + i + 1;
                break;
            }
        }
        auto item = new ButtonListItem(str, "\uE098");
        item->setClickListener([this, item](u64 keys) -> bool {
            // adjust index for above CategoryHeader.
            const auto index = this->m_list->getIndexInList(item);
            const auto tune_index = index - 1;

            if (keys & HidNpadButton_A) {
                tuneSelect(tune_index);
                return true;
            }
            else if (keys & HidNpadButton_Y) {
                if (R_SUCCEEDED(tuneRemove(tune_index))) {
                    this->removeFocus();
                    this->m_list->removeIndex(index);
                    auto element = this->m_list->getItemAtIndex(index + 1);
                    if (element != nullptr) {
                        this->requestFocus(element, tsl::FocusDirection::Down);
                        this->m_list->setFocusedIndex(index + 1);
                    } else if (index > 0) {
                        element = this->m_list->getItemAtIndex(index - 1);
                        this->requestFocus(element, tsl::FocusDirection::Up);
                        this->m_list->setFocusedIndex(index - 1);
                    }
                }
                return true;
            }
            else if (keys & HidNpadButton_X) {
                if (R_SUCCEEDED(tuneClearQueue())) {
                    this->removeFocus();
                    this->m_list->clear();
                    m_list->addItem(new tsl::elm::ListItem("Playlist empty."));
                }
                return true;
            } else if (keys & HidNpadButton_ZR) {
                char path[FS_MAX_PATH];
                if (R_SUCCEEDED(tuneGetPlaylistItem(tune_index, path, sizeof(path)))) {
                    config::set_load_path(path);
                    // todo: toast
                    // m_frame->setToast("Set start up file", item->getText().c_str());
                }
                return true;
            }
            return false;
        });

        if (found) {
            g_focus_item = item;
        }

        m_list->addItem(item);
    }
}

tsl::elm::Element *PlaylistGui::createUI() {
    auto rootFrame = new SysTuneOverlayFrame();

    rootFrame->setContent(this->m_list);
    rootFrame->setDescription("\uE0E1  Back     \uE0E0  Play   \uE0E3  Remove");

    return rootFrame;
}

void PlaylistGui::update()  {
    if (g_focus_item) {
        // wait until its added to the list.
        const auto index = m_list->getIndexInList(g_focus_item);
        if (index >= 0) {
            this->removeFocus();
            this->requestFocus(g_focus_item, tsl::FocusDirection::Down);
            m_list->setFocusedIndex(index);
            g_focus_item = nullptr;
        }
    }
}
