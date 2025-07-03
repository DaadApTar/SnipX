#include <X11/X.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "assertation.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include <libavcodec/codec_id.h>
#include <libavcodec/packet.h>
#include <libavformat/avio.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

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
    XImage *image = XGetImage(display, root, screen_x, screen_y, screen_width, screen_height, AllPlanes, ZPixmap);
    if (!image) {
      fprintf(stderr, RED"Cannot get image.\n"RESET);
      return;
    }
    circular_array_push(array, image, i);
  }

  // FFmpeg part
 av_log_set_level(AV_LOG_DEBUG);
 const char* filename = "test_xvideo.mp4";
  const AVCodec *codec;
  AVCodecContext *codec_ctx = 0;
  AVFormatContext *format_ctx = 0;
  AVFrame *frame;
  AVPacket *packet;

  codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  codec_ctx = avcodec_alloc_context3(codec);
  avformat_alloc_output_context2(&format_ctx, 0, 0, filename);

  packet = av_packet_alloc();

  codec_ctx->bit_rate = 8000000;
  codec_ctx->width = screen_width;
  codec_ctx->height = screen_height;
  codec_ctx->time_base = (AVRational) {1, 60};
  codec_ctx->framerate = (AVRational) {60, 1};
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx->gop_size = 10;
  codec_ctx->max_b_frames = 2;
  codec_ctx->qmin = 10;
  codec_ctx->qmax = 51;

  avcodec_open2(codec_ctx, codec, 0);

  AVStream *stream = avformat_new_stream(format_ctx, 0);
  stream->id = format_ctx->nb_streams -1;
  stream->time_base = codec_ctx->time_base;
  avcodec_parameters_from_context(stream->codecpar, codec_ctx);
  avio_open(&format_ctx->pb, filename, AVIO_FLAG_WRITE);

  avformat_write_header(format_ctx, 0);
  
  frame = av_frame_alloc();
  frame->format = codec_ctx->pix_fmt;
  frame->width = codec_ctx->width;
  frame->height = codec_ctx->height;

  av_frame_get_buffer(frame, 32);

  struct SwsContext *sws_ctx = sws_getContext(screen_width, screen_height, AV_PIX_FMT_BGR0,
                                              screen_width, screen_height, AV_PIX_FMT_YUV420P,
                                              SWS_BILINEAR, 0, 0, 0);

  XImage *dst;
  for (int i = 0; i < 600; ++i) {
    av_frame_make_writable(frame);
    circular_array_get(array, i, (void **)&dst);

    uint8_t const *src_slices[1] = { (uint8_t *)dst->data };
    int src_stride[1] = {4 * screen_width};

    sws_scale(sws_ctx, src_slices, src_stride, 0, screen_height, frame->data, frame->linesize);
    frame->pts = i;
    int ret = avcodec_send_frame(codec_ctx, frame);

    if (ret < 0) {
      fprintf(stderr, RED"Error sending a frame for encoding.\n"RESET);
      return;
    }

    while ((ret = avcodec_receive_packet(codec_ctx, packet)) == 0) {
      packet->stream_index = stream->index;
      ret = av_interleaved_write_frame(format_ctx, packet);
      if (ret < 0) {
        fprintf(stderr, RED"Error writing packet: %s\n"RESET, av_err2str(ret));
        return;
      }
      av_packet_unref(packet);
      avcodec_receive_packet(codec_ctx, packet);
    }
    if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
        fprintf(stderr, RED"Error receiving packet from encoder: %s.\n"RESET, av_err2str(ret));
        return;
    }
  }
  avcodec_send_frame(codec_ctx, 0);
  int ret;
  while ((ret = avcodec_receive_packet(codec_ctx, packet)) == 0) {
    packet->stream_index = stream->index;
    av_interleaved_write_frame(format_ctx, packet);
    av_packet_unref(packet);
  }
  if (ret != AVERROR_EOF) {
      fprintf(stderr, RED"Error flushing encoder: %s.\n"RESET, av_err2str(ret));
      return;
  }

  av_write_trailer(format_ctx);

  avcodec_free_context(&codec_ctx);
  av_frame_free(&frame);
  av_packet_free(&packet);

  XFree(screens);
  XCloseDisplay(display);
}

int main() {
  test_circular_array();
  test_Xscreenshot();
  test_Xvideo();
  return 0;
}
