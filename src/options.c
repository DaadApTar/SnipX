#include "options.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

flag available_flags[] = {
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
    {"bitrate", 'b', "Video bitrate.", BITRATE, .priority = 0},
    {"stop", 0, "Stop the recording.", STOP, .priority = 2},
    {"immediate", 'i', "Immediate render.", IMMEDIATE, .priority = 2},
    {"defer", 0, "Defer video rendering.", DEFER, .priority = 2},
    {"render", 'r', "Render deferred videos.", RENDER, .priority = 2},
#ifdef FEATURE_SENDER
    {"address", 'a', "Server IP-address.", ADDRESS, .priority = 0},
    {"port", 'p', "Server port.", PORT, .priority = 0},
    {"local", 0, "Save clip locally.", LOCALLY, .priority = 0},
#endif
    {"mbps", 0, "Speed limit of deferred clips file dumping.", MBPS, .priority = 0},
    {"sources", 0, "Print available sources to record audio.", SOURCES,
     .priority = 0},
    {"output", 'o', "Output directory.", OUTPUT, .priority = 0},
    {"autocompletion", 'u', "Dump an autocompletion script. Supported: zsh, bash.", AUTOCOMPLETION, .priority = 0},
#if defined(FEATURE_ZSTD) || defined(FEATURE_LZ4)
    {"compression", 'c', "Real-time compression algorithm. Supported:"
     #ifdef FEATURE_ZSTD
     " zstd"
     #endif
     #ifdef FEATURE_LZ4
     " lz4"
     #endif
     ".", COMPRESSION, .priority = 0
    },
    {"effort", 'e', "Compression effort (1-3).", COMPRESSION_LEVEL, .priority = 0},
#endif
    {"debug", 0, "Enable debug logs.", DEBUG, .priority = 0},
    {"help", 'h', "Print this message.", HELP, .priority = 0},
    {"version", 'v', "Print the version.", VERSION, .priority = 0},
};

size_t available_flags_size = sizeof(available_flags);

void print_usage(char *program) {
  printf("%s [options]\n\n", program);
  printf("PARAMS:\n");
  for (size_t i = 0; i < (int)sizeof(available_flags) / (int)sizeof(available_flags[0]); ++i) {
    flag available_flag = available_flags[i];
    if (available_flag.short_flag != 0) printf("%-4c-%c", ' ', available_flag.short_flag);
    else printf("%-6c", ' ');
    printf("%-4c--%-16s%s\n", ' ', available_flag.long_flag,
                                    available_flag.description);
  }
  printf("\n");
}

static_assert(OPTION_TYPE_LENGTH - 1 == 22, "New option has been added");
options *parse_flags(char **args, size_t size) {
  options *opts = (options *)malloc(sizeof(options));
  // Default values
  opts->stop                  = false;
  opts->immediate             = false;
  opts->defer                 = false;
  opts->render                = false;
  opts->help                  = false;
  opts->fps                   = 30;
  opts->screen_number         = 0;
  opts->desktop_sound_monitor = 0;
  opts->mic_sound_monitor     = -1;
  opts->port                  = 0;
  opts->bitrate               = 2500000;
  opts->length                = 10;
  opts->sources               = false;
  opts->version               = false;
  opts->mbps                  = 50;
  opts->debug                 = false;
  opts->compression_level     = 1;
#ifdef FEATURE_SENDER
  opts->locally               = false;
#else
  opts->locally               = true;
#endif
  // Fill the opts.
  for (int i = 0; i < (int)size; ++i) {
    option_type option = parse_flag(args[i]); 
    value_type actual_value = NONE;
    if (get_value_type(option) != NONE && i != (int)size-1) {
      actual_value = parse_value(args[i+1]);
    }
    if(get_value_type(option) != actual_value) {
      free(opts);
      return 0;
    }
static_assert(OPTION_TYPE_LENGTH - 1 == 22, "New option has been added");
    switch (option) {
    case SCREEN_NUMBER:
      opts->screen_number = atoi(args[i+1]);
      break;
    case FPS:
      opts->fps = atoi(args[i+1]);
      break;
    case DESKTOP_SOUND_MONITOR:
      opts->desktop_sound_monitor = atoi(args[i+1]);
      break;
    case MIC_SOUND_MONITOR:
      opts->mic_sound_monitor = atoi(args[i+1]);
      break;
    case LENGTH:
      opts->length = atoi(args[i+1]);
      break;
    case BITRATE:
      opts->bitrate = atoi(args[i+1]);
      break;
    case MBPS:
      opts->mbps = atoi(args[i+1]);
      break;
    case STOP:
      opts->stop = true;
      opts->close_after = 1;
      break;
    // space between debug and : is essential for magit-todos to not show this line
    case DEBUG :
      opts->debug = true;
      break;
    case IMMEDIATE:
      opts->immediate = true;
      opts->close_after = 1;
      break;
    case DEFER:
      opts->defer = true;
      opts->close_after = 1;
      break;
    case RENDER:
      opts->render = true;
      opts->close_after = 1;
      break;
    case ADDRESS:
      opts->address = args[i+1];
      break;
    case PORT:
      opts->port = atoi(args[i+1]);
      break;
    case LOCALLY:
      opts->locally = true;
      break;
    case HELP:
      opts->help = true;
      opts->close_after = 1;
      break;
    case SOURCES:
      opts->sources = true;
      opts->close_after = 1;
      break;
    case VERSION:
      opts->version = true;
      opts->close_after = 1;
      break;
    case OUTPUT:
      opts->output = args[i+1];
      break;
    case AUTOCOMPLETION:
      opts->autocompletion = args[i+1];
      opts->close_after = true;
      break;
    case COMPRESSION:
      opts->compression = args[i+1];
      break;
    case COMPRESSION_LEVEL:
      opts->compression_level = atoi(args[i+1]);
      break;
    default:
      free(opts);
      return 0;
    }
    i += get_value_type(option) != NONE;
  }

  return opts;
}

static_assert(OPTION_TYPE_LENGTH - 1 == 22, "New option has been added");
value_type value_types[OPTION_TYPE_LENGTH] = {
  [UNKNOWN] = NONE,
  [SCREEN_NUMBER] = NUMBER,
  /* [WINDOW] = NONE, */
  [FPS] = NUMBER,
  [DESKTOP_SOUND_MONITOR] = NUMBER,
  [MIC_SOUND_MONITOR] = NUMBER,
  [STOP] = NONE,
  [IMMEDIATE] = NONE,
  [DEFER] = NONE,
  [RENDER] = NONE,
  [LENGTH] = NUMBER,
  [BITRATE] = NUMBER,
  [MBPS] = NUMBER,
  [ADDRESS] = STRING,
  [PORT] = NUMBER,
  [LOCALLY] = NONE,
  [HELP] = NONE,
  [OUTPUT] = STRING,
  [SOURCES] = NONE,
  [VERSION] = NONE,
  [COMPRESSION] = STRING,
  [COMPRESSION_LEVEL] = NUMBER,
  [AUTOCOMPLETION] = STRING,
  [DEBUG] = NONE,
};

value_type get_value_type(option_type option) {
  return value_types[option];
}

option_type parse_flag(char *flag) {
  char long_flag[64];
  char short_flag[64];
  for (int i = 0; i < (int)sizeof(available_flags) / (int)sizeof(available_flags[0]); ++i) {
    sprintf(long_flag, "--%s", available_flags[i].long_flag);
    bool is_short_flag_exist = available_flags[i].short_flag != 0;
    if (is_short_flag_exist) sprintf(short_flag, "-%c", available_flags[i].short_flag);
    if (strcmp(flag, long_flag) == 0 || (is_short_flag_exist && strcmp(flag, short_flag) == 0)) return available_flags[i].type;
  }
  return UNKNOWN;
}

value_type parse_value(char *value) {
  while (*(++value) != '\0') {
    if (*value < '0' || *value > '9') return STRING;
  }
  return NUMBER;
}
