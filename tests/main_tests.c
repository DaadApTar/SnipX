#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "circular_array.h"

void test_circular_array() {
  circular_array *array = circular_array_init(5, sizeof(int));
  assert(array != 0);
  for (int i = 0; i < 10; ++i) {
    int *value = malloc(sizeof(int));
    *value = i;
    assert(circular_array_push(array, value, i) == 0);
  }
  int *dst;
  assert(circular_array_get(array, 9, (void **)&dst) == 0);
  assert(*dst == 9);
  assert(circular_array_get(array, 5, (void **)&dst) == 0);
  assert(*dst == 5);
  assert(circular_array_get(array, 4, (void **)&dst) == 0);
  assert(*dst == 9);
  assert(circular_array_get(array, 0, (void **)&dst) == 0);
  assert(*dst == 5);
}

int main() {
  test_circular_array();
  printf("Circular array tests are completed.\n");
  return 0;
}
