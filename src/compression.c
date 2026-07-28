#include "compression.h"
#include <string.h>
#include <stdint.h>

#ifdef FEATURE_ZSTD
#include <zstd.h>
#endif // FEATURE_ZSTD

#ifdef FEATURE_LZ4
#include <lz4.h>
#endif // FEATURE_LZ4

compression_algorithm dispatch_string(char *compression) {
  if (compression == NULL || strcasecmp(compression, NONE_STRING) == 0) return COMPRESSION_NONE;
#ifdef FEATURE_ZSTD
  if (strcasecmp(compression, ZSTD_STRING) == 0) return COMPRESSION_ZSTD;
#endif // FEATURE_ZSTD
#ifdef FEATURE_LZ4
  if (strcasecmp(compression, LZ4_STRING) == 0) return COMPRESSION_LZ4;
#endif // FEATURE_LZ4
  return COMPRESSION_INVALID;
}

algorithm_wrapper dispatch_algorithm(compression_algorithm algorithm) {
  switch (algorithm) {
  case COMPRESSION_NONE: return none_wrapper;
#ifdef FEATURE_ZSTD
  case COMPRESSION_ZSTD: return zstd_wrapper;
#endif // FEATURE_ZSTD
#ifdef FEATURE_LZ4
  case COMPRESSION_LZ4: return lz4_wrapper;
#endif // FEATURE_LZ4
  case COMPRESSION_INVALID:
  default: return none_wrapper;
  }
}

size_t get_compression_bound(compression_algorithm algorithm, size_t src_size) {
  switch (algorithm) {
  case COMPRESSION_NONE: return src_size;
#ifdef FEATURE_ZSTD
  case COMPRESSION_ZSTD: return ZSTD_COMPRESSBOUND(src_size);
#endif // FEATURE_ZSTD
#ifdef FEATURE_LZ4
  case COMPRESSION_LZ4: return LZ4_COMPRESSBOUND(src_size);
#endif // FEATURE_LZ4
  case COMPRESSION_INVALID:
  default: return src_size;
  }
}

void xor_delta(char *dst, char *data1, char *data2, size_t size) {
  while (size >= 8) {
    uint64_t chunk1, chunk2 = 0;

    memcpy(&chunk1, data1, 8);
    memcpy(&chunk2, data2, 8);
    uint64_t diff = chunk1 ^ chunk2;
    memcpy(dst, &diff, 8);

    size -= 8;
    dst += 8;
    data1 += 8;
    data2 += 8;
  }
  if (size) {
    uint64_t chunk1, chunk2 = 0;

    memcpy(&chunk1, data1, size);
    memcpy(&chunk2, data2, size);
    uint64_t diff = chunk1 ^ chunk2;
    memcpy(dst, &diff, size);
  }
}

// ================COMPRESSION WRAPPERS================

#ifdef FEATURE_ZSTD
size_t zstd_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level) {
  return ZSTD_compress(dst, dst_capacity, src, src_size, level);
}
#endif // FEATURE_ZSTD

#ifdef FEATURE_LZ4
size_t lz4_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level) {
  (void) level; // LZ4 default compression does not take compression level.
  // TODO: consider adding lz4hc as a compression algorithm.
  return LZ4_compress_default(src, dst, src_size, dst_capacity);
}
#endif // FEATURE_LZ4

size_t none_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level) {
  (void) level;
  size_t result = 0;
  if (dst_capacity <= src_size) result = dst_capacity;
  else result = src_size;
  memcpy(dst, src, result);

  return result;
}
