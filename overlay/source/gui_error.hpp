#pragma once

#include <tesla.hpp>

class ErrorGui final : public tsl::Gui {
  private:
    const char *m_msg    = nullptr;
    const char *m_result = nullptr;
    u32 msgW = 0, msgH = 0;

  public:
    ErrorGui(const char *msg, Result rc);

    tsl::elm::Element *createUI() override;
};
