#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "audout.h"
#include <switch.h>

static Service g_audoutaSrv;
static Service g_audoutdSrv;

// AudioOutManagerForApplet
Result audoutaInitialize(void) {
    if (hosversionAtLeast(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_audoutaSrv, "audout:a");
}

void audoutaExit(void) {
    serviceClose(&g_audoutaSrv);
}

Service* audoutaGetServiceSession(void) {
    return &g_audoutaSrv;
}

Result audoutaRequestSuspendOld(u64 pid, u64 delay, Handle* handle_out) {
    if (hosversionAtLeast(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchInOut(&g_audoutaSrv, 0, in, *handle_out);
}

Result audoutaRequestResumeOld(u64 pid, u64 delay, Handle* handle_out) {
    if (hosversionAtLeast(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchInOut(&g_audoutaSrv, 1, in, *handle_out);
}

Result audoutaRequestSuspend(u64 pid, u64 delay) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audoutaSrv, 0, in);
}

Result audoutaRequestResume(u64 pid, u64 delay) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audoutaSrv, 1, in);
}

Result audoutaGetProcessMasterVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audoutaSrv, 2, pid, *volume_out);
}

Result audoutaSetProcessMasterVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 timespan;
    } in = { volume, pid, 0 };

    return serviceDispatchIn(&g_audoutaSrv, 3, in);
}

Result audoutaGetProcessRecordVolume(u64 pid, float* volume_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return serviceDispatchInOut(&g_audoutaSrv, 4, pid, *volume_out);
}

Result audoutaSetProcessRecordVolume(u64 pid, u64 delay, float volume) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    const struct {
        float volume;
        u64 pid;
        u64 timespan;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audoutaSrv, 5, in);
}

// IAudioOutManagerForDebugger
Result audoutdInitialize(void) {
    if (hosversionAtLeast(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_audoutdSrv, "audout:d");
}

void audoutdExit(void) {
    serviceClose(&g_audoutdSrv);
}

Service* audoutdGetServiceSession(void) {
    return &g_audoutdSrv;
}

Result audoutdRequestSuspendForDebug(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audoutdSrv, 0, in);
}

Result audoutdRequestResumeForDebug(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audoutdSrv, 1, in);
}
