#include "tune.h"

#include <string.h>

Service g_tune;

Result smAtmosphereHasService(bool *out, SmServiceName name) {
    u8 tmp = 0;
    Result rc = serviceDispatchInOut(smGetServiceSession(), 65100, name, tmp);
    if (R_SUCCEEDED(rc) && out)
        *out = tmp;
    return rc;
}

Result tuneInitialize() {
    SmServiceName tune = smEncodeName("tune");
    bool exists;
    Result rc = smAtmosphereHasService(&exists, tune);
    if (R_SUCCEEDED(rc) && exists)
        rc = smGetServiceWrapper(&g_tune, tune);
    else
        rc = MAKERESULT(Module_Libnx, LibnxError_NotFound);
    return rc;
}

void tuneExit() {
    serviceClose(&g_tune);
}

Result tuneGetStatus(bool *status) {
    u8 tmp=0;
    Result rc = serviceDispatchOut(&g_tune, 0, tmp);
    if (R_SUCCEEDED(rc) && status) *status = tmp & 1;
    return rc;
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
    u8 out = 0;
    Result rc = serviceDispatchOut(&g_tune, 20, out);
    if (R_SUCCEEDED(rc) && state)
        *state = out;
    return rc;
}

Result tuneSetRepeatMode(TuneRepeatMode state) {
    u8 tmp = state;
    return serviceDispatchIn(&g_tune, 21, tmp);
}

Result tuneGetShuffleMode(TuneShuffleMode *state) {
    u8 out = 0;
    Result rc = serviceDispatchOut(&g_tune, 22, out);
    if (R_SUCCEEDED(rc) && state)
        *state = out;
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
                              .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                              .buffers = {{out_path, out_path_length}}, );
}

Result tuneGetCurrentQueueItem(char *out_path, size_t out_path_length, TuneCurrentStats *out) {
    return serviceDispatchOut(&g_tune, 32, *out,
                              .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                              .buffers = {{out_path, out_path_length}}, );
}

Result tuneClearQueue() {
    return serviceDispatch(&g_tune, 33);
}

Result tuneMoveQueueItem(u32 src, u32 dst) {
    const struct {
        u32 src;
        u32 dst;
    } in = {src, dst};
    return serviceDispatchIn(&g_tune, 34, in);
}

Result tuneSelect(u32 index) {
    return serviceDispatchIn(&g_tune, 35, index);
}

Result tuneSeek(u32 position) {
    return serviceDispatchIn(&g_tune, 36, position);
}

Result tuneEnqueue(const char *path, TuneEnqueueType type) {
    u8 tmp = type;
    size_t path_length = strlen(path);
    return serviceDispatchIn(&g_tune, 40, tmp,
                             .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                             .buffers = {{path, path_length}}, );
}

Result tuneRemove(u32 index) {
    return serviceDispatchIn(&g_tune, 41, index);
}

Result tuneQuit() {
    return serviceDispatch(&g_tune, 50);
}

Result tuneGetApiVersion(u32 *version) {
    return serviceDispatchOut(&g_tune, 5000, *version);
}
