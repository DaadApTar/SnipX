#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <stddef.h>
#include <stdbool.h>

/**
 * Types of options.
 */
typedef enum {
  UNKNOWN = 0,
  SCREEN_NUMBER,
  WINDOW, // Isn't implemented yet.
  FPS,
  SOUND_MONITOR,
  BRAKE,
  LENGTH,
  BITRATE,
  ADDRESS,
  PORT,
  HELP,

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
  int screen_number;
  int window;
  int fps;
  char *sound_monitor;
  bool brake;
  bool help;
  int length;
  char *address;
  int port;
  long bitrate;
} options;

static flag available_flags[] = {
  {"screen",       's', "Screen to record.", SCREEN_NUMBER, /*WINDOW,*/ .priority = 1},
  //{"window",       'w', "Window to record.", WINDOW, SCREEN_NUMBER, .priority = 0},
  {"fps",          'f', "Framerate.", FPS,                          .priority = 0},
  {"monitor",      'm', "Sound monitor to record.", SOUND_MONITOR,  .priority = 0},
  {"length",       'l', "Length of the video in seconds.", LENGTH,  .priority = 0},
  {"bitrate",        0, "Video bitrate.", BITRATE,                  .priority = 0},
  {"brake",        'b', "Brake the recording.", BRAKE,              .priority = 2},
  {"address",      'a', "Server IP-address", ADDRESS,               .priority = 0},
  {"port",         'p', "Server port", PORT,                        .priority = 0},
  {"help",         'h', "Print this message", HELP,                 .priority = 0},
};

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

/** @brief Compares option and value.
 *  @param[in] option option.
 *  @param[in] value value.
 *  @return true if compare, false if not.
 *  @deprecated use {@link #get_value_type()} instead.
 */
bool option_value_compare(option_type option, value_type value);

/** @brief Getting relative value type.
 *  @param[in] option option.
 *  @return Relative #value_type.
 */
value_type get_value_type(option_type option);

#endif
