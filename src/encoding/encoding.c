#include "encoding/encoding.h"

#include <strings.h>

int get_encoder_profile(const char *encoder, const char *codec, encoding_profile *profile) {
  if (profile == NULL) return -1;
  if (encoder == NULL) return -1;
  if (codec == NULL) return -1;
  profile->backend = BACKEND_INVALID;
  profile->codec = CODEC_INVALID;
#ifdef FEATURE_NVENC_FFMPEG
  if (strcasecmp(encoder, "nvenc_ffmpeg") == 0) profile->backend = BACKEND_NVENC_FFMPEG;
#endif
#ifdef FEATURE_VAAPI
  if (strcasecmp(encoder, "vaapi") == 0) profile->backend = BACKEND_VAAPI;
#endif
  if (strcasecmp(codec, "h.264") == 0) profile->codec = CODEC_H264;
  if (strcasecmp(codec, "hevc") == 0) profile->codec = CODEC_HEVC;
  if (strcasecmp(codec, "av1") == 0) profile->codec = CODEC_AV1;

  if (profile->backend == BACKEND_INVALID || profile->codec == CODEC_INVALID) return -1;

  return 0;
}
