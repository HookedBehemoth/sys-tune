#include "impl/music_player.hpp"
#include "impl/source.hpp"
#include "tune_service.hpp"
#include "tune_result.hpp"

namespace {

    alignas(0x1000) u8 gpioThreadBuffer[0x1000];
    alignas(0x1000) u8 pscmThreadBuffer[0x1000];
    alignas(0x1000) u8 tuneThreadBuffer[0x6000];

}

int main(int argc, char *argv[]) {
    R_ABORT_UNLESS(tune::impl::Initialize());

    /* Register audio as our dependency so we can pause before it prepares for sleep. */
    constexpr const u32 dependencies[] = { PscPmModuleId_Audio };

    /* Get pm module to listen for state change. */
    PscPmModule pm_module;
    R_ABORT_UNLESS(pscmGetPmModule(&pm_module, PscPmModuleId(420), dependencies, sizeof(dependencies) / sizeof(u32), true));

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
    R_ABORT_UNLESS(tune::InitializeServer());
    tune::LoopProcess();
    R_ABORT_UNLESS(tune::ExitServer());

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
