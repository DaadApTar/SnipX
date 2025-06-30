#include "circular_array.h"
#include <stdlib.h>
#include <string.h>

circular_array *circular_array_init(size_t length, size_t size) {
  circular_array *array = (circular_array *)malloc(sizeof(circular_array));
  if (array == 0) return 0;
  array->size = size;
  array->length = length;
  array->data = calloc(length, sizeof(void *));
  if (array->data == 0) {
    free(array);
    return 0;
  }
  for (size_t i = 0; i < length; ++i) {
    array->data[i] = 0;
  }
  return array;
}

int circular_array_push(circular_array *array, void *data, size_t index) {
  // FIXME: Probably memory leaks here, but I'm not sure about it rn.
  /*
  if (array->data[idx] != 0) {
    free(array->data[idx]);
  }
  */
  array->data[index % array->length] = data;
  //memcpy(array->data[index % array->length], data, array->size);
  return 0;
}

int circular_array_get(circular_array *array, int index, void **dst) {
  *dst = array->data[index % array->length];
  return 0;
}
