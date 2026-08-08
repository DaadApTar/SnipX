#ifndef ALLOC_H_
#define ALLOC_H_

#include <stdlib.h>

/** @brief reserves chunk of memory.
 *  @param[in] size size of chunk.
 *  Neither malloc nor calloc guarantee memory reservation, though modern OSs usually do it instead allocating it in RAM.
 *  This function does guarantee this.
 *  @return pointer to chunk of data on success, NULL on error.
 */
void *reserve_memory(size_t size);

/** @brief Reserves new region and copies data.
 *  @param[in] chunk pointer to chunk
 *  @param[in] old_size size of chunk.
 *  @param[in] new_size new size of chunk to reserve.
 *  @note On success, old data is freed.
 *  @return pointer to new data on success, NULL on error.
 */
void *rereserve_memory(void *chunk, size_t old_size, size_t new_size);

/** @brief Frees reserved region.
 *  @param[in] chunk pointer to chunk
 *  @param[in] size size of chunk.
 */
void free_reserved_memory(void *chunk, size_t size);

#endif
