#include "impl/music_player.hpp"
#include "impl/sdmc.hpp"
#include "impl/source.hpp"
#include "tune_service.hpp"
#include "tune_result.hpp"

extern "C" {
u32 __nx_applet_type     = AppletType_None;
u32 __nx_fs_num_sessions = 1;

#define INNER_HEAP_SIZE 0x58000
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void);
void __appInit(void);
void __appExit(void);
}

void __libnx_initheap(void) {
    void *addr  = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = (char *)addr;
    fake_heap_end   = (char *)addr + size;
}

void __appInit() {
    R_ABORT_UNLESS(smInitialize());
    R_ABORT_UNLESS(setsysInitialize());
    {
        SetSysFirmwareVersion version;
        R_ABORT_UNLESS(setsysGetFirmwareVersion(&version));
        hosversionSet(MAKEHOSVERSION(version.major, version.minor, version.micro));
        setsysExit();
    }

    R_ABORT_UNLESS(gpioInitialize());
    R_ABORT_UNLESS(pscmInitialize());
    R_ABORT_UNLESS(audrenInitialize(&audren_cfg));
    R_ABORT_UNLESS(fsInitialize());
    R_ABORT_UNLESS(sdmc::Open());
    R_ABORT_UNLESS(tune::InitializeServer());
    smExit();
}

void __appExit(void) {
    sdmc::Close();

    R_ABORT_UNLESS(smInitialize());
    R_ABORT_UNLESS(tune::ExitServer());
    smExit();

    fsExit();
    audrenExit();
    pscmExit();
    gpioExit();
}

namespace {

    alignas(0x1000) u8 gpioThreadBuffer[0x1000];
    alignas(0x1000) u8 pscmThreadBuffer[0x1000];
    alignas(0x1000) u8 tuneThreadBuffer[0x6000];

}

int main(int argc, char *argv[]) {
    R_ABORT_UNLESS(tune::impl::Initialize());

    /* Register audio as our dependency so we can pause before it prepares for sleep. */
    constexpr const u16 dependencies[] = {PscPmModuleId_Audio};

    /* Get pm module to listen for state change. */
    PscPmModule pm_module;
    R_ABORT_UNLESS(pscmGetPmModule(&pm_module, PscPmModuleId(420), dependencies, sizeof(dependencies) / sizeof(u16), true));

    /* Get GPIO session for the headphone jack pad. */
    GpioPadSession headphone_detect_session;
    R_ABORT_UNLESS(gpioOpenSession(&headphone_detect_session, GpioPadName(0x15)));

    ::Thread gpioThread;
    ::Thread pscmThread;
    ::Thread tuneThread;
    R_ABORT_UNLESS(threadCreate(&gpioThread, tune::impl::GpioThreadFunc, &headphone_detect_session, gpioThreadBuffer, 0x1000, 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&pscmThread, tune::impl::PscmThreadFunc, &pm_module, pscmThreadBuffer, 0x1000, 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&tuneThread, tune::impl::TuneThreadFunc, nullptr, tuneThreadBuffer, 0x6000, 0x20, -2));

    R_ABORT_UNLESS(threadStart(&gpioThread));
    R_ABORT_UNLESS(threadStart(&pscmThread));
    R_ABORT_UNLESS(threadStart(&tuneThread));

    /* Create services */
    tune::LoopProcess();

    tune::impl::Exit();
    svcCancelSynchronization(gpioThread.handle);
    svcCancelSynchronization(pscmThread.handle);

    R_ABORT_UNLESS(threadWaitForExit(&gpioThread));
    R_ABORT_UNLESS(threadWaitForExit(&pscmThread));
    R_ABORT_UNLESS(threadWaitForExit(&tuneThread));

    R_ABORT_UNLESS(threadClose(&gpioThread));
    R_ABORT_UNLESS(threadClose(&pscmThread));
    R_ABORT_UNLESS(threadClose(&tuneThread));

    /* Close gpio session. */
    gpioPadClose(&headphone_detect_session);

    /* Unregister Psc module. */
    pscPmModuleFinalize(&pm_module);
    pscPmModuleClose(&pm_module);

    return 0;
}
