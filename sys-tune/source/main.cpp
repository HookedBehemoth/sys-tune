#include "impl/music_player.hpp"
#include "sdmc/sdmc.hpp"
#include "pm/pm.hpp"
#include "impl/aud_wrapper.h"
#include "impl/source.hpp"
#include "tune_service.hpp"
#include "tune_result.hpp"

extern "C" {
u32 __nx_applet_type     = AppletType_None;
u32 __nx_fs_num_sessions = 1;

// do not decrease this, will either cause fatal or will fail to start
// - 1024 * 216: needed for sys-tune to boot
// - 1024 * 236: base
// - 1024 * 268: needed for mp3 playback (at 32kb)
// - 1024 * 300: needed for mp3 playback (at 64kb)
// - 1024 * 332: needed for mp3 playback (at 96kb)
// - 1024 * 364: needed for closing / reopening audrv and audren (mp3 at 0kb)
// - 1024 * 460: needed for closing / reopening audrv and audren (mp3 at 96kb)
#define INNER_HEAP_SIZE 1024 * (364 + MP3_CHUNK_SIZE_KB)

void __libnx_initheap(void) {
    static char inner_heap[INNER_HEAP_SIZE];
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    // Configure the newlib heap.
    fake_heap_start = inner_heap;
    fake_heap_end   = inner_heap + sizeof(inner_heap);
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
    R_ABORT_UNLESS(fsInitialize());
    R_ABORT_UNLESS(audWrapperInitialize());
    R_ABORT_UNLESS(pm::Initialize());
    R_ABORT_UNLESS(sdmc::Open());
    smExit();
}

void __appExit(void) {
    sdmc::Close();
    pm::Exit();
    audWrapperExit();
    fsExit();
    pscmExit();
    gpioExit();
}

} // extern "C"

namespace {

    alignas(0x1000) u8 gpioThreadBuffer[0x1000];
    alignas(0x1000) u8 pscmThreadBuffer[0x1000];
    alignas(0x1000) u8 pmdmntThreadBuffer[0x1000];
    alignas(0x1000) u8 tuneThreadBuffer[0x6000];

}

int main(int argc, char *argv[]) {
    std::vector<tune::impl::PlaylistEntry> playlist;
    std::vector<tune::impl::PlaylistID> shuffle;
    tune::impl::PlaylistEntry current;
    R_ABORT_UNLESS(tune::impl::Initialize(&playlist, &shuffle, &current));

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
    ::Thread pmdmtThread;
    ::Thread tuneThread;
    R_ABORT_UNLESS(threadCreate(&gpioThread, tune::impl::GpioThreadFunc, &headphone_detect_session, gpioThreadBuffer, sizeof(gpioThreadBuffer), 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&pscmThread, tune::impl::PscmThreadFunc, &pm_module, pscmThreadBuffer, sizeof(pscmThreadBuffer), 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&pmdmtThread, tune::impl::PmdmntThreadFunc, nullptr, pmdmntThreadBuffer, sizeof(pmdmntThreadBuffer), 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&tuneThread, tune::impl::TuneThreadFunc, nullptr, tuneThreadBuffer, sizeof(tuneThreadBuffer), 0x20, -2));

    R_ABORT_UNLESS(threadStart(&gpioThread));
    R_ABORT_UNLESS(threadStart(&pscmThread));
    R_ABORT_UNLESS(threadStart(&pmdmtThread));
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
    R_ABORT_UNLESS(threadWaitForExit(&pmdmtThread));
    R_ABORT_UNLESS(threadWaitForExit(&tuneThread));

    R_ABORT_UNLESS(threadClose(&gpioThread));
    R_ABORT_UNLESS(threadClose(&pscmThread));
    R_ABORT_UNLESS(threadClose(&pmdmtThread));
    R_ABORT_UNLESS(threadClose(&tuneThread));

    /* Close gpio session. */
    gpioPadClose(&headphone_detect_session);

    /* Unregister Psc module. */
    pscPmModuleFinalize(&pm_module);
    pscPmModuleClose(&pm_module);

    return 0;
}
