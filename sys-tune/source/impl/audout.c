#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "audout.h"
#include <switch.h>

Result audoutaRequestSuspendOld(u64 pid, u64 delay, Handle* handle_out) {
    if (hosversionAtLeast(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    Service* audouta = audoutaGetServiceSession();
    return serviceDispatchInOut(audouta, 0, in, *handle_out);
}

Result audoutaRequestResumeOld(u64 pid, u64 delay, Handle* handle_out) {
    if (hosversionAtLeast(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    Service* audouta = audoutaGetServiceSession();
    return serviceDispatchInOut(audouta, 1, in, *handle_out);
}
