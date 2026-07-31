#include <X11/X.h>
#include <pulse/def.h>
#include <pulse/sample.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "assertation.h"
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include <unistd.h>

#include <pulse/simple.h>
#include <pulse/error.h>

#include "circular_array.h"
#include "options.h"
#include "directory_manager.h"

#include "dynamic_circular_array.h"
#include "compression.h"

#ifdef FEATURE_ZSTD
#include <zstd.h>
#endif
#ifdef FEATURE_LZ4
#include <lz4.h>
#endif

bool test_circular_array() {
  test test = {.name = "circular array"};
  circular_array array;
  circular_array_init(&array, 5, sizeof(int));
  for (int i = 0; i < 10; ++i) {
    int value = i;
    circular_array_push(&array, &value);
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
  return assert_done(&test);
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

bool test_option_value_comparation() {
  test test = {.name = "option value comparation"};
  assert_int(&test, 1, get_value_type(UNKNOWN) == NONE);
  assert_int(&test, 1, get_value_type(SCREEN_NUMBER) == NUMBER);
  /* assert_int(&test, 1, get_value_type(WINDOW) == NONE); */
  assert_int(&test, 1, get_value_type(FPS) == NUMBER);
  assert_int(&test, 1, get_value_type(DESKTOP_SOUND_MONITOR) == NUMBER);
  assert_int(&test, 1, get_value_type(IMMEDIATE) == NONE);
  assert_int(&test, 1, get_value_type(ADDRESS) == STRING);
  assert_int(&test, 1, get_value_type(PORT) == NUMBER);
  assert_int(&test, 0, get_value_type(IMMEDIATE) == STRING);
  assert_int(&test, 0, get_value_type(SCREEN_NUMBER) == STRING);
  assert_int(&test, 0, get_value_type(FPS) == NONE);
  assert_int(&test, 1, get_value_type(LOCALLY) == NONE);
  return assert_done(&test);
}

bool test_parse_flag() {
  test test = {.name = "flag parsing"};
  assert_int(&test, SCREEN_NUMBER, parse_flag("--screen"));
  assert_int(&test, SCREEN_NUMBER, parse_flag("-s"));
  assert_int(&test, FPS, parse_flag("--fps"));
  assert_int(&test, FPS, parse_flag("-f"));
  assert_int(&test, DESKTOP_SOUND_MONITOR, parse_flag("--desktop"));
  assert_int(&test, DESKTOP_SOUND_MONITOR, parse_flag("-d"));
#ifdef FEATURE_SENDER
  assert_int(&test, ADDRESS, parse_flag("--address"));
  assert_int(&test, PORT, parse_flag("-p"));
#endif
  assert_int(&test, IMMEDIATE, parse_flag("--immediate"));
  assert_int(&test, IMMEDIATE, parse_flag("-i"));
  assert_int(&test, UNKNOWN, parse_flag("--dfg"));
#ifdef FEATURE_SENDER
  assert_int(&test, LOCALLY, parse_flag("--local"));
#endif
  return assert_done(&test);
}

bool test_parse_value() {
  test test = {.name = "value parsing"};
  assert_int(&test, NUMBER, parse_value("34543"));
  assert_int(&test, NUMBER, parse_value("04343"));
  assert_int(&test, NUMBER, parse_value("23"));
  assert_int(&test, NUMBER, parse_value("345"));
  assert_int(&test, STRING, parse_value("0c4343"));
  assert_int(&test, STRING, parse_value("erter"));
  assert_int(&test, STRING, parse_value("345dsf"));
  return assert_done(&test);
}

bool test_parse_flags() {
  test test = {.name = "bunch of flags parsing"};
  char *args[] = {
    "--screen",
    "0",
    "-f",
    "60",
    "--desktop",
    "60",
    "-i",
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
    fprintf(stderr, RED"Failed to parse opts.\n"RESET);
    return false;
  }
  assert_int(&test, 0, opts->screen_number);
  assert_int(&test, 60, opts->fps);
  assert_int(&test, 1, opts->immediate);
  assert_int(&test, 2500000, opts->bitrate);
  assert_int(&test, 20, opts->length);
  assert_int(&test, 60, opts->desktop_sound_monitor);
#ifdef FEATURE_SENDER
  assert_int(&test, 4227, opts->port);
  assert_int(&test, 0, strcmp("0.0.0.0", opts->address));
  assert_int(&test, 1, opts->locally);
#endif
  return assert_done(&test);
}

bool test_parse_env_string() {
  test test = {.name = "Environment expanding test. Make sure to set environment variables on start."};
  assert_int(&test, 0, strcmp("Hello world", dir_expand_env("$ENV_TEST1 world")));
  assert_int(&test, 0, strcmp("123/test", dir_expand_env("$ENV_TEST2/test")));
  return assert_done(&test);
}

bool test_dynamic_circular_array() {
  test test = {.name = "Circular array test."};
  dynamic_circular_array array;
  int idata1 = 4345;
  const char *cdata2 = "12345345646456564";
  unsigned long uldata3 = 454353342;
  const char *cdata4 = "1234567890123456789012345678901234567890";
  size_t initial_capacity = 8;
  printf("--------Resizing--------\n");
  assert_int(&test, 0, dynamic_circular_array_init(&array, initial_capacity, 4));
  assert_int(&test, initial_capacity, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &idata1, sizeof(idata1)));
  assert_int(&test, initial_capacity, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2) + 1));
  assert_int(&test, initial_capacity*4, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2) + 1));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &uldata3, sizeof(uldata3)));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &uldata3, sizeof(uldata3)));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &uldata3, sizeof(uldata3)));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2) + 1));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2) + 1));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &uldata3, sizeof(uldata3)));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &uldata3, sizeof(uldata3)));
  assert_int(&test, initial_capacity*8, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, &idata1, sizeof(idata1)));
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata4, strlen(cdata4) + 1));
  printf("-----Final size test-----\n");
  assert_int(&test, 1, array.capacity <= 64);
  size_t size;
  void *got_data;
  printf("------Getting data------\n");
  got_data = dynamic_circular_array_get(&array, array.last_index-3, &size);
  assert_int(&test, sizeof(uldata3), size);
  assert_int(&test, uldata3, *(unsigned long *)got_data);
  free(got_data);
  got_data = dynamic_circular_array_get(&array, array.last_index-2, &size);
  assert_int(&test, sizeof(uldata3), size);
  assert_int(&test, uldata3, *(unsigned long *)got_data);
  free(got_data);
  got_data = dynamic_circular_array_get(&array, array.last_index-1, &size);
  assert_int(&test, sizeof(idata1), size);
  assert_int(&test, idata1, *(int *)got_data);
  free(got_data);
  got_data = dynamic_circular_array_get(&array, array.last_index, &size);
  char *string = malloc(size);
  strcpy(string, got_data);
  assert_int(&test, strlen(cdata4) + 1, size);
  assert_int(&test, 0, strcmp(cdata4, string));
  free(got_data);
  free(string);

  printf("------Resizing again------\n");
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2) + 1));
  assert_int(&test, initial_capacity*12, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2)));
  assert_int(&test, initial_capacity*12, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata2, strlen(cdata2) + 1));
  assert_int(&test, initial_capacity*12, array.capacity);
  assert_int(&test, 0, dynamic_circular_array_push(&array, (void *)cdata4, strlen(cdata4) + 1));
  printf("-----Final size test-----\n");
  assert_int(&test, 1, array.capacity > 64 && array.capacity < 128);

  dynamic_circular_array_free(&array);
  
  return assert_done(&test);
}

bool test_compression() {
  test test = {.name = "Compression test."};

  assert_int(&test, COMPRESSION_NONE, dispatch_string(NULL));
  assert_int(&test, COMPRESSION_NONE, dispatch_string("none"));
  #ifdef FEATURE_ZSTD
  assert_int(&test, COMPRESSION_ZSTD, dispatch_string("ZSTD"));
  assert_int(&test, COMPRESSION_ZSTD, dispatch_string("zstd"));
  #else
  assert_int(&test, COMPRESSION_INVALID, dispatch_string("ZSTD"));
  assert_int(&test, COMPRESSION_INVALID, dispatch_string("zstd"));
  #endif
  #ifdef FEATURE_LZ4
  assert_int(&test, COMPRESSION_LZ4, dispatch_string("LZ4"));
  assert_int(&test, COMPRESSION_LZ4, dispatch_string("lz4"));
  #else
  assert_int(&test, COMPRESSION_INVALID, dispatch_string("LZ4"));
  assert_int(&test, COMPRESSION_INVALID, dispatch_string("lz4"));
  #endif

  size_t size = 1920*1080*4;
  assert_int(&test, size, get_compression_bound(COMPRESSION_NONE, size));
  #ifdef FEATURE_ZSTD
  assert_int(&test, ZSTD_COMPRESSBOUND(size), get_compression_bound(COMPRESSION_ZSTD, size));
  #endif
  #ifdef FEATURE_LZ4
  assert_int(&test, LZ4_COMPRESSBOUND(size), get_compression_bound(COMPRESSION_LZ4, size));
  #endif

  assert_int(&test, *(size_t *)none_compression_wrapper, *(size_t *)dispatch_compression_algorithm(COMPRESSION_NONE));
  #ifdef FEATURE_ZSTD
  assert_int(&test, *(size_t *)zstd_compression_wrapper, *(size_t *)dispatch_compression_algorithm(COMPRESSION_ZSTD));
  #endif
  #ifdef FEATURE_LZ4
  assert_int(&test, *(size_t *)lz4_compression_wrapper, *(size_t *)dispatch_compression_algorithm(COMPRESSION_LZ4));
  #endif

  char data1[] = {
    0b10101010,
    0b01010101
  };
  char data2[] = {
    0b11111111,
    0b11111111
  };
  char result_data[2];
  char expected[] = {
    0b01010101,
    0b10101010
  };

  xor_delta(result_data, data1, data2, sizeof(data1));
  assert_int(&test, 0, memcmp(result_data, expected, 2));

  unsigned int data3 = 0xFADE6969;
  unsigned int data4 = 0xF1DE1337;
  unsigned int result_data2;
  unsigned int expected2 = 0xB007A5E;

  xor_delta((char *)&result_data2, (char *)&data3, (char *)&data4, 4);
  assert_int(&test, 0, memcmp(&result_data2, &expected2, 4));

  return assert_done(&test);
}

int main() {
  bool result = 1;
  result &= test_circular_array();
  //test_Xscreenshot();
  //test_Xvideo();
  result &= test_option_value_comparation();
  result &= test_parse_flag();
  result &= test_parse_value();
  result &= test_parse_flags();
  result &= test_parse_env_string();
  result &= test_dynamic_circular_array();
  result &= test_compression();
  return !result;
}
