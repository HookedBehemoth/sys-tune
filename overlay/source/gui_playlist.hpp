#pragma once

#include <tesla.hpp>

class PlaylistGui final : public tsl::Gui {
  private:
    tsl::elm::List *m_list;

  public:
    PlaylistGui();

    tsl::elm::Element *createUI() override;
};
