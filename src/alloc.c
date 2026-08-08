#include "alloc.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

void *reserve_memory(size_t size) {
  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
  if (data == MAP_FAILED) return NULL;
  return data;
}

void free_reserved_memory(void *chunk, size_t size) {
  munmap(chunk, size);
}

void *rereserve_memory(void *chunk, size_t old_size, size_t new_size) {
  if (chunk == NULL) return NULL;
  void *data = reserve_memory(new_size);
  if (data == NULL) return NULL;

  size_t copy_size = old_size < new_size ? old_size : new_size;
  memcpy(data, chunk, copy_size);

  free_reserved_memory(chunk, old_size);

  return data;
}
