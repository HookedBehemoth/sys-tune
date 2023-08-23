#include "gui_main.hpp"

#include "elm_overlayframe.hpp"
#include "elm_list_item_sv.hpp"
#include "gui_browser.hpp"
#include "gui_playlist.hpp"
#include "i18n/i18n.hpp"

namespace {

    constexpr const size_t num_steps = 20;
    constexpr const float max_volume = 2;

    void volumeCallback(u8 value) {
        float music_volume = (float(value) / num_steps) * max_volume;
        tuneSetVolume(music_volume);
    }

}

MainGui::MainGui() {
    m_status_bar    = new StatusBar();
    m_volume_slider = new tsl::elm::StepTrackBar("\uE13C", num_steps);
    /* Get initial volume. */
    float volume = 0;
    if (R_SUCCEEDED(tuneGetVolume(&volume))) {
        this->m_volume_slider->setProgress((volume / max_volume) * num_steps);
        this->m_volume_slider->setValueChangedListener(volumeCallback);
    } else {
        this->m_volume_slider->setProgress(0);
    }
}

tsl::elm::Element *MainGui::createUI() {
    auto *frame = new SysTuneOverlayFrame();
    auto *list  = new tsl::elm::List();

    /* Current track. */
    list->addItem(this->m_status_bar, tsl::style::ListItemDefaultHeight * 2);

    /* Playlist. */
    auto *queue_button = new ListItemSV("Playlist"_lang());
    queue_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<PlaylistGui>();
            return true;
        }
        return false;
    });
    list->addItem(queue_button);

    /* Browser. */
    auto *browser_button = new ListItemSV("Music browser"_lang());
    browser_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<BrowserGui>();
            return true;
        }
        return false;
    });
    list->addItem(browser_button);

    /* Volume indicator */
    list->addItem(this->m_volume_slider);

    auto *exit_button = new ListItemSV("Close sys-tune"_lang());
    exit_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tuneQuit();
            tsl::goBack();
            return true;
        }
        return false;
    });
    list->addItem(exit_button);

    frame->setContent(list);

    frame->setDescription("\uE0E1  Minimize  \uE0E0  OK"_lang());

    return frame;
}

void MainGui::update() {
    static u8 tick = 0;
    /* Update status 4 times per second. */
    if ((tick % 15) == 0)
        this->m_status_bar->update();
    tick++;
}
