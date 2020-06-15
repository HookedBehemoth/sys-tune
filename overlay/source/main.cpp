#define TESLA_INIT_IMPL
#include "gui_error.hpp"
#include "gui_player.hpp"
#include "tune.h"

#include "gui_omni.hpp"

#include <tesla.hpp>

constexpr const SocketInitConfig sockConf = {
    .bsdsockets_version = 1,

    .tcp_tx_buf_size = 0x800,
    .tcp_rx_buf_size = 0x800,
    .tcp_tx_buf_max_size = 0x25000,
    .tcp_rx_buf_max_size = 0x25000,

    .udp_tx_buf_size = 0x800,
    .udp_rx_buf_size = 0x800,

    .sb_efficiency = 1,

    .num_bsd_sessions = 0,
    .bsd_service_type = BsdServiceType_Auto,
};

class OverlayTest : public tsl::Overlay {
  private:
    const char *msg = nullptr;
    Result fail     = 0;
    int nxlink = -1;

  public:
    virtual void initServices() final {
        socketInitialize(&sockConf);
        nxlink = nxlinkStdio();

        Result rc = tuneInitialize();

        if (R_VALUE(rc) == MAKERESULT(Module_Libnx, LibnxError_NotFound)) {
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

        u32 api;
        if (R_FAILED(tuneGetApiVersion(&api)) || api != TUNE_API_VERSION) {
            this->msg = "   Unsupported\n"
                        "sys-tune version!";
        }
    }
    virtual void exitServices() final {
        tuneExit();

        ::close(nxlink);
        socketExit();
    }

    virtual void onShow() final {}
    virtual void onHide() final {}

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() final {
        return std::make_unique<OmniTest>();
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
