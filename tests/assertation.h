#ifndef ASSERTATION_H_
#define ASSERTATION_H_

typedef struct {
  int amount;
  int passed;
  char *name;
} test;

#define RESET "\e[0m"
#define RED "\e[0;31m"
#define GREEN "\e[0;32m"

void assert_int(test *test, int expected, int actual);
void assert_done(test *test);

#endif
