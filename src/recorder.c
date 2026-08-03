#include "recorder.h"
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdatomic.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include "log.h"
#include "x11.h"

atomic_bool video_capturing_running_flag;
pthread_mutex_t video_capturing_lock;

void *thread_video_capturing(void *arg) {
  snipx_x11 *params = arg;
  while (atomic_load(&video_capturing_running_flag)) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    Status ok = XShmGetImage(params->display, params->window, params->shared_image, params->capture.screen_x, params->capture.screen_y, AllPlanes);
    if (!ok) {
      pthread_mutex_lock(&video_capturing_lock);
      log_print(params->logger, LOG_ERROR, "Cannot get image.\n");
      pthread_mutex_unlock(&video_capturing_lock);
    }
    compression_submit(params->capture.compression_ctx, params->shared_image->data);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    long sleep_ns = FRAME_NS(params->capture.framerate) - elapsed_ns;
    if (elapsed_ns < FRAME_NS(params->capture.framerate)) {
      struct timespec sleep_time = {
        .tv_sec = sleep_ns / 1e9,
        .tv_nsec = sleep_ns % (long)1e9
      };
      nanosleep(&sleep_time, 0);
    }
  }
  if (!atomic_load(&video_capturing_running_flag)) {
    pthread_mutex_lock(&video_capturing_lock);
    log_print(params->logger, LOG_INFO, "Video thread has been closed.\n");
    pthread_mutex_unlock(&video_capturing_lock);
  }
  return 0;
}
