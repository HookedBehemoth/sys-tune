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

bool musicIsRunning();

Result musicInitialize();

void musicExit();

/**
 * @brief returns the current status of playback.
 * @param[out] status \ref MusicPlayerStatus
 */
Result musicGetStatus(MusicPlayerStatus *status);

/**
 * @brief Set playback status.
 * @param[in] status \ref MusicPlayerStatus
 */
Result musicSetStatus(MusicPlayerStatus status);

/**
 * @brief Get the current queue size.
 * @param[out] count remaining tracks after current.
 */
Result musicGetQueueCount(u32 *count);

/**
 * @brief Get current song.
 * @param[out] out_path Path to current playing song.
 */
Result musicGetCurrent(char *out_path, size_t out_path_length);

/**
 * @brief Read queue.
 * @param[out] read Amount written to buffer.
 * @param[out] out_path Path array FS_MAX_PATH * n
 * @param[in] out_path_length Size of the supplied path array.
 */
Result musicGetList(u32 *read, char *out_path, size_t out_path_length);

/**
 * @brief Get length of current track.
 * @note Will only work if song is running or paused while running.
 * @param[out] out Length in seconds.
 */
Result musicGetCurrentLength(double *out);

/**
 * @brief Get progress of current track.
 * @note Will only work if song is running or paused while running.
 * @param[out] out Progress in seconds from start.
 */
Result musicGetCurrentProgress(double *out);

/**
 * @brief Get the current decode volume.
 * @param[out] out volume value (linear factor).
 */
Result musicGetVolume(double *out);

/**
 * @brief Set the current decode volume.
 * @param[in] volume volume value (linear factor).
 */
Result musicSetVolume(double volume);

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

#ifdef __cplusplus
}
#endif
