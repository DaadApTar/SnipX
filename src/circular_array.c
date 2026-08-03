#include "circular_array.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int circular_array_init(circular_array *array, size_t capacity, size_t item_size) {
  if (array == 0) return 1;
  array->item_size = item_size;
  array->capacity = capacity;
  array->data = malloc(item_size * capacity);
  array->next_index = 0;
  if (array->data == 0) return 1;
  return 0;
}

int circular_array_push(circular_array *array, void *data) {
  size_t idx = array->next_index % array->capacity;
  memcpy(array->data + idx * array->item_size, data, array->item_size);
  if (array->length < array->capacity) array->length++;
  array->next_index++;
  return 0;
}

void *circular_array_get(circular_array *array, size_t index) {
  size_t idx = index % array->capacity;
  return array->data + idx * array->item_size;
}

void circular_array_free(circular_array *array) {
  free(array->data);
  array->item_size = 0;
  array->capacity = 0;
  array->length = 0;
  array->next_index = 0;
}

void circular_array_clear(circular_array *array) {
  memset(array->data, 0, array->capacity*array->item_size);
  array->length = 0;
  array->next_index = 0;
}

circular_array *circular_array_dup(circular_array *src) {
  circular_array *array = malloc(sizeof(circular_array));
  if (circular_array_init(array, src->capacity, src->item_size) == 1) {
    return NULL;
  }
  array->capacity = src->capacity;
  array->length = src->length;
  array->item_size = src->item_size;
  array->next_index = src->next_index;
  memcpy(array->data, src->data, src->item_size*src->capacity);
  return array;
}
