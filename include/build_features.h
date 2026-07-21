#ifndef BUILD_FEATURES_H_
#define BUILD_FEATURES_H_
/** @brief Features macroses
 *  Actually I'm not sure if it will become necessary, but I feel like I don't want to
 *  have this huge mess of macroses in other files.
 */

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

#ifdef FEATURE_ZSTD
#define FEATURE_ZSTD_STRING ANSI_GREEN"+zstd "
#else
#define FEATURE_ZSTD_STRING ANSI_RED"-zstd "
#endif

#ifdef FEATURE_LZ4
#define FEATURE_LZ4_STRING ANSI_GREEN"+lz4 "
#else
#define FEATURE_LZ4_STRING ANSI_RED"-lz4 "
#endif

#endif
