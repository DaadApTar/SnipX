#ifndef DEFERRED_RENDER_H_
#define DEFERRED_RENDER_H_

#include <stdlib.h>
#include "circular_array.h"
#include "recorder.h"
#include "log.h"
#include "pthread.h"
#include "stdbool.h"

#define AUDIO_FILES_CAPACITY 8

typedef struct {
  char *video_file;
  char *audio_files[AUDIO_FILES_CAPACITY];
  size_t audio_files_length;
} video_components;

/** @brief dumps raw circular array into a binary file.
 *  @param[in] ring_buffer buffer to dump
 *  @param[in] buffer_index index of the *last* element
 *  @param[in] path path to write into
 *  @return 0 on success, -1 on error.
 */
int dump_media_file(circular_array *ring_buffer, size_t buffer_index, char *path);

/** @brief packs files.
 *  @param[in] video_buffer video ring buffer
 *  @param[in] buffer_index index of the *last* frame
 *  @param[in] capture audio streams
 *  @param[in] temp_directory directory to save files
 *  @param[in] group id of group to store
 *  @return packed video components.
 */
video_components defer_video(circular_array *video_buffer, size_t buffer_index, audio_stream **streams, size_t stream_length, char *temp_directory, size_t group);

typedef struct {
  circular_array *video_buffer;
  size_t buffer_index;
  audio_stream **streams;
  size_t streams_length;
  char *temp_directory;
  size_t group;
  video_components *components;
} defer_video_args;

/** @brief allocates args on a heap.
 *  @param[in] video_buffer video ring buffer
 *  @param[in] buffer_index index of the *last* frame
 *  @param[in] streams audio streams
 *  @param[in] streams_length amount of streams
 *  @param[in] temp_directory directory to save files
 *  @param[in] group id of group to store
 *  @param[in] components pointer to array item
 *  @note you should not free it manually if you pass them to thread
 *  @return pointer to new args if succeed, NULL on error.
 */
defer_video_args *alloc_defer_video_args(circular_array *video_buffer, size_t buffer_index, audio_stream **streams, size_t streams_length, char *temp_directory, size_t group, video_components *components);

void free_defer_video_args(defer_video_args *args);

void *thread_defer_video(void *args);

/** @brief deletes temp media files and frees structure
 *  @param[in,out] components
 *  @return 0 on success, -1 on error
 */
int delete_video_components(video_components *components);

/** @brief frees structure
 *  @param[out] components
 */
void free_video_components(video_components *components);


/** @brief Loads forgotten deferred files into directory.
 *  @param[in] path path to temp directory
 *  @param[in] components pointer to array
 *  @param[in] capacity array capacity
 *  @return next free group index
 */
size_t load_deferred_files(char *path, video_components *components, size_t capacity);

#endif
