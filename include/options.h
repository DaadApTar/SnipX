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
#ifndef DISABLE_SENDER
  ADDRESS,
  PORT,
  LOCALLY,
#endif
  HELP,
  OUTPUT,
  SOURCES,
  MBPS,
  VERSION,

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
} options;

static flag available_flags[] = {
    {"screen", 's', "Screen to record.", SCREEN_NUMBER,
     /*WINDOW,*/ .priority = 1},
    //{"window",       'w', "Window to record.", WINDOW, SCREEN_NUMBER,
    //.priority = 0},
    {"fps", 'f', "Framerate.", FPS, .priority = 0},
    {"desktop", 'd', "Desktop sound monitor to record.", DESKTOP_SOUND_MONITOR,
     .priority = 0},
    {"mic", 'm',
     "Mic sound monitor to record. Note: unless this flag is set explicitly, "
     "mic won't be recorded.",
     MIC_SOUND_MONITOR, .priority = 0},
    {"length", 'l', "Length of the video in seconds.", LENGTH, .priority = 0},
    {"bitrate", 0, "Video bitrate.", BITRATE, .priority = 0},
    {"stop", 0, "Stop the recording.", STOP, .priority = 2},
    {"immediate", 'i', "Immediate render.", IMMEDIATE, .priority = 2},
    {"defer", 0, "Defer video rendering.", DEFER, .priority = 2},
    {"render", 'r', "Render deferred videos.", RENDER, .priority = 2},
#ifndef DISABLE_SENDER
    {"address", 'a', "Server IP-address.", ADDRESS, .priority = 0},
    {"port", 'p', "Server port.", PORT, .priority = 0},
    {"local", 0, "Save clip locally.", LOCALLY, .priority = 0},
#endif
    {"mbps", 0, "Speed limit of deferred clips' file dumping.", MBPS, .priority = 0},
    {"sources", 0, "Print available sources to record audio.", SOURCES,
     .priority = 0},
    {"output", 'o', "Output directory.", OUTPUT, .priority = 0},
    {"help", 'h', "Print this message.", HELP, .priority = 0},
    {"version", 'v', "Print the version.", VERSION, .priority = 0},
};

static_assert(sizeof(available_flags) / sizeof(flag) == OPTION_TYPE_LENGTH - 1, "Not all options are set in flags.");

extern value_type value_types[OPTION_TYPE_LENGTH];

/** @brief Prints program usage.
 *  @param[in] program Program name.
 */
void print_usage(char *program);

/** @brief Prints the version.
 */
void print_version();

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
