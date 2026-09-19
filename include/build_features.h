/** @file build_features.h
 *  Actually I'm not sure if it will become necessary, but I feel like I don't want to
 *  have this huge mess of macroses in other files.
 *
 *  Available features:
 *  - FEATURE_COLORS
 *  - FEATURE_SENDER
 *  - FEATURE_VAAPI
 *  - FEATURE_NVENC_FFMPEG
 */
#ifndef BUILD_FEATURES_H_
#define BUILD_FEATURES_H_

#ifdef FEATURE_COLORS

#define ANSI_RED "\e[0;31m"
#define ANSI_GREEN "\e[0;32m"
#define ANSI_YELLOW "\e[0;33m"
#define ANSI_RESET "\e[0m"
#define ANSI_GREY "\e[2;37m"
#define FEATURE_COLORS_STRING ANSI_GREEN"+colors "

#else

#define ANSI_RED ""
#define ANSI_GREEN ""
#define ANSI_YELLOW ""
#define ANSI_RESET ""
#define ANSI_GREY ""
#define FEATURE_COLORS_STRING ANSI_RED"-colors "

#endif // FEATURE_COLORS

#ifdef FEATURE_SENDER
#define FEATURE_SENDER_STRING ANSI_GREEN"+sender "
#else
#define FEATURE_SENDER_STRING ANSI_RED"-sender "
#endif

#ifdef FEATURE_VAAPI
#define FEATURE_VAAPI_STRING ANSI_GREEN"+vaapi "
#else
#define FEATURE_VAAPI_STRING ANSI_RED"-vaapi "
#endif

#ifdef FEATURE_NVENC_FFMPEG
#define FEATURE_NVENC_FFMPEG_STRING ANSI_GREEN"+nvenc_ffmpeg "
#else
#define FEATURE_NVENC_FFMPEG_STRING ANSI_RED"-nvenc_ffmpeg "
#endif

#if !defined (FEATURE_VAAPI) && !defined (FEATURE_NVENC_FFMPEG)
#error None of the encoding backends were enabled during compilation. God only knows how you want to record the video with this.
#endif

#endif
