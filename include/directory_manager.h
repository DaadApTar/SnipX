#ifndef DIRECTORY_MANAGER_H_
#define DIRECTORY_MANAGER_H_

/** @brief Creates directory if not exists.
 *  @param[in] Path to directory.
 *  @return 0 if succeed, -1 on error.
 */
int dir_create_if_not_exists(char *restrict path);

/** @brief Selects default path if env is not set.
 *  @param[in] default_path default path.
 *  @param[in] env_name name of environment variable.
 *  @return Path.
 */
char *dir_default_or_env(char *default_path, char *env_name);

/** @brief Expands all variables in string.
 *  @param[in] string string.
 *  @retrrn String with expanded variables.
 */
char *dir_expand_env(char *restrict string);

#endif
