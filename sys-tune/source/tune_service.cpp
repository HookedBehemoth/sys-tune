#include "tune_service.hpp"

#include "impl/music_player.hpp"
#include "ipc_cmd.h"
#include "tune_result.hpp"
#include "tune_types.hpp"

#include <nxExt.h>

#define GET_SINGLE(type, expr)            \
    ({                                    \
        *out_dataSize     = sizeof(type); \
        *(type *)out_data = expr();       \
        return 0;                         \
    })

#define SET_SINGLE(type, expr)              \
    ({                                      \
        if (r->data.size >= sizeof(type)) { \
            expr(*(type *)r->data.ptr);     \
            return 0;                       \
        }                                   \
        break;                              \
    })

namespace tune {

    namespace {

        IpcServer g_server;
        bool running = true;

        Result ServiceHandlerFunc(void *, const IpcServerRequest *r, u8 *out_data, size_t *out_dataSize) {
            switch (r->data.cmdId) {
                case TuneIpcCmd_GetStatus:
                    GET_SINGLE(bool, impl::GetStatus);

                case TuneIpcCmd_Play:
                    impl::Play();
                    return 0;

                case TuneIpcCmd_Pause:
                    impl::Pause();
                    return 0;

                case TuneIpcCmd_Next:
                    impl::Next();
                    return 0;

                case TuneIpcCmd_Prev:
                    impl::Prev();
                    return 0;

                case TuneIpcCmd_GetVolume:
                    GET_SINGLE(float, impl::GetVolume);

                case TuneIpcCmd_SetVolume:
                    SET_SINGLE(float, impl::SetVolume);

                case TuneIpcCmd_GetTitleVolume:
                    GET_SINGLE(float, impl::GetTitleVolume);

                case TuneIpcCmd_SetTitleVolume:
                    SET_SINGLE(float, impl::SetTitleVolume);

                case TuneIpcCmd_GetDefaultTitleVolume:
                    GET_SINGLE(float, impl::GetDefaultTitleVolume);

                case TuneIpcCmd_SetDefaultTitleVolume:
                    SET_SINGLE(float, impl::SetDefaultTitleVolume);

                case TuneIpcCmd_GetRepeatMode:
                    GET_SINGLE(RepeatMode, impl::GetRepeatMode);

                case TuneIpcCmd_SetRepeatMode:
                    SET_SINGLE(RepeatMode, impl::SetRepeatMode);

                case TuneIpcCmd_GetShuffleMode:
                    GET_SINGLE(ShuffleMode, impl::GetShuffleMode);

                case TuneIpcCmd_SetShuffleMode:
                    SET_SINGLE(ShuffleMode, impl::SetShuffleMode);

                case TuneIpcCmd_GetPlaylistSize:
                    GET_SINGLE(u32, impl::GetPlaylistSize);

                case TuneIpcCmd_GetPlaylistItem:
                    if (r->hipc.meta.num_recv_buffers >= 1 && r->data.size >= sizeof(u32)) {
                        return impl::GetPlaylistItem(
                            *(u32 *)r->data.ptr,
                            (char *)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers));
                    }
                    break;

                case TuneIpcCmd_GetCurrentQueueItem:
                    if (r->hipc.meta.num_recv_buffers >= 1) {
                        *out_dataSize = sizeof(CurrentStats);
                        return impl::GetCurrentQueueItem(
                            (CurrentStats *)out_data,
                            (char *)hipcGetBufferAddress(r->hipc.data.recv_buffers),
                            hipcGetBufferSize(r->hipc.data.recv_buffers));
                    }
                    break;

                case TuneIpcCmd_ClearQueue:
                    impl::ClearQueue();
                    return 0;

                case TuneIpcCmd_MoveQueueItem:
                    if (r->data.size >= 2 * sizeof(u32)) {
                        u32 *data = (u32 *)r->data.ptr;
                        impl::MoveQueueItem(data[0], data[1]);
                        return 0;
                    }
                    break;

                case TuneIpcCmd_Select:
                    SET_SINGLE(u32, impl::Select);

                case TuneIpcCmd_Seek:
                    SET_SINGLE(u32, impl::Seek);

                case TuneIpcCmd_Enqueue:
                    if (r->hipc.meta.num_send_buffers >= 1 && r->data.size >= sizeof(EnqueueType)) {
                        return impl::Enqueue(
                            (const char *)hipcGetBufferAddress(r->hipc.data.send_buffers),
                            hipcGetBufferSize(r->hipc.data.send_buffers),
                            *(EnqueueType *)r->data.ptr);
                    }
                    break;

                case TuneIpcCmd_Remove:
                    SET_SINGLE(u32, impl::Remove);

                case TuneIpcCmd_QuitServer:
                    running = false;
                    return 0;

                case TuneIpcCmd_GetApiVersion:
                    *out_dataSize    = sizeof(u32);
                    *(u32 *)out_data = TUNE_API_VERSION;
                    return 0;
            }
            return tune::Generic;
        }

    }

    Result InitializeServer() {
        return ipcServerInit(&g_server, "tune", 2);
    }

    Result ExitServer() {
        return ipcServerExit(&g_server);
    }

    void LoopProcess() {
        while (running) {
            if (ipcServerProcess(&g_server, ServiceHandlerFunc, nullptr) == KERNELRESULT(Cancelled))
                break;
        }
    }

}
