/** This wild data structure is inteded to be a replacement for circular array for compressed data.
 *  This array stores chunks of undefined-sized data continuously in char* array. It also stores
 *  circular array of indices to chunks and counts amount of elements inside.
 *
 *  In case if new element size overlaps index of next element, array is expanded.
 *  @par Storing data
 *  There are a few rules in array:
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

#ifndef DYNAMIC_CIRUCLAR_ARRAY_H_
#define DYNAMIC_CIRUCLAR_ARRAY_H_

#include "circular_array.h"
#include <stddef.h>

typedef struct {
  char *data;
  size_t capacity;
  size_t items_length;
  size_t items_max;
  size_t last_index;
  circular_array indices;
} dynamic_circular_array;

/** @brief Initialises dynamic circular array.
 *  @param[out] array pointer to array to initialise.
 *  @param[in] initial_capacity length of the array in bytes.
 *  @param[in] items_max amount of possible elements.
 *  @return 0 if succeed, -1 on error.
 */
int dynamic_circular_array_init(dynamic_circular_array *array, size_t initial_capacity, size_t items_amount);

/** @brief Pushes data in the head of array.
 *  @param[in] array pointer to array.
 *  @param[in] data data.
 *  @param[in] size data size.
 *  @returns 0 if succeed, -1 on error.
 */
int dynamic_circular_array_push(dynamic_circular_array *array, void *data, size_t size);

/** @brief Gets item from relative array.
 *  @param[in] array pointer to array.
 *  @param[in] index item index.
 *  @param[out] size item size.
 *  @returns pointer to copy of data. If index doesn't contain data, pointer and size are set to 0.
 */
void *dynamic_circular_array_get(dynamic_circular_array *array, size_t index, size_t *size);

/** @brief Deallocates the data inside buffer and sets fiels to 0.
 *  @param[in] array pointer to array.
 */
void dynamic_circular_array_free(dynamic_circular_array *array);

/** @brief Clears the data inside buffer and sets length to 0.
 *  @param[in] array pointer to array.
 */
void dynamic_circular_array_clear(dynamic_circular_array *array);

/** @brief Duplicates circular array.
 *  @param[in] src array to copy
 *  @return pointer to new array if succeed, NULL on error.
 */
dynamic_circular_array *dynamic_circular_array_dup(dynamic_circular_array *src);

#endif
