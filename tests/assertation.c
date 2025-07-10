#include "assertation.h"
#include <stdio.h>

void assert_int(test* test, int expected, int actual) {
  test->amount++;
  if (expected == actual) {
    test->passed++;
    printf("[%d/%d] "GREEN"PASSED.\n"RESET, test->passed, test->amount);
  }
  else {
    printf("[%d/%d] "RED"NOT PASSED. %d is expected, but got %d.\n"RESET, test->passed, test->amount, expected, actual);
  }
}

void assert_done(test *test) {
  printf("Tests `%s` are finished with ", test->name);
  if (test->amount == test->passed) printf(GREEN);
  else printf(RED);
  printf("%d"RESET" passed tests out of %d.\n", test->passed, test->amount);
}
