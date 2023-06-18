#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "aud.h"
#include <switch.h>

static Service g_audaSrv;
static Service g_auddSrv;

// IAudioSystemManagerForApplet
Result audaInitialize(void) {
    if (hosversionBefore(11,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    return smGetService(&g_audaSrv, "aud:a");
}

void audaExit(void) {
    serviceClose(&g_audaSrv);
}

Service* audaGetServiceSession(void) {
    return &g_audaSrv;
}

Result audaRequestSuspendAudio(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audaSrv, 2, in);
}

Result audaRequestResumeAudio(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_audaSrv, 3, in);
}

Result audaGetAudioOutputProcessMasterVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audaSrv, 4, pid, *volume_out);
}

Result audaSetAudioOutputProcessMasterVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 timespan;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audaSrv, 5, in);
}

Result audaGetAudioInputProcessMasterVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audaSrv, 6, pid, *volume_out);
}

Result audaSetAudioInputProcessMasterVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 timespan;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audaSrv, 7, in);
}

Result audaGetAudioOutputProcessRecordVolume(u64 pid, float* volume_out) {
    return serviceDispatchInOut(&g_audaSrv, 8, pid, *volume_out);
}

Result audaSetAudioOutputProcessRecordVolume(u64 pid, u64 delay, float volume) {
    const struct {
        float volume;
        u64 pid;
        u64 timespan;
    } in = { volume, pid, delay };

    return serviceDispatchIn(&g_audaSrv, 9, in);
}

// IAudioSystemManagerForDebugger
Result auddInitialize(void) {
    return smGetService(&g_auddSrv, "aud:d");
}

void auddExit(void) {
    serviceClose(&g_auddSrv);
}

Service* auddGetServiceSession(void) {
    return &g_auddSrv;
}

Result auddRequestSuspendAudioForDebug(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_auddSrv, 0, in);
}

Result auddRequestResumeAudioForDebug(u64 pid, u64 delay) {
    const struct {
        u64 pid;
        u64 timespan;
    } in = { pid, delay };

    return serviceDispatchIn(&g_auddSrv, 1, in);
}
