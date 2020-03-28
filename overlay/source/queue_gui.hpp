#pragma once

#include <tesla.hpp>

class QueueGui : public tsl::Gui {
  private:
    u32 count;
    tsl::elm::List *m_list;

  public:
    QueueGui();

    virtual tsl::elm::Element *createUI() override;
    virtual void update() override;
    virtual bool handleInput(u64 keysDown, u64, touchPosition, JoystickPosition, JoystickPosition) override;
};
