#include "gui_main.hpp"

#include "elm_overlayframe.hpp"
#include "elm_volume.hpp"
#include "gui_browser.hpp"
#include "gui_playlist.hpp"
#include "pm/pm.hpp"
#include "config/config.hpp"

namespace {
    constexpr const size_t num_steps = 20;
}

MainGui::MainGui() {
    m_status_bar    = new StatusBar();
}

tsl::elm::Element *MainGui::createUI() {
    auto frame = new SysTuneOverlayFrame();
    auto list  = new tsl::elm::List();

    u64 pid{}, tid{};
    pm::getCurrentPidTid(pid, tid);

    /* Current track. */
    list->addItem(this->m_status_bar, tsl::style::ListItemDefaultHeight * 2);

    /* Playlist. */
    auto queue_button = new tsl::elm::ListItem("Playlist");
    queue_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<PlaylistGui>();
            return true;
        }
        return false;
    });
    list->addItem(queue_button);

    /* Browser. */
    auto browser_button = new tsl::elm::ListItem("Music browser");
    browser_button->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<BrowserGui>();
            return true;
        }
        return false;
    });
    list->addItem(browser_button);

    /* Volume indicator */
    list->addItem(new tsl::elm::CategoryHeader("Volume Control"));

    /* Get initial volume. */
    float tune_volume = 1.f;
    float title_volume = 1.f;
    float default_title_volume = 1.f;

    tuneGetVolume(&tune_volume);
    tuneGetTitleVolume(&title_volume);
    tuneGetDefaultTitleVolume(&default_title_volume);

    auto tune_volume_slider = new ElmVolume("\uE13C", "Tune Volume", num_steps);
    tune_volume_slider->setProgress(tune_volume * num_steps);
    tune_volume_slider->setValueChangedListener([](u8 value){
        const float volume = float(value) / float(num_steps);
        tuneSetVolume(volume);
    });
    list->addItem(tune_volume_slider);

    // empty pid means we are qlaunch :)
    if (tid && pid) {
        auto title_volume_slider = new ElmVolume("\uE13C", "Game Volume", num_steps);
        title_volume_slider->setProgress(title_volume * num_steps);
        title_volume_slider->setValueChangedListener([tid](u8 value){
            const float volume = float(value) / float(num_steps);
            tuneSetTitleVolume(volume);
            config::set_title_volume(tid, volume);
        });
        list->addItem(title_volume_slider);
    }

    auto default_title_volume_slider = new ElmVolume("\uE13C", "Game Volume (default)", num_steps);
    default_title_volume_slider->setProgress(default_title_volume * num_steps);
    default_title_volume_slider->setValueChangedListener([](u8 value){
        const float volume = float(value) / float(num_steps);
        tuneSetDefaultTitleVolume(volume);
    });
    list->addItem(default_title_volume_slider);

    list->addItem(new tsl::elm::CategoryHeader("Play / Pause"));

    /* Per title tune toggle. */
    auto tune_play = new tsl::elm::ToggleListItem("Tune", config::get_title_enabled(tid), "Play", "Pause");
    tune_play->setStateChangedListener([tid](bool new_value) {
        config::set_title_enabled(tid, new_value);
        if (new_value) {
            tunePlay();
        } else {
            tunePause();
        }
    });
    list->addItem(tune_play);

    /* Default title tune toggle. */
    auto tune_default_play = new tsl::elm::ToggleListItem("Tune (default)", config::get_title_enabled_default(), "Play", "Pause");
    tune_default_play->setStateChangedListener([](bool new_value) {
        config::set_title_enabled_default(new_value);
        if (new_value) {
            tunePlay();
        } else {
            tunePause();
        }
    });
    list->addItem(tune_default_play);

    list->addItem(new tsl::elm::CategoryHeader("Misc"));

    auto exit_button = new tsl::elm::ListItem("Close sys-tune");
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
