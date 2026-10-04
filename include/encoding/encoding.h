#ifndef ENCODING_H_
#define ENCODING_H_

#include <dynamic_circular_array.h>
#include "log.h"

typedef enum {
  BACKEND_NVENC_FFMPEG = 0,
  BACKEND_VAAPI = 1,

  BACKEND_INVALID,
} encoding_backends;

typedef enum {
  CODEC_H264,
  CODEC_HEVC,
  CODEC_AV1,
  CODEC_INVALID,
} encoding_codec;

typedef struct {
  encoding_backends backend;
  encoding_codec codec;
} encoding_profile;

/** @brief Parses user string to encoding profile.
 *  @param[in] encoder user provided encoder backend.
 *  @param[in] codec user provided codec.
 *  @param[out] profile pointer to profile to fill.
 *  @return 0 on success, -1 on error.
 *  @note string should be given in this format: backend_codec. For example: nvenc_ffmpeg_av1, vaapi_hevc.
 */
int get_encoder_profile(const char *encoder, const char *codec, encoding_profile *profile);

/** @brief Encoding init function for filling the encoding context.
 *  @param[in] logger application logger.
 *  @param[out] context encoding context.
 *  @param[in] gop_length group of picture length.
 *  @param[in] bitrate encoding bitrate.
 *  @param[in] codec encoding codec.
 *  @param[in] width frame width.
 *  @param[in] height frame height.
 *  @param[in] fps fps.
 *  @param[in] array array to write to.
 *  @return 0 on success, -1 on error.
 */
typedef int (*encoding_init)(logger *logger, void *context, unsigned int gop_length, unsigned int bitrate, encoding_codec codec, unsigned int width, unsigned int height, unsigned int fps, dynamic_circular_array *array);

/** @brief Encoding closing function for freeing the encoding context.
 *  @param[out] context encoding context.
 */
typedef void (*encoding_close)(void *context);

/** @brief Submits the frame into encoder.
 *  @param[in] context encoding context.
 *  @param[in] frame frame.
 *  @param[in] bytes_per_line size of image line stored in img struct.
 *  @param[in] timestamp frame timestamp. Necessary to set for the right frame order.
 *  TODO: now it uses GPU to CPU to GPU frame pipeline. Will use textures later.
 */
typedef int (*encoding_submit)(void *context, const void *frame, int bytes_per_line, int timestamp);

/** @brief Stops the encoding.
 *  @param[in] context encoding context.
 *  @return 0 on success, -1 on error.
 */
typedef int (*encoding_stop)(void *context);

typedef struct {
  void *context;
  encoding_init init;
  encoding_submit submit_frame;
  encoding_close close;
  encoding_stop stop;
} encoding_interface;

#endif // ENCODING_H_
