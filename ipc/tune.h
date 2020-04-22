#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

typedef enum {
    TuneShuffleMode_Off,
    TuneShuffleMode_On,
} TuneShuffleMode;

typedef enum {
    TuneRepeatMode_Off,
    TuneRepeatMode_One,
    TuneRepeatMode_All,
} TuneRepeatMode;

typedef enum {
    TuneEnqueueType_Next,
    TuneEnqueueType_Last,
} TuneEnqueueType;

typedef struct {
    u32 sample_rate;
    u32 current_frame;
    u32 total_frames;
} TuneCurrentStats;

Result tuneInitialize();

void tuneExit();

/**
 * @brief Get the current status of playback.
 * @param[out] status \ref AudioOutState
 */
Result tuneGetStatus(bool *status);

Result tunePlay();
Result tunePause();
Result tuneNext();
Result tunePrev();

/**
 * @brief Get the current playback volume.
 * @note On FW lower than [6.0.0] this will set the decode volume.
 * @param[out] out volume value (linear factor).
 */
Result tuneGetVolume(float *out);

/**
 * @brief Set the playback volume.
 * @note On FW lower than [6.0.0] this will return the decode volume.
 * @param[in] volume volume value (linear factor).
 */
Result tuneSetVolume(float volume);

/**
 * @brief Get the current loop status.
 * @param[out] state \ref TuneRepeatMode
 */
Result tuneGetRepeatMode(TuneRepeatMode *state);

/**
 * @brief Set repeat mode.
 * @param[in] state \ref TuneRepeatMode
 */
Result tuneSetRepeatMode(TuneRepeatMode state);

Result tuneGetShuffleMode(TuneShuffleMode *state);
Result tuneSetShuffleMode(TuneShuffleMode state);

/**
 * @brief Get the current queue size.
 * @param[out] count remaining tracks after current.
 */
Result tuneGetCurrentPlaylistSize(u32 *count);

/**
 * @brief Read queue.
 * @param[out] read Amount written to buffer.
 * @param[out] out_path Path array FS_MAX_PATH * n
 * @param[in] out_path_length Size of the supplied path array.
 */
Result tuneGetCurrentPlaylist(u32 *read, char *out_path, size_t out_path_length);

/**
 * @brief Get current song.
 * @param[out] out_path Path to current playing song.
 * @param[in] out_path_length Size of the out_path buffer. Path of the current track needs to fit.
 * @param[out] out \ref MusicCurrentTune
 */
Result tuneGetCurrentQueueItem(char *out_path, size_t out_path_length, TuneCurrentStats *out);

/**
 * @brief Clear queue.
 */
Result tuneClearQueue();
Result tuneMoveQueueItem(u32 src, u32 dst);
Result tuneSelect(u32 index);
Result tuneSeek(u32 position);

/**
 * @brief Add track to queue.
 * @note Must not include leading mount name.
 * @note Must match ^(/.*.mp3)$
 * @param[in] path Path to file on sdcard.
 */
Result tuneEnqueue(const char *path, TuneEnqueueType type);

Result tuneRemove(u32 index);

Result tuneGetApiVersion(u32 *version);

#ifdef __cplusplus
}
#endif
