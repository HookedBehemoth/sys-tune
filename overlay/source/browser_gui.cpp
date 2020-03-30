#include "browser_gui.hpp"

#include "../../ipc/music.h"
#include "select_list_item.hpp"

constexpr const char *const base_path = "/music/";

char path_buffer[FS_MAX_PATH];

BrowserGui::BrowserGui()
    : m_fs(), open(), has_music(), cwd("/"), m_flag(false) {
    Result rc = fs::OpenSdCardFileSystem(&this->m_fs);
    if (R_SUCCEEDED(rc)) {
        this->open = true;
        this->m_flag = true;
        IDirectory dir;
        if (R_SUCCEEDED(this->m_fs.OpenDirectoryFormat(&dir, FsDirOpenMode_ReadFiles, base_path))) {
            std::strcpy(this->cwd, base_path);
            this->has_music = true;
        }
    }
    this->m_list = new tsl::elm::List();
}

tsl::elm::Element *BrowserGui::createUI() {
    auto rootFrame = new tsl::elm::OverlayFrame("ovl-tune \u266B", VERSION);

    if (!open) {
        this->m_list->addItem(new tsl::elm::ListItem("Couldn't open SdCard filesystem"));
    }

    rootFrame->setContent(this->m_list);

    return rootFrame;
}

void BrowserGui::update() {
    if (this->m_flag) {
        this->scanCwd();
        this->m_flag = false;
    }
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
    /* Open directory. */
    IDirectory dir;
    Result rc = this->m_fs.OpenDirectory(&dir, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, this->cwd);
    if (R_SUCCEEDED(rc)) {
        /* Iternate over directory. */
        for (const auto &elm : DirectoryIterator(&dir)) {
            /* Add directory entries. */
            if (elm.type == FsDirEntryType_Dir) {
                this->m_list->addItem(new SelectListItem(elm.name, [&](const std::string &text) {
                    std::snprintf(this->cwd, FS_MAX_PATH, "%s%s/", this->cwd, text.c_str());
                    this->m_flag = true;
                }));
            } else if (strcasecmp(elm.name + std::strlen(elm.name) - 4, ".mp3") == 0) {
                this->m_list->addItem(new SelectListItem(elm.name, [&](const std::string &text) {
                    std::snprintf(path_buffer, FS_MAX_PATH, "%s%s", this->cwd, text.c_str());
                    musicAddToQueue(path_buffer);
                }));
            }
        }
    } else {
        this->m_list->addItem(new tsl::elm::ListItem("something went wrong :/"));
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
