#include "log.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#define HOME getenv("HOME")
#define SNIPX_LOG_DIR getenv("SNIPX_LOG_DIR")

char *log_get_time() {
  time_t t = time(0);
  struct tm tm = *localtime(&t);
  char *return_time = (char *)malloc(sizeof(char) * 128);
  sprintf(return_time, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  return return_time;
}

int create_log_directory() {
  struct stat st = {0};
  char *path = log_get_directory();
  if (stat(path, &st) == -1) {
    return mkdir(path, 0777);
  } 
  free(path);
  return 0;
}

char *log_get_directory() {
  char *path = (char *)malloc(sizeof(char) * 256);
  char *snipx_log_directory = SNIPX_LOG_DIR;
  if (snipx_log_directory == 0)
    sprintf(path, "%s/.local/share/snipx/", HOME);
  else 
    sprintf(path, "%s/", snipx_log_directory);
  return path;
}

int log_init(logger *logger) {
  int ret = create_log_directory();
  if (ret == -1) {
    return -1;
  }
  char *filename = (char *)malloc(sizeof(char) * 256);
  char *path = log_get_directory();
  sprintf(filename, "%s%s.log", path, log_get_time());
  logger->file = fopen(filename, "w");
  logger->filename = filename;
  free(path);
  return 0;
}

int log_print(logger *logger, log_level level, const char *restrict format,
              ...) {
  char log_string[16];
  int ret;
  switch (level) {
    case LOG_INFO:
      sprintf(log_string, "INFO");
      break;
    case LOG_WARNING:
      sprintf(log_string, "WARNING");
      break;
    case LOG_ERROR:
      sprintf(log_string, "ERROR");
      break;
    default:
      break;
  }
  va_list args;
  va_start(args, format);
  va_list args_copy;
  va_copy(args_copy, args);
  printf("[%s %s%s"ANSI_RESET"] ", log_get_time(), level == LOG_INFO ? ANSI_GREEN : level == LOG_WARNING ? ANSI_YELLOW : level == LOG_ERROR ? ANSI_RED : "", log_string);
  ret = vprintf(format, args);
  if (ret < 0) return -1;
  ret = fprintf(logger->file, "[%s %s] ", log_get_time(), log_string);
  if (ret < 0) return -1;
  ret = vfprintf(logger->file, format, args_copy);
  if (ret < 0) return -1;
  va_end(args);
  return 0;
}

int log_close(logger *logger) {
  int ret = fclose(logger->file);
  free(logger->filename);
  if (ret != 0) return -1;
  return 0;
}
