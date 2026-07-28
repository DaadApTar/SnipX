#include "dynamic_circular_array.h"
#include <stdlib.h>
#include <string.h>
#include "circular_array.h"

/** Represents data bounds.
 *  @field start is an index of the first byte. 
 *  @field end is an index of the last byte.
 *  @note these indices are in absolute values. It means that they can be > capacity.
 */
typedef struct {
  size_t start;
  size_t end;
} element_bounds;

int dynamic_circular_array_init(dynamic_circular_array *array, size_t capacity, size_t items_amount) {
  array->data = malloc(capacity);
  if (!array->data) return -1;
  array->items_length = 0;
  array->items_max = items_amount;
  array->capacity = capacity;
  array->last_index = 0;
  if (circular_array_init(&array->indices, items_amount, sizeof(element_bounds)) < 0) {
    free(array->data);
    return -1;
  }

  return 0;
}

/** @brief Reallocates #dynamic_circular_array.data to new size.
 *  @param[in] array array to reallocate.
 *  @param[in] new_size new size of data buffer.
 *  @return 0 on success, -1 on error.
 */
int dynamic_circular_array_realloc(dynamic_circular_array *array, size_t new_size) {
  if (new_size == array->capacity) return 0;
  void *new_data = realloc(array->data, new_size);
  if (!new_data) {
    return -1;
  }
  array->data = new_data;
  array->capacity = new_size;
  return 0;
}

/** There are a few rules in array:
 *  - If length < items_amount, and newly added data overlaps the capacity, buffer is too small for all elements
 *    and its size should be doubled without moving data.
 *  - If newly added element overlaps next element wrapped end pointer, buffer is too small for new element,
 *    so it should be slightly expanded, and data from buffer beginning should be moved to fill *all* new space.
 *  - Indices are stored as absolute values (they can be greater than array capacity), because then it's not needed
 *    to move them after expanding.
 *  - Data *can* be wrapped, because it protects buffer from continuous slight expanding that will lead to full
 *    straightening of buffer. It means that it won't be possible to return pointer to data in `get` function, so
 *    we will copy the data in straightened buffer. Since getting is less likely to be time dependent operation
 *    (it's used only on dumping), it's affordable to be this way.
 */
int dynamic_circular_array_push(dynamic_circular_array *array, void *data,
                                size_t size) {
  // Check if array is wrongly initialised.
  if (!array) return -1;
  if (!array->data) return -1;
  if (array->items_max == 0) return -1;
  // Check if params are invalid.
  if (size == 0) return -1;
  if (!data) return -1;

  element_bounds *last = circular_array_get(&array->indices, array->last_index);
  size_t new_data_start = 0;
  // Check if last element is not empty
  if (last->start != last->end) {
    new_data_start = last->end + 1;
  }
  size_t new_data_end = new_data_start + size - 1;

  // Check if array is too small for all elements.
  if (array->items_length < array->items_max && new_data_end > array->capacity) {
    size_t new_capacity = array->capacity * 2;
    while (new_capacity < new_data_end) new_capacity *= 2;
    if (dynamic_circular_array_realloc(array, new_capacity) < 0)
      return -1;
  }

  // Check if next element exists
  if (array->items_length == array->items_max) {
    element_bounds *next = circular_array_get(&array->indices, array->last_index+1);
    size_t old_capacity = array->capacity;
    size_t new_capacity = old_capacity;
    while (next->end + new_capacity < new_data_end) {
      new_capacity *= 1.5;
    }
    if (new_capacity > old_capacity) {
      if (dynamic_circular_array_realloc(array, new_capacity) < 0)
        return -1;
      size_t diff = new_capacity - old_capacity;
      memcpy(array->data + next->end + 1, array->data, diff);
    }
  }

  // Check if data will be wrapped
  size_t offset = new_data_start % array->capacity;
  if (new_data_start % array->capacity > new_data_end % array->capacity) {
    size_t first = array->capacity - offset;
    size_t second = size - first;
    memcpy(array->data + offset, data, first);
    memcpy(array->data, data + first, second);
  }
  else memcpy(array->data + offset, data, size);

  array->last_index++;
  if (array->items_length < array->items_max) array->items_length++;
  element_bounds bounds = {new_data_start, new_data_end};
  circular_array_push(&array->indices, &bounds, array->last_index);

  return 0;
}

void *dynamic_circular_array_get(dynamic_circular_array *array, size_t index, size_t *size) {
  element_bounds *pair = circular_array_get(&array->indices, index);
  if (!pair) {
    *size = 0;
    return NULL;
  }
  *size = pair->end - pair->start + 1;
  char *result = malloc(*size);

  if (!result) {
    *size = 0;
    return NULL;
  }

  // check if data is divided by buffer bounds.
  size_t offset = (pair->start % array->capacity);
  if (pair->end % array->capacity < pair->start % array->capacity) {
    size_t first = array->capacity - pair->start % array->capacity;
    memcpy(result, array->data + offset, first);
    memcpy(result + first, array->data, *size - first);
  }
  else {
    memcpy(result, array->data + offset, *size);
  }
  return result;
}

void dynamic_circular_array_free(dynamic_circular_array *array) {
  circular_array_free(&array->indices);
  array->capacity = 0;
  array->items_max = 0;
  array->items_length = 0;
  free(array->data);
}

void dynamic_circular_array_clear(dynamic_circular_array *array) {
  circular_array_clear(&array->indices);
  array->items_length = 0;
  memset(array->data, 0, array->capacity);
}

dynamic_circular_array *dynamic_circular_array_dup(dynamic_circular_array *src) {
  dynamic_circular_array *res = (dynamic_circular_array *)malloc(sizeof(dynamic_circular_array));
  res->capacity = src->capacity;
  res->items_length = src->items_length;
  res->items_max = src->items_max;
  void *tmp = circular_array_dup(&src->indices);
  if (!tmp) {
    free(res->data);
    free(res);
    free(tmp);
    return NULL;
  }
  res->indices = *(circular_array*)tmp;
  memccpy(res->data, src->data, res->capacity, src->capacity);

  return res;
}
