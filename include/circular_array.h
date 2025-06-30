#ifndef CIRCULAR_ARRAY_H_
#define CIRCULAR_ARRAY_H_
#include <stddef.h>
typedef struct {
  void **data;
  size_t length;
  size_t size;
} circular_array;

/** @brief Initialises circular array.
 *  @param[in] length length of the array.
 *  @param[in] size size of each item.
 *  @returns #circular_array instance if succeed, NULL on error.
 */
circular_array *circular_array_init(size_t length, size_t size);

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
 *  @param[out] dst pointer to destination.
 *  @returns 0 if succeed, -1 on error.
 */
int circular_array_get(circular_array *array, int index, void **dst);

#endif
