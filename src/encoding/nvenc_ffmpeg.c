#ifdef FEATURE_NVENC_FFMPEG
#include "encoding/encoding.h"
#include "encoding/nvenc_ffmpeg.h"
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <string.h>

#define AV_ERROR(logger, code) do {                       \
char buf[AV_ERROR_MAX_STRING_SIZE]; \
av_strerror(code, buf, AV_ERROR_MAX_STRING_SIZE); \
log_print(logger, LOG_ERROR, "%s\n", buf); \
} while (0)

int nvenc_ffmpeg_init(logger *logger, void *context, unsigned int gop_length, unsigned int bitrate, encoding_codec codec, unsigned int width, unsigned int height, unsigned int fps, dynamic_circular_array *array) {
  if (context == NULL) {
    log_print(logger, LOG_ERROR, "Context is NULL.\n");
    return -1;
  }
  nvenc_ffmpeg_context *ctx = context;
  memset(ctx, 0, sizeof(nvenc_ffmpeg_context));
  ctx->logger = logger;
  ctx->array = array;
  switch (codec) {
    case CODEC_H264:
      ctx->codec = avcodec_find_encoder_by_name("h264_nvenc");
      break;
    case CODEC_HEVC:
      ctx->codec = avcodec_find_encoder_by_name("hevc_nvenc");
      break;
    case CODEC_AV1:
      ctx->codec = avcodec_find_encoder_by_name("av1_nvenc");
      break;
    default:
      return -1;
  }
  if (ctx->codec == NULL) {
    log_print(ctx->logger, LOG_ERROR, "Codec not found.\n");
    goto fail;
  }

  ctx->codec_ctx = avcodec_alloc_context3(ctx->codec);
  if (ctx->codec_ctx == NULL) {
    log_print(ctx->logger, LOG_ERROR, "Could not allocate the codec context.\n");
    goto fail;
  }

  ctx->codec_ctx->width = (int)width;
  ctx->codec_ctx->height = (int)height;
  ctx->codec_ctx->time_base = (AVRational) {.num = 1, .den = (int)fps};
  ctx->codec_ctx->framerate = (AVRational) {.num = (int)fps, .den = 1};
  ctx->codec_ctx->bit_rate = bitrate;
  ctx->codec_ctx->gop_size = (int)gop_length;
  ctx->codec_ctx->max_b_frames = 0;

  ctx->codec_ctx->pix_fmt = AV_PIX_FMT_BGR0;
  int status;
  if (codec != CODEC_H264 && (status = av_opt_set(ctx->codec_ctx->priv_data, "tier", "main", 0)) != 0) {
    AV_ERROR(ctx->logger, status);
    goto fail;
  }
  if ((status = av_opt_set(ctx->codec_ctx->priv_data, "preset", "p6", 0)) != 0) {
    AV_ERROR(ctx->logger, status);
    goto fail;
  }

  if ((status = avcodec_open2(ctx->codec_ctx, ctx->codec, NULL)) < 0) {
    AV_ERROR(ctx->logger, status);
    goto fail;
  }

  ctx->frame = av_frame_alloc();
  if (ctx->frame == NULL) {
    log_print(ctx->logger, LOG_ERROR, "Could not allocate the frame.\n");
    goto fail;
  }
  ctx->frame->format = ctx->codec_ctx->pix_fmt;
  ctx->frame->width = (int) width;
  ctx->frame->height = (int) height;
  if ((status = av_frame_get_buffer(ctx->frame, 32)) != 0) {
    AV_ERROR(ctx->logger, status);
    goto fail;
  }

  ctx->pkt = av_packet_alloc();

  return 0;
  fail:
  nvenc_ffmpeg_close(ctx);
  return -1;
}

int nvenc_ffmpeg_submit(void *context, const void *frame, int bytes_per_line, int timestamp) {
  int status;
  nvenc_ffmpeg_context *ctx = (nvenc_ffmpeg_context *)context;
  if ((status =av_frame_make_writable(ctx->frame)) != 0) {
    AV_ERROR(ctx->logger, status);
    return -1;
  }
  av_image_copy_plane(ctx->frame->data[0], ctx->frame->linesize[0], (const uint8_t *) frame, bytes_per_line, ctx->frame->width * 4, ctx->frame->height);
  ctx->frame->pts = timestamp;

  if ((status = avcodec_send_frame(ctx->codec_ctx, ctx->frame)) != 0) {
    AV_ERROR(ctx->logger, status);
    return -1;
  }

  while (avcodec_receive_packet(ctx->codec_ctx, ctx->pkt) == 0) {
    if (dynamic_circular_array_push(ctx->array, ctx->pkt->data, ctx->pkt->size) != 0) {
      log_print(ctx->logger, LOG_ERROR, "Could not enqueue packet.\n");
      av_packet_unref(ctx->pkt);
      return -1;
    }
    av_packet_unref(ctx->pkt);
  }

  return 0;
}

int nvenc_ffmpeg_stop(void *context) {
  nvenc_ffmpeg_context *ctx = (nvenc_ffmpeg_context *)context;
  avcodec_send_frame(ctx->codec_ctx, NULL);
  while (avcodec_receive_packet(ctx->codec_ctx, ctx->pkt) == 0) {
    if (dynamic_circular_array_push(ctx->array, ctx->pkt->data, ctx->pkt->size) != 0) {
      log_print(ctx->logger, LOG_ERROR, "Could not enqueue packet.\n");
      av_packet_unref(ctx->pkt);
      return -1;
    }
    av_packet_unref(ctx->pkt);
  }

  return 0;
}

void nvenc_ffmpeg_close(void *context) {
  nvenc_ffmpeg_context *ctx = context;
  if (ctx == NULL) return;
  if (ctx->pkt != NULL) av_packet_free(&ctx->pkt);
  if (ctx->frame != NULL) av_frame_free(&ctx->frame);
  if (ctx->codec_ctx != NULL) avcodec_free_context(&ctx->codec_ctx);
}

#endif