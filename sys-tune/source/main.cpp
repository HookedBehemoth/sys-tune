#include "impl/music_player.hpp"
#include "music_control_service.hpp"

extern "C" {
extern u32 __start__;

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;
u32 __nx_fsdev_direntry_cache_size = 1;

#define INNER_HEAP_SIZE 0x40000
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void);
void __appInit(void);
void __appExit(void);

/* Exception handling. */
alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = {0x4200000000000000};

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_initheap(void) {
    void *addr = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = (char *)addr;
    fake_heap_end = (char *)addr + size;
}

void __appInit() {
    hos::SetVersionForLibnx();

    sm::DoWithSession([] {
        R_ABORT_UNLESS(gpioInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(audoutInitialize());
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(fsdevMountSdmc());
}

void __appExit(void) {
    fsdevUnmountAll();

    fsExit();
    audoutExit();
    pscmExit();
    gpioExit();
}

namespace {

    /* music */
    constexpr size_t NumServers = 1;
    sf::hipc::ServerManager<NumServers> g_server_manager;

    constexpr sm::ServiceName MusicServiceName = sm::ServiceName::Encode("tune");
    constexpr size_t MusicMaxSessions = 0x2;

}

int main(int argc, char *argv[]) {
    R_ABORT_UNLESS(tune::impl::Initialize());

    /* Register audio as our dependency so we can pause before it prepares for sleep. */
    u16 dependencies[] = {PscPmModuleId_Audio};

    /* Get pm module to listen for state change. */
    PscPmModule pm_module;
    R_ABORT_UNLESS(pscmGetPmModule(&pm_module, PscPmModuleId(420), dependencies, 1, false));

    /* Get GPIO session for the headphone jack pad. */
    GpioPadSession headphone_detect_session;
    R_ABORT_UNLESS(gpioOpenSession(&headphone_detect_session, GpioPadName(0x15)));

    os::Thread gpioThread, pscThread, audioThread;
    R_ABORT_UNLESS(gpioThread.Initialize(tune::impl::GpioThreadFunc, nullptr, 0x1000, 0x20));
    R_ABORT_UNLESS(pscThread.Initialize(tune::impl::PscThreadFunc, nullptr, 0x1000, 0x20));
    R_ABORT_UNLESS(audioThread.Initialize(tune::impl::ThreadFunc, nullptr, 0x8000, 0x20));

    R_ABORT_UNLESS(gpioThread.Start());
    R_ABORT_UNLESS(pscThread.Start());
    R_ABORT_UNLESS(audioThread.Start());

    /* Create services */
    R_ABORT_UNLESS(g_server_manager.RegisterServer<tune::ControlService>(MusicServiceName, MusicMaxSessions));

    g_server_manager.LoopProcess();

    tune::impl::Exit();

    R_ABORT_UNLESS(gpioThread.Wait());
    R_ABORT_UNLESS(pscThread.Wait());
    R_ABORT_UNLESS(audioThread.Wait());

    R_ABORT_UNLESS(gpioThread.Join());
    R_ABORT_UNLESS(pscThread.Join());
    R_ABORT_UNLESS(audioThread.Join());

    /* Close gpio session. */
    gpioPadClose(&headphone_detect_session);

    /* Unregister Psc module. */
    pscPmModuleFinalize(&pm_module);
    pscPmModuleClose(&pm_module);

    return 0;
}
