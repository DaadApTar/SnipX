#include <X11/X.h>
#include <pulse/def.h>
#include <pulse/sample.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "assertation.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include <unistd.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "circular_array.h"
#include "options.h"
#include "directory_manager.h"

void test_circular_array() {
  test test = {.name = "circular array"};
  circular_array array;
  circular_array_init(&array, 5, sizeof(int));
  for (int i = 0; i < 10; ++i) {
    int value = i;
    circular_array_push(&array, &value, i);
  }
  int *dst;
  dst = circular_array_get(&array, 9);
  assert_int(&test, 9, *dst);
  dst = circular_array_get(&array, 5);
  assert_int(&test, 5, *dst);
  dst = circular_array_get(&array, 4);
  assert_int(&test, 9, *dst);
  dst = circular_array_get(&array, 0);
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
#define RECORDING_FPS 60
#define AUDIO_BYTES_PER_FRAME ((SAMPLE_RATE * CHANNELS * BYTES_PER_SAMPLE) / RECORDING_FPS)

void test_Xvideo() {
  // FFmpeg part


  //XFree(screens);
  //XCloseDisplay(display);
}

void test_option_value_comparation() {
  test test = {.name = "option value comparation"};
  assert_int(&test, 1, get_value_type(UNKNOWN) == NONE);
  assert_int(&test, 1, get_value_type(SCREEN_NUMBER) == NUMBER);
  assert_int(&test, 1, get_value_type(WINDOW) == NONE);
  assert_int(&test, 1, get_value_type(FPS) == NUMBER);
  assert_int(&test, 1, get_value_type(SOUND_MONITOR) == STRING);
  assert_int(&test, 1, get_value_type(BRAKE) == NONE);
  assert_int(&test, 1, get_value_type(ADDRESS) == STRING);
  assert_int(&test, 1, get_value_type(PORT) == NUMBER);
  assert_int(&test, 0, get_value_type(BRAKE) == STRING);
  assert_int(&test, 0, get_value_type(SCREEN_NUMBER) == STRING);
  assert_int(&test, 0, get_value_type(FPS) == NONE);
  assert_int(&test, 1, get_value_type(LOCALLY) == NONE);
  assert_done(&test);
}

void test_parse_flag() {
  test test = {.name = "flag parsing"};
  assert_int(&test, SCREEN_NUMBER, parse_flag("--screen"));
  assert_int(&test, SCREEN_NUMBER, parse_flag("-s"));
  assert_int(&test, FPS, parse_flag("--fps"));
  assert_int(&test, FPS, parse_flag("-f"));
  assert_int(&test, SOUND_MONITOR, parse_flag("--monitor"));
  assert_int(&test, SOUND_MONITOR, parse_flag("-m"));
#ifdef FEATURE_SENDER
  assert_int(&test, ADDRESS, parse_flag("--address"));
  assert_int(&test, PORT, parse_flag("-p"));
#endif
  assert_int(&test, BRAKE, parse_flag("--brake"));
  assert_int(&test, BRAKE, parse_flag("-b"));
  assert_int(&test, UNKNOWN, parse_flag("--dfg"));
#ifdef FEATURE_SENDER
  assert_int(&test, LOCALLY, parse_flag("--local"));
#endif
  assert_done(&test);
}

void test_parse_value() {
  test test = {.name = "value parsing"};
  assert_int(&test, NUMBER, parse_value("34543"));
  assert_int(&test, NUMBER, parse_value("04343"));
  assert_int(&test, NUMBER, parse_value("23"));
  assert_int(&test, NUMBER, parse_value("345"));
  assert_int(&test, STRING, parse_value("0c4343"));
  assert_int(&test, STRING, parse_value("erter"));
  assert_int(&test, STRING, parse_value("345dsf"));
  assert_done(&test);
}

void test_parse_flags() {
  test test = {.name = "bunch of flags parsing"};
  char *args[] = {
    "--screen",
    "0",
    "-f",
    "60",
    "--monitor",
    "some_monitor",
    "-b",
    "-l",
    "20",
    "--bitrate",
    "2500000",
#ifdef FEATURE_SENDER
    "-p",
    "4227",
    "--address",
    "0.0.0.0",
    "--local",
#endif
  };
  options *opts = parse_flags(args, sizeof(args) / sizeof(args[0]));
  if (opts == 0) {
    fprintf(stderr, RED"Failed to parse opts.\n");
    return;
  }
  assert_int(&test, 0, opts->screen_number);
  assert_int(&test, 60, opts->fps);
  assert_int(&test, 1, opts->brake);
  assert_int(&test, 2500000, opts->bitrate);
  assert_int(&test, 20, opts->length);
  assert_int(&test, 0, strcmp("some_monitor", opts->sound_monitor));
#ifdef FEATURE_SENDER
  assert_int(&test, 4227, opts->port);
  assert_int(&test, 0, strcmp("0.0.0.0", opts->address));
  assert_int(&test, 1, opts->locally);
#endif
  assert_done(&test);
}

void test_parse_env_string() {
  test test = {.name = "Environment expanding test. Make sure to set environment variables on start."};
  assert_int(&test, 0, strcmp("Hello world", dir_expand_env("$ENV_TEST1 world")));
  assert_int(&test, 0, strcmp("123/test", dir_expand_env("$ENV_TEST2/test")));
  assert_done(&test);
}

int main() {
  test_circular_array();
  //test_Xscreenshot();
  //test_Xvideo();
  test_option_value_comparation();
  test_parse_flag();
  test_parse_value();
  test_parse_flags();
  test_parse_env_string();
  return 0;
}
