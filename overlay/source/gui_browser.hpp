#pragma once

#include <tesla.hpp>

class BrowserGui final : public tsl::Gui {
  private:
    tsl::elm::List *m_list;
    FsFileSystem m_fs;
    bool has_music;
    char cwd[FS_MAX_PATH];
    /* Parameters for controlling the info section */
    tsl::elm::CategoryHeader* info_header;
    tsl::elm::CustomDrawer* info_draw_string;
    bool show_info;

  public:
    BrowserGui();
    ~BrowserGui();

    virtual tsl::elm::Element *createUI() final;
    virtual void update() final;
    virtual bool handleInput(u64 keysDown, u64, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) final;

  private:
    void scanCwd();
    void upCwd();
    void addAllToPlaylist(FsDir dir);
    void infoAlert(std::string text, tsl::elm::List *list);
};
