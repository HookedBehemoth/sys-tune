#include "browser_gui.hpp"

#include "../../ipc/music.h"
#include "music_ovl_frame.hpp"
#include "select_list_item.hpp"

constexpr const char *const base_path = "/music/";

const char *description = "   \uE0E1  Back     \uE0E0  Add";
char path_buffer[FS_MAX_PATH];

BrowserGui::BrowserGui()
    : open(), cwd("/") {
    FsFileSystem fs;
    Result rc = fsOpenSdCardFileSystem(&fs);
    if (R_SUCCEEDED(rc)) {
        m_fs = IFileSystem(std::move(fs));
        open = true;
        IDirectory dir;
        if (R_SUCCEEDED(m_fs.OpenDirectoryFormat(&dir, FsDirOpenMode_ReadFiles, base_path))) {
            std::strcpy(this->cwd, base_path);
        }
    }
    m_list = new tsl::elm::List(7);
}

tsl::elm::Element *BrowserGui::createUI() {
    auto rootFrame = new MusicOverlayFrame("Audioplayer \u266B", "v1.0.0", &description);

    if (open) {
        this->scanCwd();
    } else {
        m_list->addItem(new tsl::elm::ListItem("Couldn't open SdCard filesystem"));
    }

    rootFrame->setContent(m_list);

    return rootFrame;
}

void BrowserGui::update() {}
bool BrowserGui::handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) {
    if (keysDown & KEY_B && this->cwd[1] != '\0') {
        this->upCwd();
        return true;
    }
    return false;
}

void BrowserGui::scanCwd() {
    tsl::Gui::removeFocus();
    m_list->clear();
    /* Open directory. */
    IDirectory dir;
    Result rc = m_fs.OpenDirectory(&dir, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, this->cwd);
    if (R_SUCCEEDED(rc)) {
        /* Iternate over directory. */
        for (const auto &elm : DirectoryIterator(&dir)) {
            /* Add directory entries. */
            if (elm.type == FsDirEntryType_Dir) {
                m_list->addItem(new SelectListItem(elm.name, [&](const std::string &text) {
                    std::snprintf(this->cwd, FS_MAX_PATH, "%s%s/", this->cwd, text.c_str());
                    scanCwd();
                }));
            } else if (strcasecmp(elm.name + std::strlen(elm.name) - 4, ".mp3") == 0) {
                m_list->addItem(new SelectListItem(elm.name, [&](const std::string &text) {
                    std::snprintf(path_buffer, FS_MAX_PATH, "%s%s", this->cwd, text.c_str());
                    musicAddToQueue(path_buffer);
                }));
            }
        }
    } else {
        m_list->addItem(new tsl::elm::ListItem("something went wrong :/"));
    }
    tsl::Gui::requestFocus(m_list, tsl::FocusDirection::None);
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
