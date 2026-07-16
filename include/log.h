#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
typedef struct {
  FILE *file;
  char *filename;
} logger;

typedef enum {
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR
} log_level;

/** @brief Provides a localtime.
 *  @return string.
 */
char *log_get_time();

/** @brief Initialises logger.
 *  @param[out] logger pointer to instance.
 *  @return 0 if succeed, -1 on error.
 */
int log_init(logger *logger);

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
