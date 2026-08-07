#include "compression.h"
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include "recorder.h"

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

compression_wrapper dispatch_compression_algorithm(compression_algorithm algorithm) {
  switch (algorithm) {
  case COMPRESSION_NONE: return none_compression_wrapper;
#ifdef FEATURE_ZSTD
  case COMPRESSION_ZSTD: return zstd_compression_wrapper;
#endif // FEATURE_ZSTD
#ifdef FEATURE_LZ4
  case COMPRESSION_LZ4: return lz4_compression_wrapper;
#endif // FEATURE_LZ4
  case COMPRESSION_INVALID:
  default: return none_compression_wrapper;
  }
}

decompression_wrapper dispatch_decompression_algorithm(compression_algorithm algorithm) {
  switch (algorithm) {
  case COMPRESSION_NONE: return none_decompression_wrapper;
#ifdef FEATURE_ZSTD
  case COMPRESSION_ZSTD: return zstd_decompression_wrapper;
#endif // FEATURE_ZSTD
#ifdef FEATURE_LZ4
  case COMPRESSION_LZ4: return lz4_decompression_wrapper;
#endif // FEATURE_LZ4
  case COMPRESSION_INVALID:
  default: return none_decompression_wrapper;
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

void *compression_worker(void *params) {
  compression_context *ctx = (compression_context *)params;
  size_t current = 0;
  size_t item_size = ctx->args.queue->item_size;
  void *compressed_frame = malloc(ctx->args.compression_bound);
  void *delta_frame = malloc(item_size);
  void *prev_frame = malloc(item_size);
  size_t pack_buffer_size = item_size + sizeof(frame_header);
  void *pack_buffer = malloc(pack_buffer_size);

  while (atomic_load(&ctx->running)) {
    pthread_mutex_lock(&ctx->queue_lock);
    while (ctx->args.queue->length == 0 && atomic_load(&ctx->running))
      pthread_cond_wait(&ctx->cond, &ctx->queue_lock);
    if (!atomic_load(&ctx->running)) {
      pthread_mutex_unlock(&ctx->queue_lock);
      break;
    }

    size_t last_index = ctx->args.queue->next_index;
    pthread_mutex_unlock(&ctx->queue_lock);
    if (current > last_index) current = 0;

    while (current < last_index) {
      pthread_mutex_lock(&ctx->queue_lock);

      frame_header header;
      void *current_frame = circular_array_get(ctx->args.queue, current);

      if (current % ctx->args.gop_length == 0) {
        header.type = FRAME_KEYFRAME;
        memcpy(delta_frame, current_frame, item_size);
      }
      else {
        header.type = FRAME_PREDICTED;
        xor_delta(delta_frame, current_frame, prev_frame, item_size);
      }
      memcpy(prev_frame, current_frame, item_size);

      pthread_mutex_unlock(&ctx->queue_lock);
      size_t compressed_size = ctx->args.compression(compressed_frame, ctx->args.compression_bound, delta_frame, item_size, ctx->args.compression_level);
      header.framesize = compressed_size;
      pthread_mutex_lock(&ctx->dst_lock);
      push_frame_with_header(ctx->args.dst, compressed_frame, compressed_size, header, pack_buffer, pack_buffer_size);
      pthread_mutex_unlock(&ctx->dst_lock);
      current++;
    }
  }

  free(compressed_frame);
  free(delta_frame);
  free(prev_frame);
  free(pack_buffer);
  return NULL;
}

int compression_init(compression_context *ctx, compression_args args) {
  ctx->args = args;
  atomic_init(&ctx->running, false);
  if (pthread_mutex_init(&ctx->queue_lock, NULL) != 0) return -1;
  if (pthread_mutex_init(&ctx->dst_lock, NULL) != 0) return -1;
  if (pthread_cond_init(&ctx->cond, NULL) != 0) return -1;
  return 0;
}

int compression_start(compression_context *ctx) {
  if (atomic_load(&ctx->running)) return -1;
  atomic_store(&ctx->running, true);
  return pthread_create(&ctx->thread, 0, compression_worker, ctx);
}

int compression_stop(compression_context *ctx) {
  if (!atomic_load(&ctx->running)) return -1;
  atomic_store(&ctx->running, false);
  pthread_mutex_lock(&ctx->queue_lock);
  pthread_cond_broadcast(&ctx->cond);
  pthread_mutex_unlock(&ctx->queue_lock);
  return pthread_join(ctx->thread, NULL);
}

void compression_destroy(compression_context *ctx) {
  pthread_mutex_destroy(&ctx->queue_lock);
  pthread_mutex_destroy(&ctx->dst_lock);
  pthread_cond_destroy(&ctx->cond);
}

void compression_submit(compression_context *ctx, void *data) {
  pthread_mutex_lock(&ctx->queue_lock);
  circular_array_push(ctx->args.queue, data);
  pthread_cond_signal(&ctx->cond);
  pthread_mutex_unlock(&ctx->queue_lock);
}

// ================COMPRESSION WRAPPERS================

#ifdef FEATURE_ZSTD
size_t zstd_compression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level) {
  return ZSTD_compress(dst, dst_capacity, src, src_size, level);
}

size_t zstd_decompression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size) {
  return ZSTD_decompress(dst, dst_capacity, src, src_size);
}
#endif // FEATURE_ZSTD

#ifdef FEATURE_LZ4
size_t lz4_compression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level) {
  (void) level; // LZ4 default compression does not take compression level.
  // TODO: consider adding lz4hc as a compression algorithm.
  return LZ4_compress_default(src, dst, src_size, dst_capacity);
}

size_t lz4_decompression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size) {
  return LZ4_decompress_safe(src, dst, src_size, dst_capacity);
}
#endif // FEATURE_LZ4

size_t none_compression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level) {
  (void) level;
  size_t result = 0;
  if (dst_capacity <= src_size) result = dst_capacity;
  else result = src_size;
  memcpy(dst, src, result);

  return result;
}

size_t none_decompression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size) {
  size_t result = 0;
  if (dst_capacity <= src_size) result = dst_capacity;
  else result = src_size;
  memcpy(dst, src, result);

  return result;
}
