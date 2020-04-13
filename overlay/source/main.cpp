#define TESLA_INIT_IMPL
#include "../../ipc/tune.h"
#include "error_gui.hpp"
#include "main_gui.hpp"

#include <tesla.hpp>

class OverlayTest : public tsl::Overlay {
  private:
    bool running, supported;
    Result init_rc;

  public:
    virtual void initServices() override {
        this->init_rc = tuneInitialize();
        if (R_SUCCEEDED(this->init_rc)) {
            this->running = true;
            u32 api;
            if (R_SUCCEEDED(tuneGetApiVersion(&api))) {
                supported = api == TUNE_API_VERSION;
            } else {
                supported = false;
            }
        } else if (this->init_rc != MAKERESULT(Module_Libnx, LibnxError_NotFound)) {
            this->running = true;
        } else {
            this->running = false;
        }
    }
    virtual void exitServices() override {
        if (R_SUCCEEDED(this->init_rc))
            tuneExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        if (!this->running) {
            return std::make_unique<ErrorGui>("sys-tune service not running!");
        } else if (R_FAILED(this->init_rc)) {
            return std::make_unique<ErrorGui>("Something went wrong:", this->init_rc);
        } else if (!supported) {
            return std::make_unique<ErrorGui>("Unsupported sys-tune version!");
        }else {
            return std::make_unique<MainGui>();
        }
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
