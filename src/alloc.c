#define _GNU_SOURCE
#include "alloc.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

void *reserve_memory(size_t size) {
  void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (data == MAP_FAILED) return NULL;
  return data;
}

void free_reserved_memory(void *chunk, size_t size) {
  munmap(chunk, size);
}

void *rereserve_memory(void *chunk, size_t old_size, size_t new_size) {
  if (chunk == NULL) return NULL;
  void *data = mremap(chunk, old_size, new_size, MREMAP_MAYMOVE);

  if (data == MAP_FAILED) return NULL;
  return data;
}
