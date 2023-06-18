#include "aud.h"
#include "audout.h"
#include "aud_wrapper.h"
#include <switch.h>

// IAudioSystemManagerForApplet
Result audWrapperInitialize(void) {
    if (hosversionBefore(11,0,0))
        return audoutaInitialize();
    else
        return audaInitialize();
}

void audWrapperExit(void) {
    if (hosversionBefore(11,0,0))
        audoutaInitialize();
    else
        audaInitialize();
}

Result audWrapperRequestSuspend(u64 pid, u64 delay) {
    Handle h;
    if (hosversionBefore(4,0,0))
        return audoutaRequestSuspendOld(pid, delay, &h);
    else if (hosversionBefore(11,0,0))
        return audoutaRequestSuspend(pid, delay);
    else
        return audaRequestSuspendAudio(pid, delay);
}

Result audWrapperRequestResume(u64 pid, u64 delay) {
    Handle h;
    if (hosversionBefore(4,0,0))
        return audoutaRequestResumeOld(pid, delay, &h);
    else if (hosversionBefore(11,0,0))
        return audoutaRequestResume(pid, delay);
    else
        return audaRequestResumeAudio(pid, delay);
}

Result audWrapperGetProcessMasterVolume(u64 pid, float* volume_out) {
    if (hosversionBefore(11,0,0))
        return audoutaGetProcessMasterVolume(pid, volume_out);
    else
        return audaGetAudioOutputProcessMasterVolume(pid, volume_out);
}

Result audWrapperSetProcessMasterVolume(u64 pid, u64 delay, float volume) {
    if (hosversionBefore(11,0,0))
        return audoutaSetProcessMasterVolume(pid, delay, volume);
    else {
        Result rc = audaSetAudioOutputProcessMasterVolume(pid, delay, volume);
        if (R_FAILED(rc)) {
            return rc;
        }
        return audaSetAudioInputProcessMasterVolume(pid, delay, volume);
    }
}

Result audWrapperGetProcessRecordVolume(u64 pid, float* volume_out) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (hosversionBefore(11,0,0))
        return audoutaGetProcessRecordVolume(pid, volume_out);
    else
        return audaGetAudioOutputProcessRecordVolume(pid, volume_out);
}

Result audWrapperSetProcessRecordVolume(u64 pid, u64 delay, float volume) {
    if (hosversionBefore(4,0,0))
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);

    if (hosversionBefore(11,0,0))
        return audoutaSetProcessRecordVolume(pid, delay, volume);
    else
        return audaSetAudioOutputProcessRecordVolume(pid, delay, volume);
}
