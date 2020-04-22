#include "main_gui.hpp"

#include "browser_gui.hpp"
#include "playlist_gui.hpp"

namespace {

    constexpr const size_t num_steps = 20;
    constexpr const float max_volume = 2;

    void volumeCallback(u8 value) {
        float music_volume = (float(value) / num_steps) * max_volume;
        tuneSetVolume(music_volume);
    }

}

MainGui::MainGui() {
    m_status_bar = new StatusBar();
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
    auto *frame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);
    auto *list = new tsl::elm::List();

    /* Current track. */
    list->addItem(this->m_status_bar, tsl::style::ListItemDefaultHeight * 2);

    /* Playlist. */
    auto *queue_button = new tsl::elm::ListItem("Playlist");
    queue_button->setClickListener([](u64 keys) {
        if (keys & KEY_A) {
            tsl::changeTo<PlaylistGui>();
            return true;
        }
        return false;
    });
    list->addItem(queue_button);

    /* Browser. */
    auto *browser_button = new tsl::elm::ListItem("Music browser");
    browser_button->setClickListener([](u64 keys) {
        if (keys & KEY_A) {
            tsl::changeTo<BrowserGui>();
            return true;
        }
        return false;
    });
    list->addItem(browser_button);

    /* Volume indicator */
    list->addItem(this->m_volume_slider);

    frame->setContent(list);

    return frame;
}

void MainGui::update() {
    static u8 tick = 0;
    /* Update status 4 times per second. */
    if ((tick % 15) == 0)
        this->m_status_bar->update();
    tick++;
}
