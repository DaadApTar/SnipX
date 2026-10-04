#ifndef NVENC_FFMPEG_H
#define NVENC_FFMPEG_H

#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include "encoding.h"
#include "log.h"

typedef struct {
  logger *logger;
  unsigned int gop_length;
  unsigned int bitrate;
  const AVCodec *codec;
  AVCodecContext *codec_ctx;
  AVFrame *frame;
  AVPacket *pkt;
  dynamic_circular_array *array;
} nvenc_ffmpeg_context;

/**
 * @copydoc encoding_init
 * @param[in] context must be a pointer to #nvenc_ffmpeg_context instance.
 */
int nvenc_ffmpeg_init(logger *logger, void *context, unsigned int gop_length, unsigned int bitrate, encoding_codec codec, unsigned int width, unsigned int height, unsigned int fps, dynamic_circular_array *array);

/**
 * @copydoc encoding_close
 * @param[in] context must be a pointer to #nvenc_ffmpeg_context instance.
 */
void nvenc_ffmpeg_close(void *context);

/**
 * @copydoc encoding_submit
 * @param[in] context must be a pointer to #nvenc_ffmpeg_context instance.
 */
int nvenc_ffmpeg_submit(void *context, const void *frame, int bytes_per_line, int timestamp);

/**
 * @copydoc encoding_stop
 * @param[in] context must be a pointer to #nvenc_ffmpeg_context instance.
 */
int nvenc_ffmpeg_stop(void *context);

#endif //NVENC_FFMPEG_H
