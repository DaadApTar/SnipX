#include "autocompletion.h"
#include "log.h"
#include "string.h"
#include "options.h"
#include "stdio.h"

#define FLAGS_SIZE sizeof(available_flags) / sizeof(available_flags[0])

void generate_zsh() {
  printf("#compdef snipx\n");
  printf("_arguments \\\n");
  for (size_t i = 0; i < FLAGS_SIZE; ++i) {
    flag available_flag = available_flags[i];
    if (available_flag.short_flag != 0) {
      printf("\"(-%c --%s)\"{-%c,--%s}\"[%s]\" \\\n", available_flag.short_flag,
             available_flag.long_flag, available_flag.short_flag,
             available_flag.long_flag, available_flag.description);
    }
    else {
      printf("\"--%s[%s]\" \\\n", available_flag.long_flag, available_flag.description);
    }
  }
}

void generate_bash() {
  printf("#!/usr/bin/env bash\n");
  printf("_snipx_completion() {\n");
  printf("local cur=\"${COMP_WORDS[COMP_CWORD]}\"\n");
  printf("COMPREPLY=($(compgen -W \"");
  for(size_t i = 0; i < FLAGS_SIZE; ++i) {
    flag available_flag = available_flags[i];
    printf("--%s ", available_flag.long_flag);
    if (available_flag.short_flag != 0) printf("-%c ", available_flag.short_flag);
  }
  printf("\" -- \"$cur\"))\n");
  printf("}\n");

  printf("complete -F _snipx_completion snipx");
}

int generate_autocompletion_script(logger *logger, char *target_shell) {
  if (strcmp(target_shell, "zsh") == 0) generate_zsh();
  else if (strcmp(target_shell, "bash") == 0) generate_bash();
  else {
    log_print(logger, LOG_ERROR, "Unsupported shell `%s`.\n", target_shell);
    return -1;
  }

  return 0;
}
