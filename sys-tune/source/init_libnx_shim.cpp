#include <switch.h>
#include "impl/sdmc.hpp"
#include "tune_result.hpp"
#include "impl/music_player.hpp"

extern "C" {
u32 __nx_applet_type     = AppletType_None;
u32 __nx_fs_num_sessions = 1;

#define INNER_HEAP_SIZE 0x60000
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
    smExit();
}

void __appExit(void) {
    sdmc::Close();

    fsExit();
    audrenExit();
    pscmExit();
    gpioExit();
}
