#include "music.h"

#include <string.h>

Service g_music;

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
    if (R_SUCCEEDED(rc) && out)
        *out = tmp;
    return rc;
}

Result musicAddToQueue(const char *path) {
    size_t path_length = strlen(path);
    return serviceDispatch(&g_music, 3,
                           .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                           .buffers = {{path, path_length}}, );
}

Result musicGetNext(char *out_path, size_t out_path_length) {
    return serviceDispatch(&g_music, 4,
                           .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                           .buffers = {{out_path, out_path_length}}, );
}

Result musicGetLast(char *out_path, size_t out_path_length) {
    return serviceDispatch(&g_music, 5,
                           .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                           .buffers = {{out_path, out_path_length}}, );
}

Result musicClear() {
    return serviceDispatch(&g_music, 6);
}
