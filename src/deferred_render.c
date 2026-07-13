#include "deferred_render.h"
#include "circular_array.h"
#include "limits.h"
#include "string.h"
#include "stdbool.h"

int dump_media_file(circular_array *ring_buffer, size_t buffer_index, char *path) {
  size_t start = 0;
  if (buffer_index > ring_buffer->capacity) start = buffer_index % ring_buffer->capacity;
  FILE *file = fopen(path, "wb");
  fwrite(ring_buffer->data+(start*ring_buffer->item_size), ring_buffer->item_size, ring_buffer->length-start, file);
  if (start > 0) fwrite(ring_buffer->data, ring_buffer->item_size, start, file);
  fclose(file);

  return 0;
}

video_components defer_video(circular_array *video_buffer, size_t buffer_index, audio_stream **streams, size_t streams_length, char *temp_directory, size_t group) {
  video_components components = {0};
  components.video_file = (char *)malloc(PATH_MAX);
  snprintf(components.video_file, PATH_MAX, "%s/%zu.raw", temp_directory, group);
  dump_media_file(video_buffer, buffer_index, components.video_file);

  for (size_t i = 0; i < streams_length; ++i) {
    audio_stream *stream = streams[i];
    char *audio_filename = (char *)malloc(PATH_MAX);
    snprintf(audio_filename, PATH_MAX, "%s/%zu_%zu.raw", temp_directory, group, i);
    dump_media_file(&stream->ring_buffer, stream->buffer_index, audio_filename);
    components.audio_files[components.audio_files_length++] = audio_filename;
  }

  return components;
}

defer_video_args *alloc_defer_video_args(circular_array *video_buffer, size_t buffer_index, audio_stream **streams, size_t streams_length, char *temp_directory, size_t group, video_components *components) {
  defer_video_args *video_args = malloc(sizeof(defer_video_args));

  if (!video_args) {
    return NULL;
  }

  video_args->buffer_index = buffer_index;
  video_args->temp_directory = strdup(temp_directory);
  video_args->group = group;
  video_args->components = components;
  video_args->streams_length = streams_length;
  video_args->streams = calloc(streams_length, sizeof(audio_stream *));

  if (!video_args->streams) {
    goto fail;
  }

  video_args->video_buffer = circular_array_dup(video_buffer);
  if (!video_args->video_buffer) {
    goto fail;
  }

  for (size_t i = 0; i < video_args->streams_length; ++i) {
    video_args->streams[i] = malloc(sizeof(audio_stream));

    if (!video_args->streams[i]) {
      goto fail;
    }

    circular_array *tmp = circular_array_dup(&streams[i]->ring_buffer);
    if (!tmp) {
      goto fail;
    }
    video_args->streams[i]->ring_buffer = *tmp;
    video_args->streams[i]->buffer_index = streams[i]->buffer_index;
    free(tmp);
  }

  return video_args;

  fail:
  if (video_args->streams) {
    for (size_t i = 0; i < streams_length; ++i) {
      circular_array_free(&video_args->streams[i]->ring_buffer);
      free(video_args->streams[i]);
    }
    free(video_args->streams);
  }
  circular_array_free(video_args->video_buffer);
  free(video_args->video_buffer);
  free(video_args);

  return NULL;
}

void free_defer_video_args(defer_video_args *args) {
  if (!args) return;
  free(args->temp_directory);
  circular_array_free(args->video_buffer);
  for (size_t i = 0; i < args->streams_length; ++i) {
    circular_array_free(&args->streams[i]->ring_buffer);
    free(args->streams[i]);
  }
  free(args->streams);
  free(args);
}

void *thread_defer_video(void *args) {
  defer_video_args *video_args = args;

  *video_args->components = defer_video(video_args->video_buffer, video_args->buffer_index, video_args->streams, video_args->streams_length, video_args->temp_directory, video_args->group);

  free_defer_video_args(video_args);

  return NULL;
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
