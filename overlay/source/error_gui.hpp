#pragma once

#include <tesla.hpp>

class ErrorGui : public tsl::Gui {
  private:
    const char *m_msg    = nullptr;
    const char *m_result = nullptr;
    u32 msgW = 0, msgH = 0;

  public:
    ErrorGui(const char *msg, Result rc);

    virtual tsl::elm::Element *createUI() override;
};
