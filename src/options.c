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

void print_version() {
#if defined (APP_VERSION) && defined (GIT_COMMIT)
  printf("snipx %s-%s\n", APP_VERSION, GIT_COMMIT);
  printf("Compiler: ");
  #if defined (__clang__)
    printf("Clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
  #elif defined (__GNUC__)
    printf("GCC %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
  #else
    printf("unknown compiler\n");
  #endif
  printf("Built: %s %s\n", __DATE__, __TIME__);
#else
  #if !defined(APP_VERSION)
    #error APP_VERSION is not defined
  #endif
  #if !defined(GIT_COMMIT)
    #error GIT_VERSION is not defined
  #endif
#endif
}

#ifndef DISABLE_SENDER
static_assert(OPTION_TYPE_LENGTH - 1 == 17, "New option has been added");
#else
static_assert(OPTION_TYPE_LENGTH - 1 == 14, "New option has been added");
#endif
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
  opts->desktop_sound_monitor = -1;
  opts->mic_sound_monitor     = -1;
  opts->port                  = 0;
  opts->bitrate               = 2500000;
  opts->length                = 10;
  opts->sources               = false;
  opts->version               = false;
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
#ifndef DISABLE_SENDER
static_assert(OPTION_TYPE_LENGTH - 1 == 17, "New option has been added");
#else
static_assert(OPTION_TYPE_LENGTH - 1 == 14, "New option has been added");
#endif
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
    case STOP:
      opts->stop = true;
      opts->close_after = 1;
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
#ifndef DISABLE_SENDER
    case ADDRESS:
      opts->address = args[i+1];
      break;
    case PORT:
      opts->port = atoi(args[i+1]);
      break;
    case LOCALLY:
      opts->locally = true;
      break;
#endif
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
    default:
      free(opts);
      return 0;
    }
    i += get_value_type(option) != NONE;
  }

  return opts;
}

#ifndef DISABLE_SENDER
static_assert(OPTION_TYPE_LENGTH - 1 == 17, "New option has been added");
#else
static_assert(OPTION_TYPE_LENGTH - 1 == 14, "New option has been added");
#endif
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
#ifndef DISABLE_SENDER
  [ADDRESS] = STRING,
  [PORT] = NUMBER,
  [LOCALLY] = NONE,
#endif
  [HELP] = NONE,
  [OUTPUT] = STRING,
  [SOURCES] = NONE,
  [VERSION] = NONE,
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
