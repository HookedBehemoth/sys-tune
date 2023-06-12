#define TESLA_INIT_IMPL
#include "tune.h"
#include "gui_error.hpp"
#include "gui_main.hpp"
#include "sdmc/sdmc.hpp"
#include "pm/pm.hpp"
#include "config/config.hpp"

#include <tesla.hpp>

class OverlayTest : public tsl::Overlay {
  private:
    const char *msg = nullptr;
    Result fail     = 0;

  public:
    virtual void initServices() override {
        Result rc = tuneInitialize();

        if (R_VALUE(rc) == KERNELRESULT(NotFound)) {
            u64 pid = 0;
            const NcmProgramLocation programLocation{
                .program_id = 0x4200000000000000,
                .storageID  = NcmStorageId_None,
            };
            rc = pmshellInitialize();
            if (R_SUCCEEDED(rc))
                rc = pmshellLaunchProgram(0, &programLocation, &pid);
            pmshellExit();
            if (R_FAILED(rc) || pid == 0) {
                this->fail = rc;
                this->msg  = "  Failed to\n"
                            "launch sysmodule";
                return;
            }
            svcSleepThread(100'000'000);
            rc = tuneInitialize();
        }

        if (R_FAILED(rc)) {
            this->msg  = "Something went wrong:";
            this->fail = rc;
            return;
        }

        if (R_FAILED(sdmc::Open())) {
            this->msg  = "Failed sdmc::Open()";
            return;
        }

        if (R_FAILED(pm::Initialize())) {
            this->msg  = "Failed pm::Initialize()";
            return;
        }

        u32 api;
        if (R_FAILED(tuneGetApiVersion(&api)) || api != TUNE_API_VERSION) {
            this->msg = "   Unsupported\n"
                        "sys-tune version!";
        }
    }
    virtual void exitServices() override {
        sdmc::Close();
        pm::Exit();
        tuneExit();
    }

    virtual void onShow() override {}
    virtual void onHide() override {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        if (this->msg) {
            return std::make_unique<ErrorGui>(this->msg, this->fail);
        } else {
            return std::make_unique<MainGui>();
        }
    }
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
