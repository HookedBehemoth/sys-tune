#define TESLA_INIT_IMPL
#include "../../ipc/music.h"
#include "error_gui.hpp"
#include "main_gui.hpp"

#include <tesla.hpp>

class OverlayTest : public tsl::Overlay {
  private:
    bool running;
    Result init_rc;

  public:
    virtual void initServices() override {
        this->running = musicIsRunning();
        if (this->running)
            init_rc = musicInitialize();
    }
    virtual void exitServices() override {
        if (R_SUCCEEDED(this->init_rc))
            musicExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        if (!this->running) {
            return std::make_unique<ErrorGui>("sys-tune service not running!");
        } else if (R_FAILED(this->init_rc)) {
            return std::make_unique<ErrorGui>("Something went wrong:", this->init_rc);
        } else {
            return std::make_unique<MainGui>();
        }
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
