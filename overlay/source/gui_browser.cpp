#include "gui_browser.hpp"

#include "tune.h"

namespace {

    bool ListItemTextCompare(const tsl::elm::ListItem *_lhs, const tsl::elm::ListItem *_rhs) {
        return strcasecmp(_lhs->getText().c_str(), _rhs->getText().c_str()) < 0;
    };

    ALWAYS_INLINE bool EndsWith(const char *name, const char *ext) {
        return strcasecmp(name + std::strlen(name) - std::strlen(ext), ext) == 0;
    }

}
constexpr const char *const base_path = "/music/";

char path_buffer[FS_MAX_PATH];

BrowserGui::BrowserGui()
    : m_fs(), has_music(), cwd("/") {
    this->m_list = new tsl::elm::List();

    /* Open sd card filesystem. */
    Result rc = fsOpenSdCardFileSystem(&this->m_fs);
    if (R_SUCCEEDED(rc)) {
        /* Check if base path /music/ exists. */
        FsDir dir;
        std::strcpy(this->cwd, base_path);
        if (R_SUCCEEDED(fsFsOpenDirectory(&this->m_fs, this->cwd, FsDirOpenMode_ReadFiles, &dir))) {
            this->has_music = true;
            fsDirClose(&dir);
        } else {
            this->cwd[0] = '\0';
        }
        this->scanCwd();
    } else {
        this->m_list->addItem(new tsl::elm::CategoryHeader("Couldn't open SdCard"));
    }
}

BrowserGui::~BrowserGui() {
    fsFsClose(&this->m_fs);
}

tsl::elm::Element *BrowserGui::createUI() {
    auto rootFrame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);

    rootFrame->setContent(this->m_list);

    return rootFrame;
}

void BrowserGui::update() {
}
bool BrowserGui::handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) {
    if (keysDown & KEY_B) {
        if (this->has_music && this->cwd[7] == '\0') {
            return false;
        } else if (this->cwd[1] != '\0') {
            this->upCwd();
            return true;
        }
    }
    return false;
}

void BrowserGui::scanCwd() {
    tsl::Gui::removeFocus();
    this->m_list->clear();

    /* Show absolute folder path. */
    this->m_list->addItem(new tsl::elm::CategoryHeader(this->cwd, true));

    /* Open directory. */
    FsDir dir;
    Result rc = fsFsOpenDirectory(&this->m_fs, this->cwd, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &dir);
    if (R_FAILED(rc)) {
        char result_buffer[0x10];
        std::snprintf(result_buffer, 0x10, "2%03X-%04X", R_MODULE(rc), R_DESCRIPTION(rc));
        this->m_list->addItem(new tsl::elm::ListItem("something went wrong :/"));
        this->m_list->addItem(new tsl::elm::ListItem(result_buffer));
        return;
    }

    std::vector<tsl::elm::ListItem *> folders, files;

    /* Iternate over directory. */
    s64 count = 0;
    FsDirectoryEntry entry;
    while (R_SUCCEEDED(fsDirRead(&dir, &count, 1, &entry)) && count) {
        if (entry.type == FsDirEntryType_Dir) {
            /* Add directory entries. */
            auto *item = new tsl::elm::ListItem(entry.name);
            item->setClickListener([this, item](u64 down) -> bool {
                if (down & KEY_A) {
                    std::sprintf(this->cwd, "%s%s/", this->cwd, item->getText().c_str());
                    this->scanCwd();
                    return true;
                }
                return false;
            });
            folders.push_back(item);
        } else if (EndsWith(entry.name, ".mp3") || EndsWith(entry.name, ".wav") || EndsWith(entry.name, ".wave") || EndsWith(entry.name, ".flac")) {
            /* Add file entry. */
            auto *item = new tsl::elm::ListItem(entry.name);
            item->setClickListener([this, item](u64 down) -> bool {
                if (down & KEY_A) {
                    std::sprintf(path_buffer, "%s%s", this->cwd, item->getText().c_str());
                    tuneEnqueue(path_buffer, TuneEnqueueType_Back);
                    return true;
                }
                return false;
            });
            files.push_back(item);
        }
    }
    if (folders.size() == 0 && files.size() == 0) {
        this->m_list->addItem(new tsl::elm::CategoryHeader("Empty..."));
        return;
    }

    if (folders.size() > 0) {
        std::sort(folders.begin(), folders.end(), ListItemTextCompare);
        for (auto element : folders)
            this->m_list->addItem(element);
    }
    if (files.size() > 0) {
        this->m_list->addItem(new tsl::elm::CategoryHeader("Files"));
        std::sort(files.begin(), files.end(), ListItemTextCompare);
        for (auto element : files)
            this->m_list->addItem(element);
    }
}

void BrowserGui::upCwd() {
    size_t length = std::strlen(this->cwd);
    if (length <= 1)
        return;

    for (size_t i = length - 2; i >= 0; i--) {
        if (this->cwd[i] == '/') {
            this->cwd[i + 1] = '\0';
            this->scanCwd();
            return;
        }
    }
}
