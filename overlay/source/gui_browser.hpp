#pragma once

#include <tesla.hpp>

#include "elm_overlayframe.hpp"

class BrowserGui final : public tsl::Gui {
  private:
    SysTuneOverlayFrame* m_frame;
    tsl::elm::List *m_list;
    FsFileSystem m_fs;
    bool has_music;
    char cwd[FS_MAX_PATH];

  public:
    BrowserGui();
    ~BrowserGui();

    virtual tsl::elm::Element *createUI() final;
    virtual void update() final;
    virtual bool handleInput(u64 keysDown, u64, const HidTouchState&, HidAnalogStickState, HidAnalogStickState) final;

  private:
    void scanCwd();
    void upCwd();
    void addAllToPlaylist();
    void infoAlert(const std::string &title, const std::string &text);
};
