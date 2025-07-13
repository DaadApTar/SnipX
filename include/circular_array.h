#ifndef CIRCULAR_ARRAY_H_
#define CIRCULAR_ARRAY_H_
#include <stddef.h>
typedef struct {
  void *data;
  size_t length;
  size_t size;
} circular_array;

/** @brief Initialises circular array.
 *  @param[out] array pointer to array to initialise.
 *  @param[in] length length of the array.
 *  @param[in] item_size size of each item.
 *  @returns 0 if succeed, -1 on error.
 */
int circular_array_init(circular_array *array, size_t length, size_t item_size);

/** @brief Pushes data into relative index.
 *  @param[in] array pointer to array.
 *  @param[in] data  data.
 *  @param[in] index index.
 *  @returns 0 if succeed, -1 on error.
 */
int circular_array_push(circular_array *array, void *data, size_t index);

/** @brief Gets data from relative index.
 *  @param[in] array pointer to array.
 *  @param[in] index index.
 *  @returns pointer to data.
 */
void *circular_array_get(circular_array *array, int index);

#endif
