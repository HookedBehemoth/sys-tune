#include "music.h"

#include <string.h>

Service g_music;

bool musicIsRunning() {
    Handle handle;
    SmServiceName music = smEncodeName("music");
    Result rc = smRegisterService(&handle, music, false, 1);

    if (R_SUCCEEDED(rc)) {
        smUnregisterService(music);
        return false;
    } else {
        return true;
    }
}

Result musicInitialize() {
    return smGetService(&g_music, "music");
}

void musicExit() {
    serviceClose(&g_music);
}

Result musicGetStatus(MusicPlayerStatus *out) {
    u8 tmp;
    Result rc = serviceDispatchOut(&g_music, 0, tmp);
    if (R_SUCCEEDED(rc) && out)
        *out = tmp;
    return rc;
}

Result musicSetStatus(MusicPlayerStatus status) {
    u8 tmp = status;
    return serviceDispatchIn(&g_music, 1, tmp);
}

Result musicGetQueueCount(u32 *out) {
    u32 tmp;
    Result rc = serviceDispatchOut(&g_music, 2, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result musicGetCurrent(char *out_path, size_t out_path_length) {
    return serviceDispatch(&g_music, 3,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { out_path, out_path_length } },
    );
}

Result musicGetList(u32 *read, char *out_path, size_t out_path_length) {
    u32 tmp;
    Result rc = serviceDispatchOut(&g_music, 4, tmp,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { out_path, out_path_length } },
    );
    if (R_SUCCEEDED(rc) && read) *read = tmp;
    return rc;
}

Result musicGetCurrentLength(double *out) {
    double tmp;
    Result rc = serviceDispatchOut(&g_music, 5, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result musicGetCurrentProgress(double *out) {
    double tmp;
    Result rc = serviceDispatchOut(&g_music, 6, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result musicGetVolume(double *out) {
    double tmp;
    Result rc = serviceDispatchOut(&g_music, 7, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp;
    return rc;
}

Result musicSetVolume(double volume) {
    return serviceDispatchIn(&g_music, 8, volume);
}

Result musicAddToQueue(const char *path) {
    size_t path_length = strlen(path);
    return serviceDispatch(&g_music, 10,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { path, path_length } },
    );
}

Result musicClear() {
    return serviceDispatch(&g_music, 11);
}
