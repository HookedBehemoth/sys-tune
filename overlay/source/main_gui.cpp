#include "main_gui.hpp"

#include "browser_gui.hpp"
#include "queue_gui.hpp"

namespace {

    char path_buffer[FS_MAX_PATH] = "";
    constexpr const size_t num_steps = 20;

    void volumeCallback(u8 value) {
        double music_volume = double(value) / num_steps;
        musicSetVolume(music_volume);
    }

}

MainGui::MainGui() : m_progress_text(" 0:00"), m_total_text(" 0:00") {
    m_status_bar = new StatusBar(path_buffer, this->m_progress_text, this->m_total_text);
    m_volume_slider = new tsl::elm::StepTrackBar("\uE13C", num_steps);
    /* Get initial volume. */
    double volume = 0;
    musicGetVolume(&volume);
    this->m_volume_slider->setProgress(volume * 100);
    this->m_volume_slider->setValueChangedListener(volumeCallback);
}

tsl::elm::Element *MainGui::createUI() {
    auto *frame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);
    auto *list = new tsl::elm::List();

    /* Current track. */
    list->addItem(this->m_status_bar, tsl::style::ListItemDefaultHeight * 2);

    /* Queue. */
    auto *queue_button = new tsl::elm::ListItem("Current Queue");
    queue_button->setClickListener([](u64 keys) {
        if (keys & KEY_A) {
            tsl::changeTo<QueueGui>();
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

#define MIN(val) (int)val / 60
#define SEC(val) (int)val % 60

void MainGui::update() {
    static u8 counter = 0;
    if ((counter % 15) == 0) {
        MusicPlayerStatus status = MusicPlayerStatus_Stopped;
        musicGetStatus(&status);
        MusicCurrentTune tune;
        if (R_SUCCEEDED(musicGetCurrent(path_buffer, FS_MAX_PATH, &tune))) {
            /* Progress text and bar */
            double total = tune.tpf * tune.total_frame_count;
            double progress = tune.tpf * tune.progress_frame_count;
            std::snprintf(this->m_progress_text, 0x10, "%2d:%02d", MIN(progress), SEC(progress));
            std::snprintf(this->m_total_text, 0x10, "%2d:%02d", MIN(total), SEC(total));
            double percentage = progress / total;
            /* Only show file name. Ignore path to file and extension. */
            size_t length = std::strlen(path_buffer);
            path_buffer[length - 4] = '\0';
            for (size_t i = length; i >= 0; i--) {
                if (path_buffer[i] == '/') {
                    this->m_status_bar->update(status, path_buffer + i + 1, percentage);
                    counter = 1;
                    return;
                }
            }
            this->m_status_bar->update(status, path_buffer, percentage);
        } else {
            this->m_status_bar->update(status, nullptr, 0);
        }
        counter = 0;
    }
    counter++;
}
