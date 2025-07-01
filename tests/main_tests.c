#include <stdlib.h>
#include <stdio.h>
#include "assertation.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

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

int main() {
  test_circular_array();
  test_Xscreenshot();
  return 0;
}
