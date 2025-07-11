#include "options.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void print_usage(char *program) {
  fprintf(stderr, "%s [options]\n\n", program);
  fprintf(stderr, "PARAMS:\n");
  for (size_t i = 0; i < (int)sizeof(available_flags) / (int)sizeof(available_flags[0]); ++i) {
    flag available_flag = available_flags[i];
    fprintf(stderr, "\t-%c\t--%s\t\t%s\n", available_flag.short_flag,
                                           available_flag.long_flag,
                                           available_flag.description);
  }
}

options *parse_flags(char **args, size_t size) {
  options *opts = (options *)malloc(sizeof(options));
  // Default values
  opts->brake         = false;
  opts->fps           = 30;
  opts->screen_number = 0;
  opts->sound_monitor = 0;
  // Fill the opts.
  for (int i = 0; i < (int)size; ++i) {
    option_type option = parse_flag(args[i]); 
    value_type value = 0;
    if (i != (int)size-1) {
      value = parse_value(args[i+1]);
    }
    if(!option_value_compare(option, value)) {
      free(opts);
      return 0;
    }
    switch (option) {
    case SCREEN_NUMBER:
      opts->screen_number = atoi(args[i+1]);
      break;
    case FPS:
      opts->fps = atoi(args[i+1]);
      break;
    case SOUND_MONITOR:
      opts->sound_monitor = args[i+1];
      break;
    case BRAKE:
      opts->brake = true;
      break;
    case WINDOW:
    default:
      free(opts);
      return 0;
    }
    ++i;
  }

  return opts;
}

bool option_value_compare(option_type option, value_type value) {
  value_type value_types[OPTION_TYPE_LENGTH];
  value_types[UNKNOWN] = NONE;
  value_types[SCREEN_NUMBER] = NUMBER;
  value_types[WINDOW] = NONE;
  value_types[FPS] = NUMBER;
  value_types[SOUND_MONITOR] = STRING;
  value_types[BRAKE] = NONE;

  return value_types[option] == value;
}

option_type parse_flag(char *flag) {
  char long_flag[64];
  char short_flag[64];
  for (int i = 0; i < (int)sizeof(available_flags) / (int)sizeof(available_flags[0]); ++i) {
    sprintf(long_flag, "--%s", available_flags[i].long_flag);
    sprintf(short_flag, "-%c", available_flags[i].short_flag);
    if (strcmp(flag, long_flag) == 0 || strcmp(flag, short_flag) == 0) return available_flags[i].type;
  }
  return UNKNOWN;
}

value_type parse_value(char *value) {
  while (*(++value) != '\0') {
    if (*value < '0' || *value > '9') return STRING;
  }
  return NUMBER;
}
