#ifndef ENCODING_H_
#define ENCODING_H_

#include <string.h>

#include "build_features.h"

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
inline int get_encoder_profile(const char *encoder, const char *codec, encoding_profile *profile) {
  profile->backend = BACKEND_INVALID;
  profile->codec = CODEC_INVALID;
#ifdef FEATURE_NVENC_FFMPEG
  if (strcasecmp(encoder, "nvenc_ffmpeg") == 0) profile->backend = BACKEND_NVENC_FFMPEG;
#endif
#ifdef FEATURE_VAAPI
  if (strcasecmp(encoder, "vaapi") == 0) profile->backend = BACKEND_VAAPI;
#endif
  if (strcasecmp(codec, "h264") == 0) profile->codec = CODEC_H264;
  if (strcasecmp(codec, "hevc") == 0) profile->codec = CODEC_HEVC;
  if (strcasecmp(codec, "av1") == 0) profile->codec = CODEC_AV1;

  if (profile->backend == BACKEND_INVALID || profile->codec == CODEC_INVALID) return -1;

  return 0;
}

/** @brief Encoding init function for filling the encoding context.
 *  @param[out] context encoding context.
 *  @param[in] gop_length group of picture length.
 *  @param[in] bitrate encoding bitrate.
 *  @return 0 on success, -1 on error.
 */
typedef int (*encoding_init)(void *context, unsigned int gop_length, unsigned int bitrate);

/** @brief Encoding closing function for freeing the encoding context.
 *  @param[out] context encoding context.
 */
typedef void (*encoding_close)(void *context);

typedef struct {
  void *context;
  encoding_init init;
  void *send_frame;
  encoding_close close;
} encoding_interface;

#endif // ENCODING_H_
