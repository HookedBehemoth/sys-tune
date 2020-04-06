#include "tune.h"

#include <string.h>

Service g_tune;

bool tuneIsRunning() {
    Handle handle;
    SmServiceName tune = smEncodeName("tune");
    Result rc = smRegisterService(&handle, tune, false, 1);

    if (R_SUCCEEDED(rc)) {
        smUnregisterService(tune);
        return false;
    } else {
        return true;
    }
}

Result tuneInitialize() {
    return smGetService(&g_tune, "tune");
}

void tuneExit() {
    serviceClose(&g_tune);
}

Result tuneGetStatus(AudioOutState *status) {
    return serviceDispatchOut(&g_tune, 0, *status);
}

Result tunePlay() {
    return serviceDispatch(&g_tune, 1);
}

Result tunePause() {
    return serviceDispatch(&g_tune, 2);
}

Result tuneNext() {
    return serviceDispatch(&g_tune, 3);
}

Result tunePrev() {
    return serviceDispatch(&g_tune, 4);
}

Result tuneGetVolume(float *out) {
    return serviceDispatchOut(&g_tune, 10, *out);
}

Result tuneSetVolume(float volume) {
    return serviceDispatchIn(&g_tune, 11, volume);
}

Result tuneGetRepeatMode(TuneRepeatMode *state) {
    u8 out=0;
    Result rc = serviceDispatchOut(&g_tune, 20, out);
    if (R_SUCCEEDED(rc) && state) *state = out;
    return rc;
}

Result tuneSetRepeatMode(TuneRepeatMode state) {
    u8 tmp = state;
    return serviceDispatchIn(&g_tune, 21, tmp);
}

Result tuneGetShuffleMode(TuneShuffleMode *state) {
    u8 out=0;
    Result rc = serviceDispatchOut(&g_tune, 22, out);
    if (R_SUCCEEDED(rc) && state) *state = out;
    return rc;
}
Result tuneSetShuffleMode(TuneShuffleMode state) {
    u8 tmp = state;
    return serviceDispatchIn(&g_tune, 23, tmp);
}

Result tuneGetCurrentPlaylistSize(u32 *count) {
    return serviceDispatchOut(&g_tune, 30, *count);
}

Result tuneGetCurrentPlaylist(u32 *read, char *out_path, size_t out_path_length) {
    return serviceDispatchOut(&g_tune, 31, *read,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { out_path, out_path_length } },
    );
}

Result tuneGetCurrentQueueItem(char *out_path, size_t out_path_length, TuneCurrentStats *out) {
    return serviceDispatchOut(&g_tune, 32, *out,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { out_path, out_path_length } },
    );
}

Result tuneClearQueue() {
    return serviceDispatch(&g_tune, 33);
}

Result tuneMoveQueueItem(u32 src, u32 dst) {
    const struct {
        u32 src;
        u32 dst;
    } in = { src, dst };
    return serviceDispatchIn(&g_tune, 34, in);
}

Result tuneEnqueue(const char *path) {
    size_t path_length = strlen(path);
    return serviceDispatch(&g_tune, 40,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { path, path_length } },
    );
}

Result tuneRemove(const char *path) {
    size_t path_length = strlen(path);
    return serviceDispatch(&g_tune, 41,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { path, path_length } },
    );
}
