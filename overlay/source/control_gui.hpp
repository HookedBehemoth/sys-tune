#pragma once

#include <tesla.hpp>

class ControlGui : public tsl::Gui {
  private:
    tsl::elm::List m_list;

  public:
    ControlGui();

    virtual tsl::elm::Element *createUI() override;

    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) override;
};