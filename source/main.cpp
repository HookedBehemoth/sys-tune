#include "impl/music_player.hpp"
#include "music_control_service.hpp"
#include "scoped_file.hpp"

extern "C" {
extern u32 __start__;

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 1;
u32 __nx_fsdev_direntry_cache_size = 1;

#define INNER_HEAP_SIZE 0x4000
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

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    CrashHandler(ctx);
}

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
        R_ASSERT(audoutInitialize());
        R_ASSERT(timeInitialize());
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fs::MountSdCard("sdmc"));
}

void __appExit(void) {
    fs::Unmount("sdmc");
    fsExit();
    timeExit();
    audoutExit();
}

namespace {

    /* music */
    constexpr size_t NumServers = 1;
    sf::hipc::ServerManager<NumServers> g_server_manager;

    constexpr sm::ServiceName MusicServiceName = sm::ServiceName::Encode("music");
    constexpr size_t MusicMaxSessions = 0x2;

}

int main(int argc, char *argv[]) {
    {
        ScopedFile log("sdmc:/music.log");
        log.WriteString("started\n");
    }

    music::Initialize();

    os::Thread audioThread;
    audioThread.Initialize(music::ThreadFunc, nullptr, 0x32000, 0x2C);
    audioThread.Start();

    /* Create services */
    R_ASSERT(g_server_manager.RegisterServer<music::ControlService>(MusicServiceName, MusicMaxSessions));

    g_server_manager.LoopProcess();

    music::Exit();

    audioThread.Wait();
    audioThread.Join();

    {
        ScopedFile log("sdmc:/music.log");
        log.WriteString("exit\n");
    }
    return 0;
}
