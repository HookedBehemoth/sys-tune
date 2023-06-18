#pragma once

enum TuneIpcCmd {
    TuneIpcCmd_GetStatus = 0,
    TuneIpcCmd_Play = 1,
    TuneIpcCmd_Pause = 2,
    TuneIpcCmd_Next = 3,
    TuneIpcCmd_Prev = 4,

    TuneIpcCmd_GetVolume = 10,
    TuneIpcCmd_SetVolume = 11,
    TuneIpcCmd_GetTitleVolume = 12,
    TuneIpcCmd_SetTitleVolume = 13,
    TuneIpcCmd_GetDefaultTitleVolume = 14,
    TuneIpcCmd_SetDefaultTitleVolume = 15,

    TuneIpcCmd_GetRepeatMode = 20,
    TuneIpcCmd_SetRepeatMode = 21,
    TuneIpcCmd_GetShuffleMode = 22,
    TuneIpcCmd_SetShuffleMode = 23,

    TuneIpcCmd_GetPlaylistSize = 30,
    TuneIpcCmd_GetPlaylistItem = 31,
    TuneIpcCmd_GetCurrentQueueItem = 32,
    TuneIpcCmd_ClearQueue = 33,
    TuneIpcCmd_MoveQueueItem = 34,
    TuneIpcCmd_Select = 35,
    TuneIpcCmd_Seek = 36,

    TuneIpcCmd_Enqueue = 40,
    TuneIpcCmd_Remove = 41,

    TuneIpcCmd_QuitServer = 50,

    TuneIpcCmd_GetApiVersion = 5000,
};
