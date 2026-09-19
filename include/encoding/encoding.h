#ifndef ENCODING_H_
#define ENCODING_H_

typedef enum {
  BACKEND_NVENC_FFMPEG = 0,
  BACKEND_VAAPI = 1,
} encoding_backends;

typedef enum {
  CODEC_H264,
  CODEC_HEVC,
  CODEC_AV1,
} encoding_codec;

typedef struct {
  encoding_backends backend;
  encoding_codec codec;
} encoding_profile;

/** @brief Parses user string to encoding profile.
 *  @param[in] string user string
 *  @param[out] profile pointer to profile to fill.
 *  @return 0 on success, -1 on error.
 *  @note string should be given in this format: backend_codec. For example: nvenc_ffmpeg_av1, vaapi_hevc.
 */
int parse_encoder_string(const char *string, encoding_profile *profile);

/** @brief Encoding init function for filling the encoding context.
 *  @param[out] context encoding context.
 *  @param[in] gop_length group of picture length.
 *  @param[in] bitrate encoding bitrate.
 *  @return 0 on success, -1 on error.
 */
typedef int (encoding_init)(void *context, unsigned int gop_length, unsigned int bitrate);

/** @brief Encoding closing function for freeing the encoding context.
 *  @param[out] context encoding context.
 */
typedef void (encoding_close)(void *context);

typedef struct {
  void *context;
  encoding_init init;
  void *send_frame;
  encoding_close close;
} encoding_interface;

#endif // ENCODING_H_
