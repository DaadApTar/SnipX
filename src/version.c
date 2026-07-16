#include "version.h"
#include <stdio.h>
#include "build_features.h"

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
  printf("Features: ");
  puts(FEATURE_COLORS_STRING FEATURE_SENDER_STRING);
#else
  #if !defined(APP_VERSION)
    #error APP_VERSION is not defined
  #endif
  #if !defined(GIT_COMMIT)
    #error GIT_VERSION is not defined
  #endif
#endif
}
