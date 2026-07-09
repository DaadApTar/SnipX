#ifndef DEFERRED_RENDER_H_
#define DEFERRED_RENDER_H_

#include <stdlib.h>
#include "circular_array.h"
#include "recorder.h"
#include "log.h"

#define AUDIO_FILES_CAPACITY 8

typedef struct {
  char *video_file;
  char *audio_files[AUDIO_FILES_CAPACITY];
  size_t audio_files_length;
} video_components;

/** @brief dumps raw circular array into a binary file.
 *  @param[in] ring_buffer buffer to dump
 *  @param[in] buffer_index index of the *last* element
 *  @param[in] items_amount amount of items of buffer to write
 *  @param[in] path path to write into
 *  @return 0 on success, -1 on error.
 */
int dump_media_file(circular_array ring_buffer, size_t buffer_index, size_t items_amount, char *path);

/** @brief packs files.
 *  @param[in] video_buffer video ring buffer
 *  @param[in] buffer_index index of the *last* frame
 *  @param[in] items_amount amount of items of video buffer
 *  @param[in] capture audio streams
 *  @param[in] temp_directory directory to save files
 *  @param[in] group id of group to store
 *  @return packed video components.
 */
video_components defer_video(circular_array video_buffer, size_t buffer_index, size_t items_amount, audio_capture capture, char *temp_directory, size_t group);

/** @brief deletes temp media files and frees structure
 *  @param[in,out] components
 *  @return 0 on success, -1 on error
 */
int delete_video_components(video_components *components);

/** @brief frees structure
 *  @param[out] components
 */
void free_video_components(video_components *components);

#endif
