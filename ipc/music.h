#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

typedef enum {
    MusicPlayerStatus_Stopped,
    MusicPlayerStatus_Playing,
    MusicPlayerStatus_Paused,
    MusicPlayerStatus_Next,
} MusicPlayerStatus;

typedef enum {
    MusicLoopStatus_Off,
    MusicLoopStatus_List,
    MusicLoopStatus_Single,
} MusicLoopStatus;

typedef struct {
    double volume;
    double tpf;
    double total_frame_count;
    double progress_frame_count;
} MusicCurrentTune;

bool musicIsRunning();

Result musicInitialize();

void musicExit();

/**
 * @brief Get the current status of playback.
 * @param[out] status \ref MusicPlayerStatus
 */
Result musicGetStatus(MusicPlayerStatus *status);

/**
 * @brief Set playback status.
 * @param[in] status \ref MusicPlayerStatus
 */
Result musicSetStatus(MusicPlayerStatus status);

/**
 * @brief Get the current decode volume.
 * @param[out] out volume value (linear factor).
 */
Result musicGetVolume(double *out);

/**
 * @brief Set the decode volume.
 * @param[in] volume volume value (linear factor).
 */
Result musicSetVolume(double volume);

/**
 * @brief Get the current loop status.
 * @param[out] out Is looping enabled?
 */
Result musicGetLoop(MusicLoopStatus *out);

/**
 * @brief Enable or disable looping.
 * @param[in] loop Whether to loop or not.
 */
Result musicSetLoop(MusicLoopStatus loop);

/**
 * @brief Get current song.
 * @param[out] out_path Path to current playing song.
 * @param[in] out_path_length Size of the out_path buffer. Needs to be at least FS_MAX_PATH.
 * @param[out] out \ref MusicCurrentTune
 */
Result musicGetCurrent(char *out_path, size_t out_path_length, MusicCurrentTune *out);

/**
 * @brief Get the current queue size.
 * @param[out] count remaining tracks after current.
 */
Result musicCountTunes(u32 *count);

/**
 * @brief Read queue.
 * @param[out] read Amount written to buffer.
 * @param[out] out_path Path array FS_MAX_PATH * n
 * @param[in] out_path_length Size of the supplied path array.
 */
Result musicListTunes(u32 *read, char *out_path, size_t out_path_length);

/**
 * @brief Add track to queue.
 * @note Must not include leading mount name.
 * @note Must match ^(/.*.mp3)$
 * @param[in] path Path to file on sdcard.
 */
Result musicAddToQueue(const char *path);

/**
 * @brief Clear queue.
 */
Result musicClear();

/**
 * @brief Shuffle queue.
 */
Result musicShuffle();

#ifdef __cplusplus
}
#endif
