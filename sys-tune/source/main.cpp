#include "impl/music_player.hpp"

#ifdef SYS
#include "music_control_service.hpp"

extern "C" {
extern u32 __start__;

u32 __nx_applet_type     = AppletType_None;
u32 __nx_fs_num_sessions = 1;

#define INNER_HEAP_SIZE 0x60000
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
    void *addr  = nx_inner_heap;
    size_t size = nx_inner_heap_size;

    /* Newlib */
    extern char *fake_heap_start;
    extern char *fake_heap_end;

    fake_heap_start = (char *)addr;
    fake_heap_end   = (char *)addr + size;
}

void __appInit() {
    sm::DoWithSession([] {
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
    });

    R_ABORT_UNLESS(ams::fs::MountSdCard("sdmc"));
}

void __appExit(void) {
    ams::fs::Unmount("sdmc");

    fsExit();
    audrenExit();
    pscmExit();
    gpioExit();
}
namespace ams::tune {

    /* music */
    constexpr size_t NumServers = 1;
    sf::hipc::ServerManager<NumServers> g_server_manager;

    constexpr sm::ServiceName MusicServiceName = sm::ServiceName::Encode("tune");
    constexpr size_t MusicMaxSessions = 0x2;

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
    ::Thread pscThread;
    ::Thread audioThread;
    R_ABORT_UNLESS(threadCreate(&gpioThread, tune::impl::GpioThreadFunc, &headphone_detect_session, nullptr, 0x1000, 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&pscThread, tune::impl::PscThreadFunc, &pm_module, nullptr, 0x1000, 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&audioThread, tune::impl::AudioThreadFunc, nullptr, nullptr, 0x6000, 0x20, -2));

    R_ABORT_UNLESS(threadStart(&gpioThread));
    R_ABORT_UNLESS(threadStart(&pscThread));
    R_ABORT_UNLESS(threadStart(&audioThread));

    /* Create services */
    R_ABORT_UNLESS(tune::g_server_manager.RegisterServer<tune::ControlService>(tune::MusicServiceName, tune::MusicMaxSessions));

    tune::g_server_manager.LoopProcess();

    tune::impl::Exit();
    svcCancelSynchronization(gpioThread.handle);
    svcCancelSynchronization(pscThread.handle);

    R_ABORT_UNLESS(threadWaitForExit(&gpioThread));
    R_ABORT_UNLESS(threadWaitForExit(&pscThread));
    R_ABORT_UNLESS(threadWaitForExit(&audioThread));

    R_ABORT_UNLESS(threadClose(&gpioThread));
    R_ABORT_UNLESS(threadClose(&pscThread));
    R_ABORT_UNLESS(threadClose(&audioThread));

    /* Close gpio session. */
    gpioPadClose(&headphone_detect_session);

    /* Unregister Psc module. */
    pscPmModuleFinalize(&pm_module);
    pscPmModuleClose(&pm_module);

    return 0;
}

#endif
#ifdef APPLET

namespace ams {

    ncm::ProgramId CurrentProgramId = {0x4200000000000000};

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

#include "impl/source.hpp"
#include "music_result.hpp"

#include <unistd.h>

#ifdef R_ABORT_UNLESS
#undef R_ABORT_UNLESS
#endif

#define R_ABORT_UNLESS(res_expr)                                                                                       \
    ({                                                                                                                 \
        const auto res = static_cast<::ams::Result>((res_expr));                                                       \
        if (R_FAILED(res))                                                                                             \
            LOG("%s failed with 0x%x 2%03d-%04d\n", #res_expr, res.GetValue(), res.GetModule(), res.GetDescription()); \
    })

extern "C" {
void __appInit(void);
void __appExit(void);
}

int sock = 0;

void __appInit() {
    ams::sm::DoWithSession([] {
        R_ABORT_UNLESS(setsysInitialize());
        {
            SetSysFirmwareVersion version;
            R_ABORT_UNLESS(setsysGetFirmwareVersion(&version));
            hosversionSet(MAKEHOSVERSION(version.major, version.minor, version.micro));
            setsysExit();
        }

        R_ABORT_UNLESS(socketInitializeDefault());
        sock = nxlinkStdio();

        R_ABORT_UNLESS(hidInitialize());

        R_ABORT_UNLESS(gpioInitialize());
        R_ABORT_UNLESS(audrenInitialize(&audren_cfg));
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(ams::fs::MountSdCard("sdmc"));
}

void __appExit(void) {
    ams::fs::Unmount("sdmc");

    fsExit();
    audrenExit();
    gpioExit();

    hidExit();

    close(sock);
    socketExit();
}

int main(int argc, char *argv[]) {
    R_ABORT_UNLESS(ams::tune::impl::Initialize());

    /* Get GPIO session for the headphone jack pad. */
    GpioPadSession headphone_detect_session;
    R_ABORT_UNLESS(gpioOpenSession(&headphone_detect_session, GpioPadName(0x15)));

    ::Thread gpioThread;
    ::Thread audioThread;
    R_ABORT_UNLESS(threadCreate(&gpioThread, ams::tune::impl::GpioThreadFunc, &headphone_detect_session, nullptr, 0x1000, 0x20, -2));
    R_ABORT_UNLESS(threadCreate(&audioThread, ams::tune::impl::AudioThreadFunc, nullptr, nullptr, 0x6000, 0x20, -2));

    R_ABORT_UNLESS(threadStart(&gpioThread));
    R_ABORT_UNLESS(threadStart(&audioThread));

    while (appletMainLoop()) {
        hidScanInput();
        u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
        if (kDown & KEY_A) {
            bool status = ams::tune::impl::GetStatus();
            if (status) {
                ams::tune::impl::Pause();
            } else {
                ams::tune::impl::Play();
            }
            LOG("%spaused\n", status ? "un" : "");
        }

        if (kDown & KEY_DLEFT) {
            const char *path = "sdmc:/music/flac/Stadtaffe/04 Kopf verloren.flac";
            ams::tune::impl::Enqueue(path, std::strlen(path), ams::tune::EnqueueType::Next);
            ams::tune::impl::Play();
        }

        if (kDown & KEY_DUP) {
            const char *path = "sdmc:/music/1_Garbage Day.wav";
            ams::tune::impl::Enqueue(path, std::strlen(path), ams::tune::EnqueueType::Next);
            ams::tune::impl::Play();
        }

        if (kDown & KEY_DRIGHT) {
            const char *path = "sdmc:/music/03 Superman.mp3";
            ams::tune::impl::Enqueue(path, std::strlen(path), ams::tune::EnqueueType::Next);
            ams::tune::impl::Play();
        }

        if (kDown & KEY_DDOWN) {
            /* Just a _really_ long compressed sample. Don't judge me please. */
            const char *path = "sdmc:/music/misc/Dj CUTMAN - The Legend of Dubstep.mp3";
            ams::tune::impl::Enqueue(path, std::strlen(path), ams::tune::EnqueueType::Next);
            ams::tune::impl::Play();
        }

        if (kDown & KEY_R)
            ams::tune::impl::Next();

        if (kDown & KEY_L)
            ams::tune::impl::Prev();

        extern Source *g_source;

        /* Playback */
        if ((g_source != nullptr) && g_source->IsOpen()) {
            static u64 tick = 0;
            if ((tick % 30) == 0) {
                auto [current, total] = g_source->Tell();
                u32 length            = total / g_source->GetSampleRate();
                std::printf("%d/%d %d:%d\n", current, total, length / 60, length % 60);
            }
            tick++;

            if (kDown & (KEY_ZL | KEY_ZR)) {
                auto [current, total] = g_source->Tell();
                u64 next              = current;
                if (kDown & KEY_ZL) {
                    next = std::max(s64(current) - s64(total / 10), s64(0));
                } else {
                    next = std::min(current + (total / 10), total);
                }
                std::printf("seeking to %ld\n", next);
                g_source->Seek(next);
            }
        }

        if (kDown & KEY_PLUS)
            break;

        svcSleepThread(17'000'000);
    }

    ams::tune::impl::Exit();
    svcCancelSynchronization(gpioThread.handle);

    R_ABORT_UNLESS(threadWaitForExit(&gpioThread));
    R_ABORT_UNLESS(threadWaitForExit(&audioThread));

    R_ABORT_UNLESS(threadClose(&gpioThread));
    R_ABORT_UNLESS(threadClose(&audioThread));

    /* Close gpio session. */
    gpioPadClose(&headphone_detect_session);
}

#endif
