#ifndef AUTOCOMPLETION_H_
#define AUTOCOMPLETION_H_

#include "log.h"

/** @brief Dumps autocompletion script in stdout.
 *  @param logger logger
 *  @param target_shell shell to generate script.
 *  @return 0 on success, -1 on error.
 */
int generate_autocompletion_script(logger *logger, char *target_shell);

#endif
