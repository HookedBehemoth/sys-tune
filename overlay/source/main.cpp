#define TESLA_INIT_IMPL
#include "../../ipc/tune.h"
#include "error_gui.hpp"
#include "main_gui.hpp"

#include <tesla.hpp>

constexpr const SocketInitConfig sockConf = {
    .bsdsockets_version = 1,

    .tcp_tx_buf_size     = 0x800,
    .tcp_rx_buf_size     = 0x800,
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
    int nxlink;

  public:
    virtual void initServices() override {
        socketInitialize(&sockConf);
        nxlink = nxlinkStdio();

        Result rc = tuneInitialize();

        if (rc == MAKERESULT(Module_Libnx, LibnxError_NotFound)) {
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
        }

        u32 api;
        if (R_FAILED(tuneGetApiVersion(&api)) || api != TUNE_API_VERSION) {
            this->msg = "   Unsupported\n"
                        "sys-tune version!";
        }
    }
    virtual void exitServices() override {
        tuneExit();
        ::close(nxlink);
        socketExit();
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
