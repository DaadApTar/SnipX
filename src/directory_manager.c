#include "directory_manager.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define HOME getenv("HOME")

int dir_create_if_not_exists(char *restrict path) {
  struct stat st = {0};
  char *processed_path = dir_expand_env(path);
  if (stat(processed_path, &st) == -1) {
    return mkdir(processed_path, 0777);
  } 
  free(processed_path);
  return 0;
}

char *dir_default_or_env(char *default_path, char *env_name) {
  char *env_path = getenv(env_name);
  if (env_path == 0)
    return dir_expand_env(default_path);
  return dir_expand_env(env_path);
}

char *dir_expand_env(char *restrict string) {
  int original_length = strlen(string);
  int new_length = original_length * 2;
  char *new_string = (char *)malloc(sizeof(char) * new_length);
  char *dst = new_string;
  char *src = string;
  while (*src) {
    if (*src == '$') {
      src++;
      char variable_name[128] = {0};
      int i = 0;
      while((*src == '_' || isalnum(*src)) && i < 128) {
        variable_name[i++] = *src++;
      }
      const char *value = getenv(variable_name);
      if (value) dst += sprintf(dst, "%s", value);
    }
    else {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
  return new_string;
}
