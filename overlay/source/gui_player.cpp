#include "gui_player.hpp"

#include "elements/elm_omni_layout.hpp"
#include "gui_browser.hpp"
#include "gui_playlist.hpp"
#include "symbol.hpp"

#include <tune.h>

namespace {

    constexpr const size_t VolumeStepCount = 20;
    constexpr const float VolumeMax        = 2.0f;

    char path_buffer[FS_MAX_PATH] = "";
    char current_buffer[0x20] = "";
    char total_buffer[0x20] = "";

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

    void volumeCallback(u8 value) {
        float music_volume = (float(value) / VolumeStepCount) * VolumeMax;
        tuneSetVolume(music_volume);
    }

    void updateRepeat(AlphaButton *btn, TuneRepeatMode mode) {
        switch (mode) {
            case TuneRepeatMode_Off:
                btn->setSymbol(symbol::RepeatAll);
                btn->setColor(tsl::style::color::ColorHeaderBar);
                break;
            case TuneRepeatMode_One:
                btn->setSymbol(symbol::RepeatOne);
                btn->setColor(tsl::style::color::ColorHighlight);
                break;
            case TuneRepeatMode_All:
                btn->setSymbol(symbol::RepeatAll);
                btn->setColor(tsl::style::color::ColorHighlight);
                break;
        }
    }

    void updateShuffle(AlphaButton *btn, TuneShuffleMode mode) {
        switch (mode) {
            case TuneShuffleMode_Off:
                btn->setColor(tsl::style::color::ColorHeaderBar);
                break;
            case TuneShuffleMode_On:
                btn->setColor(tsl::style::color::ColorHighlight);
                break;
        }
    }

}

tsl::elm::Element *MainGui::createUI() {
    auto *frame  = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);
    auto *layout = new OmniDirectionalLayout();

    //auto *backward = new AlphaButton(symbol::Backward, [stats = &this->m_stats](AlphaButton *btn) {
    //    tuneSeek(std::max(s64(stats->current_frame) - s64(stats->total_frames / 10), s64(0)));
    //});
    //auto *forward = new AlphaButton(symbol::Forward, [stats = &this->m_stats](AlphaButton *btn) {
    //    tuneSeek(std::min(stats->current_frame + stats->total_frames / 10, stats->total_frames));
    //});

    repeat = new AlphaButton(symbol::RepeatAll, [mode = &this->m_repeat](AlphaButton *btn) {
        if (R_SUCCEEDED(tuneSetRepeatMode(*mode = static_cast<TuneRepeatMode>((*mode + 1) % 3))))
            updateRepeat(btn, *mode);
    });

    if (R_SUCCEEDED(tuneGetRepeatMode(&this->m_repeat)))
        updateRepeat(repeat, this->m_repeat);

    shuffle = new AlphaButton(symbol::Shuffle, [mode = &this->m_shuffle](AlphaButton *btn) {
        if (R_SUCCEEDED(tuneSetShuffleMode(*mode = static_cast<TuneShuffleMode>((*mode + 1) % 2))))
            updateShuffle(btn, *mode);
    });

    if (R_SUCCEEDED(tuneGetShuffleMode(&this->m_shuffle)))
        updateRepeat(repeat, this->m_repeat);

    auto *prev  = new AlphaButton(symbol::Prev, [](AlphaButton *) { tunePrev(); });
    auto *next  = new AlphaButton(symbol::Next, [](AlphaButton *) { tuneNext(); });
    pause = new AlphaButton(symbol::Pause, [playing = &this->m_playing](AlphaButton *btn) {
        if (*playing) {
            if (R_SUCCEEDED(tunePause())) {
                btn->setSymbol(symbol::Play);
                *playing = false;
            }
        } else {
            if (R_SUCCEEDED(tunePlay())) {
                btn->setSymbol(symbol::Pause);
                *playing = true;
            }
        }
    });

    repeat->setBoundaries(120, 130, 100, 60);
    shuffle->setBoundaries(228, 130, 100, 60);

    prev->setBoundaries(70, 200, 100, 60);
    next->setBoundaries(278, 200, 100, 60);
    pause->setBoundaries(174, 200, 100, 60);

    /* Playlist. */
    auto *queue_button = new tsl::elm::ListItem("Playlist");
    queue_button->setClickListener([](u64 keys) {
        if (keys & KEY_A) {
            tsl::changeTo<PlaylistGui>();
            return true;
        }
        return false;
    });

    /* Browser. */
    auto *browser_button = new tsl::elm::ListItem("Music browser");
    browser_button->setClickListener([](u64 keys) {
        if (keys & KEY_A) {
            tsl::changeTo<BrowserGui>();
            return true;
        }
        return false;
    });

    /* Volume indicator */
    auto *volume_slider = new tsl::elm::StepTrackBar("\uE13C", VolumeStepCount);

    /* Get initial volume. */
    float volume = 0;
    tuneGetVolume(&volume);
    volume_slider->setProgress((volume / VolumeMax) * VolumeStepCount);
    volume_slider->setValueChangedListener(volumeCallback);

    auto *exit_button = new tsl::elm::ListItem("Close sys-tune");
    exit_button->setClickListener([](u64 keys) {
        if (keys & KEY_A) {
            tuneQuit();
            tsl::goBack();
            return true;
        }
        return false;
    });

    queue_button->setBoundaries(30, 270, 388, 70);
    browser_button->setBoundaries(30, 340, 388, 70);
    volume_slider->setBoundaries(30, 410, 388, 90);
    exit_button->setBoundaries(30, 500, 388, 70);

    layout->addElements({repeat, shuffle, prev, next, pause, queue_button, browser_button, volume_slider, exit_button});

    frame->setContent(layout);

    return frame;
}

void MainGui::update() {
    static u8 tick = 0;
    /* Update status 4 times per second. */
    if ((tick % 15) == 0) {
        if (R_FAILED(tuneGetStatus(&this->m_playing)))
            this->m_playing = false;

        if (R_SUCCEEDED(tuneGetCurrentQueueItem(path_buffer, FS_MAX_PATH, &this->m_stats))) {
            /* Only show file name. Ignore path to file and extension. */
            size_t length = std::strlen(path_buffer);
            NullLastDot(path_buffer);
            for (size_t i = length; i >= 0; i--) {
                if (path_buffer[i] == '/') {
                    if (this->m_current_track != path_buffer + i + 1) {
                        this->m_current_track = path_buffer + i + 1;
                        this->m_text_width = 0;
                        this->m_scroll_offset = 0;
                        this->m_counter = 0;
                    }
                    break;
                }
            }
        } else {
            this->m_current_track = "Stopped!";
            this->m_stats = {};
            /* Reset scrolling text */
            this->m_text_width = 0;
            this->m_scroll_offset = 0;
            this->m_counter = 0;
        }
        /* Progress text and bar */
        u32 current = this->m_stats.current_frame / this->m_stats.sample_rate;
        u32 total = this->m_stats.total_frames / this->m_stats.sample_rate;
        this->m_percentage = std::clamp(float(this->m_stats.current_frame) / float(this->m_stats.total_frames), 0.0f, 1.0f);

        std::snprintf(current_buffer, sizeof(current_buffer), "%d:%02d", current / 60, current % 60);
        std::snprintf(total_buffer, sizeof(total_buffer), "%d:%02d", total / 60, total % 60);
    }
    tick++;
}
