#include <X11/X.h>
#include <pulse/def.h>
#include <pulse/sample.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "assertation.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <time.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "circular_array.h"
#include "ffmpeg.h"

void test_circular_array() {
  test test = {.name = "circular array"};
  circular_array *array = circular_array_init(5, sizeof(int));
  for (int i = 0; i < 10; ++i) {
    int *value = malloc(sizeof(int));
    *value = i;
    circular_array_push(array, value, i);
  }
  int *dst;
  circular_array_get(array, 9, (void **)&dst);
  assert_int(&test, 9, *dst);
  circular_array_get(array, 5, (void **)&dst);
  assert_int(&test, 5, *dst);
  circular_array_get(array, 4, (void **)&dst);
  assert_int(&test, 9, *dst);
  circular_array_get(array, 0, (void **)&dst);
  assert_int(&test, 5, *dst);
  assert_done(&test);
}

void test_Xscreenshot() {
  Display *display = XOpenDisplay(0);
  if (!display) {
    fprintf(stderr, RED"Cannot open display.\n"RESET);
    return;
  }

  int major, minor;
  if (!XineramaQueryExtension(display, &major, &minor)) {
    fprintf(stderr, RED"Xinerama is not supported.\n"RESET);
    return;
  }

  if (!XineramaIsActive(display)) {
    fprintf(stderr, RED"Xinerama is not active.\n"RESET);
    return;
  }

  int num_screens = 0;
  XineramaScreenInfo *screens = XineramaQueryScreens(display, &num_screens);
  printf("Number of screens: %d\n", num_screens);
  for (int i = 0; i < num_screens; ++i) {
    printf("\tMonitor %d: x=%d y=%d width=%d height=%d\n", i, screens[i].x_org, screens[i].y_org, screens[i].width, screens[i].height);
  }

  int screen = DefaultScreen(display);
  Window root = DefaultRootWindow(display);

  int width = DisplayWidth(display, screen);
  int height = DisplayHeight(display, screen);

  printf("Width: %d\tHeight: %d\n", width, height);

  short screen_x = screens[0].x_org;
  short screen_y = screens[0].y_org;
  short screen_width = screens[0].width;
  short screen_height = screens[0].height;

  XImage *image = XGetImage(display, root, screen_x, screen_y, screen_width, screen_height, AllPlanes, ZPixmap);
  printf("BPP: %d, Red mask: 0x%lx, Green mask: 0x%lx, Blue mask: 0x%lx\n",
        image->bits_per_pixel, image->red_mask, image->green_mask, image->blue_mask);
 if (!image) {
    fprintf(stderr, RED"Cannot get image.\n"RESET);
    return;
  }

  FILE *file = fopen("test_x11.ppm", "wb");
  fprintf(file, "P6 %d %d 255\n", screen_width, screen_height);

  for (int y = 0; y < screen_height; ++y) {
    for(int x = 0; x < screen_width; ++x) {
      long pixel = XGetPixel(image, x, y);
      char r = (pixel & image->red_mask) >> 16;
      char g = (pixel & image->green_mask) >> 8;
      char b = (pixel & image->blue_mask);
      fwrite(&r, 1, 1, file);
      fwrite(&g, 1, 1, file);
      fwrite(&b, 1, 1, file);
    }
  }

  fclose(file);

  XDestroyImage(image);
  XFree(screens);
  XCloseDisplay(display);
}

#define FRAME_NS (long)((double)1/(double)60*1e9)
#define SAMPLE_RATE 44100
#define CHANNELS 2
#define BYTES_PER_SAMPLE 2
#define FPS 60
#define AUDIO_BYTES_PER_FRAME ((SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE) / FPS)

void test_Xvideo() {
  circular_array *video_buffer = circular_array_init(600, sizeof(XImage *));
  circular_array *audio_buffer = circular_array_init(600, sizeof(uint8_t *));
  Display *display = XOpenDisplay(0);
  static const pa_sample_spec sample_spec = {
    .format = PA_SAMPLE_S16LE,
    .rate = SAMPLE_RATE,
    .channels = CHANNELS
  };
  pa_simple *simple = 0;
  int error;

  if ((simple = pa_simple_new(0, "snipx", PA_STREAM_RECORD,
                              0, "record", &sample_spec, 0, 0, &error)) == 0) {
    fprintf(stderr, RED"Cannot create PulseAudio connection: %s.\n"RESET, pa_strerror(error));
    return;
  }
  int minor, major;
  if (!XineramaQueryExtension(display, &minor, &major)) {
    fprintf(stderr, RED"Xinerama is not supported.\n"RESET);
    return;
  }
  if (!XineramaIsActive(display)) {
    fprintf(stderr, RED"Xinerama is not active.\n"RESET);
    return;
  }

  int num_screens = 0;
  XineramaScreenInfo *screens = XineramaQueryScreens(display, &num_screens);
  Window root = DefaultRootWindow(display);
  short screen_x = screens[0].x_org;
  short screen_y = screens[0].y_org;
  short screen_width = screens[0].width;
  short screen_height = screens[0].height;

  // TODO: capturing the screen is very long. Use XShm
  for (int i = 0; i < 600; ++i) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint8_t *audio_buf = malloc(AUDIO_BYTES_PER_FRAME);
    XImage *image = XGetImage(display, root, screen_x, screen_y, screen_width, screen_height, AllPlanes, ZPixmap);
    if (!image) {
      fprintf(stderr, RED"Cannot get image.\n"RESET);
      return;
    }
    if (pa_simple_read(simple, audio_buf, AUDIO_BYTES_PER_FRAME, &error) < 0) {
      fprintf(stderr, RED"Cannot read from PulseAudio: %s.\n"RESET, pa_strerror(error));
      return;
    }
    circular_array_push(video_buffer, image, i);
    circular_array_push(audio_buffer, audio_buf, i);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    long sleep_ns = FRAME_NS - elapsed_ns;
    if (elapsed_ns < FRAME_NS) {
      struct timespec sleep_time = {
        .tv_sec = sleep_ns / 1e9,
        .tv_nsec = sleep_ns % (long)1e9
      };
      nanosleep(&sleep_time, 0);
    }
  }
  fclose(file);
  pa_simple_free(simple);

  // FFmpeg part

  ffmpeg *sound = ffmpeg_init_sound("test_xsound.aac");

  uint8_t *audio;
  for (int i = 0; i < 600; ++i) {
    circular_array_get(audio_buffer, i, (void *)&audio);
    ffmpeg_push_frame(sound, audio, AUDIO_BYTES_PER_FRAME);
  }
  free(audio);
  ffmpeg_close(sound);

  XImage *frame;
  ffmpeg *video = ffmpeg_init_video("test_xsound.aac", "test_xvideo.mp4", screen_width, screen_height, FPS);
  for (int i = 0; i < 600; ++i) {
    circular_array_get(video_buffer, i, (void *)&frame);
    ffmpeg_push_frame(video, frame->data, sizeof(uint32_t) * screen_width * screen_height);
  }
  ffmpeg_close(video);
  free(frame);

  XFree(screens);
  XCloseDisplay(display);
}

int main() {
  test_circular_array();
  test_Xscreenshot();
  test_Xvideo();
  return 0;
}
