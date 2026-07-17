#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdbool.h>
typedef struct {
  FILE *file;
  char *filename;
  bool debug;
} logger;

typedef enum {
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_DEBUG,
} log_level;

/** @brief Provides a localtime.
 *  @return string.
 */
char *log_get_time();

/** @brief Initialises logger.
 *  @param[out] logger pointer to instance.
 *  @param[in] debug should it print debug.
 *  @return 0 if succeed, -1 on error.
 */
int log_init(logger *logger, bool debug);

/** @brief Initialises logger.
 *  @param[in] logger pointer to instance.
 *  @param[in] level #log_level.
 *  @param[in] format format string.
 *  @param[in] ... Optional arguments for format.
 *  @return 0 if succeed, -1 on error.
 */
int log_print(logger *logger, log_level level, const char *restrict format, ...);

/** @brief Closes logger.
 *  @param[in] logger pointer to instance.
 *  @return 0 if succeed, -1 on error.
 */
int log_close(logger *logger);

#endif
