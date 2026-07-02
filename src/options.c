#include "options.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void print_usage(char *program) {
  printf("%s [options]\n\n", program);
  printf("PARAMS:\n");
  for (size_t i = 0; i < (int)sizeof(available_flags) / (int)sizeof(available_flags[0]); ++i) {
    flag available_flag = available_flags[i];
    if (available_flag.short_flag != 0) printf("%-4c-%c", ' ', available_flag.short_flag);
    else printf("%-6c", ' ');
    printf("%-4c--%-12s%s\n", ' ', available_flag.long_flag,
                                    available_flag.description);
  }
  printf("\n");
}

static_assert(OPTION_TYPE_LENGTH - 1 == 11, "New option has been added");
options *parse_flags(char **args, size_t size) {
  options *opts = (options *)malloc(sizeof(options));
  // Default values
  opts->brake                 = false;
  opts->help                  = false;
  opts->fps                   = 30;
  opts->screen_number         = 0;
  opts->desktop_sound_monitor = 0;
  opts->mic_sound_monitor     = 0;
  opts->port                  = 0;
  opts->bitrate               = 2500000;
  opts->length                = 10;
#ifndef DISABLE_SENDER
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
static_assert(OPTION_TYPE_LENGTH - 1 == 11, "New option has been added");
    switch (option) {
    case SCREEN_NUMBER:
      opts->screen_number = atoi(args[i+1]);
      break;
    case FPS:
      opts->fps = atoi(args[i+1]);
      break;
    case DESKTOP_SOUND_MONITOR:
      opts->desktop_sound_monitor = args[i+1];
      break;
    case MIC_SOUND_MONITOR:
      opts->desktop_sound_monitor = args[i+1];
      break;
    case LENGTH:
      opts->length = atoi(args[i+1]);
      break;
    case BITRATE:
      opts->bitrate = atoi(args[i+1]);
      break;
    case BRAKE:
      opts->brake = true;
      break;
    case ADDRESS:
      opts->address = args[i+1];
      break;
    case PORT:
      opts->port = atoi(args[i+1]);
      break;
    case HELP:
      opts->help = true;
      break;
    case LOCALLY:
      opts->locally = true;
      break;
    default:
      free(opts);
      return 0;
    }
    i += get_value_type(option) != NONE;
  }

  return opts;
}

static_assert(OPTION_TYPE_LENGTH - 1 == 11, "New option has been added");
value_type value_types[OPTION_TYPE_LENGTH] = {
  [UNKNOWN] = NONE,
  [SCREEN_NUMBER] = NUMBER,
  /* [WINDOW] = NONE, */
  [FPS] = NUMBER,
  [DESKTOP_SOUND_MONITOR] = STRING,
  [MIC_SOUND_MONITOR] = STRING,
  [BRAKE] = NONE,
  [LENGTH] = NUMBER,
  [BITRATE] = NUMBER,
  [ADDRESS] = STRING,
  [PORT] = NUMBER,
  [LOCALLY] = NONE,
  [HELP] = NONE,
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
