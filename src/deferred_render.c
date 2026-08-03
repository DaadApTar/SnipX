#include "deferred_render.h"
#include "circular_array.h"
#include "limits.h"
#include "string.h"
#include "stdbool.h"
#include <unistd.h>
#include <dirent.h>
#include <time.h>

#define SECOND_US 1000000
#define DELAY_US 10000
#define BYTES_PER_MB (1024 * 1024)

int dump_media_file(dynamic_circular_array *ring_buffer, char *path, size_t mbps) {
  size_t start = 0;
  if (ring_buffer->next_index > ring_buffer->capacity) start = ring_buffer->next_index % ring_buffer->capacity;
  FILE *file = fopen(path, "wb");
  if (!file) return -1;

  size_t bytes_per_us = 0;
  if (mbps != 0) {
    bytes_per_us = mbps * BYTES_PER_MB / SECOND_US;
    if (bytes_per_us == 0) bytes_per_us = 1;
  }
  struct timespec t0;
  clock_gettime(CLOCK_MONOTONIC, &t0);

  size_t total_written = 0;
  for (size_t i = 0; i < ring_buffer->items_length; ++i) {
    size_t framesize = 0;
    void *d = dynamic_circular_array_get(ring_buffer, start+i, &framesize);
    fwrite(d, framesize, 1, file);
    total_written += framesize;

    if (bytes_per_us == 0) continue;

    size_t target_us = total_written / bytes_per_us;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    long elapsed_us = (now.tv_sec - t0.tv_sec) * 1000000L + (now.tv_nsec - t0.tv_nsec) / 1000L;
    long sleep_us = (long) target_us - elapsed_us;

    if (sleep_us > 0) {
      fflush(file);
      long capped = sleep_us < 200000L ? 200000L : sleep_us;
      usleep((useconds_t)capped);
    }
  }
  fclose(file);

  return 0;
}

video_components defer_video(dynamic_circular_array *video_buffer, audio_stream **streams, size_t streams_length, char *temp_directory, size_t group, size_t mbps) {
  (void) streams;
  (void) streams_length;
  video_components components = {0};
  components.video_file = (char *)malloc(PATH_MAX);
  snprintf(components.video_file, PATH_MAX, "%s/%zu.raw", temp_directory, group);
  dump_media_file(video_buffer, components.video_file, mbps);

  for (size_t i = 0; i < streams_length; ++i) {
    audio_stream *stream = streams[i];
    char *audio_filename = (char *)malloc(PATH_MAX);
    snprintf(audio_filename, PATH_MAX, "%s/%zu_%zu.raw", temp_directory, group, i);
    dump_media_file(&stream->ring_buffer, audio_filename, mbps);
    components.audio_files[components.audio_files_length++] = audio_filename;
  }

  return components;
}

defer_video_args *alloc_defer_video_args(dynamic_circular_array *video_buffer, audio_stream **streams, size_t streams_length, char *temp_directory, size_t group, size_t mbps, video_components *components) {
  defer_video_args *video_args = malloc(sizeof(defer_video_args));

  if (!video_args) {
    return NULL;
  }

  video_args->temp_directory = strdup(temp_directory);
  video_args->group = group;
  video_args->components = components;
  video_args->streams_length = streams_length;
  video_args->streams = calloc(streams_length, sizeof(audio_stream *));
  video_args->mbps = mbps;

  if (!video_args->streams) {
    goto fail;
  }

  video_args->video_buffer = dynamic_circular_array_dup(video_buffer);
  if (!video_args->video_buffer) {
    goto fail;
  }

  for (size_t i = 0; i < video_args->streams_length; ++i) {
    video_args->streams[i] = malloc(sizeof(audio_stream));

    if (!video_args->streams[i]) {
      goto fail;
    }

    dynamic_circular_array *tmp = dynamic_circular_array_dup(&streams[i]->ring_buffer);
    if (!tmp) {
      goto fail;
    }
    video_args->streams[i]->ring_buffer = *tmp;
    free(tmp);
  }

  return video_args;

  fail:
  if (video_args->streams) {
    for (size_t i = 0; i < streams_length; ++i) {
      dynamic_circular_array_free(&video_args->streams[i]->ring_buffer);
      free(video_args->streams[i]);
    }
    free(video_args->streams);
  }
  dynamic_circular_array_free(video_args->video_buffer);
  free(video_args->video_buffer);
  free(video_args);

  return NULL;
}

void free_defer_video_args(defer_video_args *args) {
  if (!args) return;
  free(args->temp_directory);
  dynamic_circular_array_free(args->video_buffer);
  for (size_t i = 0; i < args->streams_length; ++i) {
    dynamic_circular_array_free(&args->streams[i]->ring_buffer);
    free(args->streams[i]);
  }
  free(args->streams);
  free(args);
}

void *thread_defer_video(void *args) {
  defer_video_args *video_args = args;

  *video_args->components = defer_video(video_args->video_buffer, video_args->streams, video_args->streams_length, video_args->temp_directory, video_args->group, video_args->mbps);

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

size_t load_deferred_files(char *path, video_components *components, size_t capacity) {
  DIR *d;
  struct dirent *file;
  d = opendir(path);
  if (d == 0) return -1;
  size_t result = 0;
  while ((file = readdir(d)) != 0) {
    if (file->d_type == DT_REG && strstr(file->d_name, ".raw") != 0) {
      char *group_end;
      size_t group = strtoull(file->d_name, &group_end, 10);

      if (file->d_name == group_end) continue;
      if (group > capacity) continue;
      if (group >= result) result = group+1;
      if (strcmp(group_end, ".raw") == 0) {
        char *filepath = malloc(PATH_MAX);
        snprintf(filepath, PATH_MAX, "%s%s", path, file->d_name);
        components[group].video_file = filepath;
      }
      else if (*group_end == '_') {
        group_end++;
        char *index_end;
        size_t audio_index = strtoull(group_end, &index_end, 10);

        if (strcmp(index_end, ".raw") == 0) {
          char *filepath = malloc(PATH_MAX);
          snprintf(filepath, PATH_MAX, "%s%s", path, file->d_name);
          components[group].audio_files[audio_index] = filepath;
          components[group].audio_files_length++;
        }
      }
    }
  }
  return result;
}
