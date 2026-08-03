#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

/**
 * Types of options.
 */
typedef enum {
  UNKNOWN = 0,
  SCREEN_NUMBER,
  /* WINDOW, // Isn't implemented yet. */
  FPS,
  DESKTOP_SOUND_MONITOR,
  MIC_SOUND_MONITOR,
  STOP,
  IMMEDIATE,
  DEFER,
  RENDER,
  LENGTH,
  BITRATE,
  ADDRESS,
  PORT,
  LOCALLY,
  HELP,
  OUTPUT,
  DEBUG,
  AUTOCOMPLETION,
  COMPRESSION,
  SOURCES,
  MBPS,
  VERSION,
  COMPRESSION_LEVEL,

  OPTION_TYPE_LENGTH
} option_type;

typedef enum {
  NONE = 0,
  NUMBER = 1,
  STRING = 2,
} value_type;

typedef struct {
  const char *long_flag;
  char short_flag;
  const char *description;
  option_type type;
  //option_type conflict;
  int priority;
} flag;

typedef struct {
  /// It's used for flags that should stop program after execution like --stop.
  bool close_after;

  int screen_number;
  int window;
  int fps;
  int desktop_sound_monitor;
  int mic_sound_monitor;
  bool stop;
  bool immediate;
  bool defer;
  bool render;
  bool help;
  int port;
  int length;
  char *address;
  bool locally;
  long bitrate;
  char *output;
  int mbps;
  bool sources;
  bool version;
  char *compression;
  int compression_level;
  char *autocompletion;
  bool debug;
} options;

extern flag available_flags[];
extern value_type value_types[OPTION_TYPE_LENGTH];
extern size_t available_flags_size;

/** @brief Prints program usage.
 *  @param[in] program Program name.
 */
void print_usage(char *program);

/** @brief Parses flags.
 *  @param[in] args Array of arguments
 *  @return pointer to #options if succeed, NULL on error.
 */
options *parse_flags(char **args, size_t size);

/** @brief Parses flag.
 *  @param[in] flag flag.
 *  @return #option_type.
 */
option_type parse_flag(char *flag);

/** @brief Parses value.
 *  @param[in] value value.
 *  @return #value_type.
 */
value_type parse_value(char *value);

/** @brief Getting relative value type.
 *  @param[in] option option.
 *  @return Relative #value_type.
 */
value_type get_value_type(option_type option);

#endif
