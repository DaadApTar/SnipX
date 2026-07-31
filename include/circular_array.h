#ifndef CIRCULAR_ARRAY_H_
#define CIRCULAR_ARRAY_H_
#include <stddef.h>
typedef struct {
  void *data;
  size_t length;
  size_t capacity;
  size_t item_size;
  size_t last_index;
} circular_array;

/** @brief Initialises circular array.
 *  @param[out] array pointer to array to initialise.
 *  @param[in] capacity length of the array.
 *  @param[in] item_size size of each item.
 *  @returns 0 if succeed, -1 on error.
 */
int circular_array_init(circular_array *array, size_t capacity, size_t item_size);

/** @brief Pushes data into relative index.
 *  @param[in] array pointer to array.
 *  @param[in] data  data.
 *  @returns 0 if succeed, -1 on error.
 */
int circular_array_push(circular_array *array, void *data);

/** @brief Gets data from relative index.
 *  @param[in] array pointer to array.
 *  @param[in] index index.
 *  @returns pointer to data.
 */
void *circular_array_get(circular_array *array, size_t index);

/** @brief Deallocates the data inside buffer and sets fiels to 0.
 *  @param[in] array pointer to array.
 */
void circular_array_free(circular_array *array);

/** @brief Clears the data inside buffer and sets length to 0.
 *  @param[in] array pointer to array.
 */
void circular_array_clear(circular_array *array);

/** @brief Duplicates circular array.
 *  @param[in] src array to copy
 *  @return pointer to new array if succeed, NULL on error.
 */
circular_array *circular_array_dup(circular_array *src);

#endif
