#include "gui_main.hpp"

#include "elm_overlayframe.hpp"
#include "gui_browser.hpp"
#include "gui_playlist.hpp"
#include "pm/pm.hpp"
#include "config/config.hpp"

namespace {

    constexpr const size_t num_steps = 20;
    constexpr const float max_volume = 2;

    void volumeCallback(u8 value) {
        float music_volume = (float(value) / num_steps) * max_volume;
        config::set_volume(value);
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
    u64 tid{};
    pm::PollCurrentTitle(tid);
    const auto config_option = config::get_title(tid, false);
    const auto default_option = config::get_title_default();

    char id_buf[21]{};
    std::sprintf(id_buf, "%016lx", tid);

    /* Current track. */
    list->addItem(this->m_status_bar, tsl::style::ListItemDefaultHeight * 2);

    /* Playlist. */
    auto *queue_button = new tsl::elm::ListItem("Playlist");
    queue_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<PlaylistGui>();
            return true;
        }
        return false;
    });
    list->addItem(queue_button);

    /* Browser. */
    auto *browser_button = new tsl::elm::ListItem("Music browser");
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

    /* Title Config. */
    auto title_config_item = new tsl::elm::ToggleListItem(std::string{""} + id_buf, config_option, "Play", "Pause");
    title_config_item->setStateChangedListener([tid](bool new_value) {
        config::set_title(tid, new_value);
        if (new_value) {
            tunePlay();
        } else {
            tunePause();
        }
    });
    list->addItem(title_config_item);

    /* Default Config. */
    auto default_config_item = new tsl::elm::ToggleListItem("Global default", default_option, "Play", "Pause");
    default_config_item->setStateChangedListener([](bool new_value) {
        config::set_title_default(new_value);
    });
    list->addItem(default_config_item);

    auto *exit_button = new tsl::elm::ListItem("Close sys-tune");
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

    return frame;
}

void MainGui::update() {
    static u8 tick = 0;
    /* Update status 4 times per second. */
    if ((tick % 15) == 0)
        this->m_status_bar->update();
    tick++;
}
