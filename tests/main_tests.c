#include <X11/X.h>
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

#include "circular_array.h"

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

void test_Xvideo() {
  circular_array *array = circular_array_init(600, sizeof(XImage *));
  Display *display = XOpenDisplay(0);
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

  for (int i = 0; i < 600; ++i) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    XImage *image = XGetImage(display, root, screen_x, screen_y, screen_width, screen_height, AllPlanes, ZPixmap);
    if (!image) {
      fprintf(stderr, RED"Cannot get image.\n"RESET);
      return;
    }
    circular_array_push(array, image, i);

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

  // FFmpeg part

  int pipefd[2];

  if (pipe(pipefd) < 0) {
    fprintf(stderr, RED"Cannot create pipe.\n"RESET);
    return;
  }
  
  pid_t ffmpeg = fork();
  if (ffmpeg < 0) {
    fprintf(stderr, RED"Cannot create fork as a child: %s\n"RESET, strerror(errno));
    return;
  }

  char resolution[32];
  snprintf(resolution, sizeof(resolution), "%dx%d", screen_width, screen_height);

  if (ffmpeg == 0) {
    if (dup2(pipefd[0], STDIN_FILENO) < 0) {
      fprintf(stderr, RED"Cannot reopen read end of pipe as stdin.\n"RESET);
      return;
    }
    close(pipefd[1]);
    int status_code = execlp("ffmpeg",
                             "ffmpeg",
                             "-loglevel", "verbose",
                             "-y",

                             "-f", "rawvideo",
                             "-pix_fmt", "bgr0",
                             "-s", resolution,
                             "-r", "60",
                             "-i", "-",

                             "-c:v", "libx264",
                             "-vb", "2500k",
                             "-c:a", "aac",
                             "-ab", "200k",
                             "-pix_fmt", "yuv420p",
                             "test_xvideo.mp4", (char *)NULL);

    if (status_code < 0) {
      fprintf(stderr, RED"Cannot run ffmpeg as a child process: %s\n"RESET, strerror(errno));
      return;
    }
  }

  close(pipefd[0]);

  XImage *frame;
  for (int i = 0; i < 600; ++i) {
    circular_array_get(array, i, (void *)&frame);
    write(pipefd[1], frame->data, sizeof(uint32_t) * screen_width * screen_height);
  }
  close(pipefd[1]);
  waitpid(ffmpeg, 0, 0);

  XFree(screens);
  XCloseDisplay(display);
}

int main() {
  test_circular_array();
  test_Xscreenshot();
  test_Xvideo();
  return 0;
}
