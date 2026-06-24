#include "recorder.h"
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdatomic.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "log.h"

pthread_mutex_t lock;
circular_array video_ring_buffer;
circular_array audio_ring_buffer;
atomic_bool running_flag;
atomic_int frame_counter;
atomic_int video_frame_counter, audio_frame_counter;

void *thread_video_capturing(void *arg) {
  video_capturing_params *params = (video_capturing_params *)arg;
  long i = 0;
  while (atomic_load(&running_flag)) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    Status ok = XShmGetImage(params->display, params->window, params->shared_image, params->screen_x, params->screen_y, AllPlanes);
    if (!ok) {
      pthread_mutex_lock(&lock);
      log_print(params->logger, LOG_ERROR, "Cannot get image.\n");
      pthread_mutex_unlock(&lock);
    }
    circular_array_push(&video_ring_buffer, params->shared_image->data, i);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    long sleep_ns = FRAME_NS(params->framerate) - elapsed_ns;
    if (elapsed_ns < FRAME_NS(params->framerate)) {
      struct timespec sleep_time = {
        .tv_sec = sleep_ns / 1e9,
        .tv_nsec = sleep_ns % (long)1e9
      };
      nanosleep(&sleep_time, 0);
    }
    i++;
  }
  if (!atomic_load(&running_flag)) {
    atomic_store(&video_frame_counter, i);
    pthread_mutex_lock(&lock);
    log_print(params->logger, LOG_INFO, "Video thread has been closed.\n");
    pthread_mutex_unlock(&lock);
  }
  return 0;
}

void *thread_audio_capturing(void *arg) {
  audio_capturing_params *params = (audio_capturing_params *)arg;
  int i = 0;
  int error;
  while (atomic_load(&running_flag)) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint8_t audio_buf[SNIPX_PA_AUDIO_BYTES_PER_FRAME(params->framerate)];
    if (pa_simple_read(params->simple, audio_buf, sizeof(audio_buf), &error) < 0) {
      pthread_mutex_lock(&lock);
      log_print(params->logger, LOG_ERROR, "Cannot read from PulseAudio: %s.\n", pa_strerror(error));
      pthread_mutex_unlock(&lock);
    }
    circular_array_push(&audio_ring_buffer, audio_buf, i);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    long sleep_ns = FRAME_NS(params->framerate) - elapsed_ns;
    if (elapsed_ns < FRAME_NS(params->framerate)) {
      struct timespec sleep_time = {
        .tv_sec = sleep_ns / 1e9,
        .tv_nsec = sleep_ns % (long)1e9
      };
      nanosleep(&sleep_time, 0);
    }
    i++;
  }
  if (!atomic_load(&running_flag)) {
    atomic_store(&audio_frame_counter, i);
    pthread_mutex_lock(&lock);
    log_print(params->logger, LOG_INFO, "Audio thread has been closed.\n");
    pthread_mutex_unlock(&lock);
  }
  return 0;
}
