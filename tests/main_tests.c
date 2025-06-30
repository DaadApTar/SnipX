#include <stdlib.h>
#include "assertation.h"

#include "circular_array.h"

void test_circular_array() {
  test test = {.name = "circular array"};
  circular_array *array = circular_array_init(5, sizeof(int));
  for (int i = 0; i < 10; ++i) {
    int *value = malloc(sizeof(int));
    *value = i;
    circular_array_push(array, value, i);
  }
  int *dst;
  circular_array_get(array, 9, (void **)&dst);
  assert_int(&test, 9, *dst);
  circular_array_get(array, 5, (void **)&dst);
  assert_int(&test, 5, *dst);
  circular_array_get(array, 4, (void **)&dst);
  assert_int(&test, 9, *dst);
  circular_array_get(array, 0, (void **)&dst);
  assert_int(&test, 5, *dst);
  assert_done(&test);
}

int main() {
  test_circular_array();
  return 0;
}
