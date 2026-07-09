#include "deferred_render.h"
#include "circular_array.h"
#include "limits.h"

int dump_media_file(circular_array ring_buffer, size_t buffer_index, size_t items_amount, char *path) {
  FILE *file = fopen(path, "wb");

  size_t start = 0;
  if (buffer_index > items_amount) start = buffer_index - items_amount;

  void *data;
  for (size_t i = start; i < buffer_index; ++i) {
    data = circular_array_get(&ring_buffer, i);
    if(fwrite(data, ring_buffer.size, 1, file) != 1) {
      fclose(file);
      return -1;
    }
  }
  fclose(file);

  return 0;
}

video_components defer_video(circular_array video_buffer, size_t buffer_index, size_t items_amount, audio_capture capture, char *temp_directory, size_t group) {
  (void) video_buffer;
  (void) buffer_index;
  (void) items_amount;
  video_components components = {0};
  components.video_file = (char *)malloc(PATH_MAX);
  snprintf(components.video_file, PATH_MAX, "%s/%zu.raw", temp_directory, group);
  dump_media_file(video_buffer, buffer_index, items_amount, components.video_file);

  for (size_t i = 0; i < capture.length; ++i) {
    audio_stream *stream = capture.streams[i];
    char *audio_filename = (char *)malloc(PATH_MAX);
    snprintf(audio_filename, PATH_MAX, "%s/%zu_%zu.raw", temp_directory, group, i);
    dump_media_file(stream->ring_buffer, stream->buffer_index, items_amount, audio_filename);
    components.audio_files[components.audio_files_length++] = audio_filename;
  }

  return components;
}

int delete_video_components(video_components *components) {
  if (remove(components->video_file) < 0) {
    return -1;
  }

  for (size_t i = 0; i < components->audio_files_length; ++i) {
    if (remove(components->audio_files[i]) < 0) {
      return -1;
    }
  }

  free_video_components(components);

  return 0;
}

void free_video_components(video_components *components) {
  free(components->video_file);
  for (size_t i = 0; i < components->audio_files_length; ++i) {
    free(components->audio_files[i]);
  }

  *components = (video_components){0};
}
