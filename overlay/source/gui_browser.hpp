#pragma once

#include <tesla.hpp>

class BrowserGui final : public tsl::Gui {
  private:
    tsl::elm::List *m_list;
    FsFileSystem m_fs;
    bool has_music;
    char cwd[FS_MAX_PATH];

  public:
    BrowserGui();
    ~BrowserGui();

    virtual tsl::elm::Element *createUI() final;
    virtual void update() final;
    virtual bool handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) final;

  private:
    void scanCwd();
    void upCwd();
};
