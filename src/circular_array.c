#include "circular_array.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int circular_array_init(circular_array *array, size_t length, size_t item_size) {
  if (array == 0) return 1;
  array->size = item_size;
  array->length = length;
  array->data = malloc(item_size * length);
  if (array->data == 0) return 1;
  return 0;
}

int circular_array_push(circular_array *array, void *data, size_t index) {
  size_t idx = index % array->length;
  memcpy(array->data + idx * array->size, data, array->size);
  return 0;
}

void *circular_array_get(circular_array *array, int index) {
  size_t idx = index % array->length;
  return array->data + idx * array->size;
}
