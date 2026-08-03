#ifndef COMPRESSION_H_
#define COMPRESSION_H_

#include <stdlib.h>
#include "build_features.h"
#include "circular_array.h"
#include "dynamic_circular_array.h"
#include <stdatomic.h>

// TODO: introduce default compression
#define ZSTD_STRING "zstd"
#define LZ4_STRING "lz4"
#define NONE_STRING "none"

typedef enum {
  COMPRESSION_NONE,
#ifdef FEATURE_ZSTD
  COMPRESSION_ZSTD,
#endif // FEATURE_ZSTD
#ifdef FEATURE_LZ4
  COMPRESSION_LZ4,
#endif // FEATURE_LZ4

  COMPRESSION_INVALID
} compression_algorithm;

/** @brief Compression algorithm wrapper.
 *  @param[out] dst allocated compression destination.
 *  @param[in] dst_capacity size of dst.
 *  @param[in] src source data.
 *  @param[in] src_size size of src.
 *  @param[in] level compression level.
 *  @return size of new data.
 */
typedef size_t (*compression_wrapper)(void *dst, size_t dst_capacity, void *src, size_t src_size, int level);

/** @brief Decompression algorithm wrapper.
 *  @param[out] dst allocated compression destination.
 *  @param[in] dst_capacity size of dst.
 *  @param[in] src source data.
 *  @param[in] src_size size of src.
 *  @return size of new data.
 */
typedef size_t (*decompression_wrapper)(void *dst, size_t dst_capacity, void *src, size_t src_size);

/** @brief Dispatches string to relative wrapper.
 *  @param[in] compression string given by the user.
 *  @return Wrapper around compression algorithm on success. On error, NULL is returned.
 */
compression_algorithm dispatch_string(char *compression);

/** @brief Dispatches enum to relative wrapper.
 *  @param[in] algorithm compression algorithm.
 *  @return Wrapper around compression algorithm on success. On error, NULL is returned.
 */
compression_wrapper dispatch_compression_algorithm(compression_algorithm algorithm);

/** @brief Dispatches enum to relative wrapper.
 *  @param[in] algorithm compression algorithm.
 *  @return Wrapper around decompression algorithm on success. On error, NULL is returned.
 */
decompression_wrapper dispatch_decompression_algorithm(compression_algorithm algorithm);

/** @param[in] algorithm compression algorithm.
 *  @param[in] src_size size of src.
 *  @return compression bound for specific algorithm.
 */
size_t get_compression_bound(compression_algorithm algorithm, size_t src_size);

/** @brief Calculates delta between two given data with XOR.
 *  @param[out] dst allocated destination.
 *  @param[in] data1 first data to XOR.
 *  @param[in] data2 seconds data to XOR.
 *  @param[in] size size dst, data1 and data2. Expected to be the same.
 */
void xor_delta(char *dst, char *data1, char *data2, size_t size);

typedef struct {
  circular_array *queue;
  compression_wrapper compression;
  dynamic_circular_array *dst;
  size_t compression_bound;
  int compression_level;
} compression_args;

typedef struct {
  atomic_bool running;
  pthread_t thread;
  pthread_mutex_t queue_lock;
  pthread_mutex_t dst_lock;
  pthread_cond_t cond;
  compression_args args;
} compression_context;

/** @brief initialises compression thread context.
 *  @param[out] ctx compression context to init.
 *  @param[in] args compression_arguments.
 *  @return 0 on success, -1 on error.
 */
int compression_init(compression_context *ctx, compression_args args);

/** @brief Starts the compression worker.
 *  @param[in] ctx compression thread context.
 *  @return 0 on success, -1 on error.
 */
int compression_start(compression_context *ctx);

/** @brief Stops the compression worker.
 *  @param[in] ctx compression thread context.
 *  @return 0 on success, -1 on error.
 */
int compression_stop(compression_context *ctx);

/** @brief Deinits the compression worker.
 *  @param[in] ctx compression thread context.
 */
void compression_destroy(compression_context *ctx);

/** @brief Queueing data for compression.
 *  @param[in] array queue ring buffer.
 *  @param[in] data data chunk.
 *  @return 0 on success, -1 on error.
 */
void compression_submit(compression_context *ctx, void *data);

/** @brief Worker that compresses incoming data.
 *  @param[in] params #compression_args.
 */
void *compression_worker(void *params);

// ================COMPRESSION WRAPPERS================

#ifdef FEATURE_ZSTD

/** @brief ZSTD compression wrapper.
 *  @copydoc compression_wrapper
 */
size_t zstd_compression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level);

/** @brief ZSTD compression wrapper.
 *  @copydoc decompression_wrapper
 */
size_t zstd_decompression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size);

#endif // FEATURE_ZSTD

#ifdef FEATURE_LZ4

/** @brief LZ4 wrapper.
 *  @copydoc compresion_wrapper
 */
size_t lz4_compression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level);

/** @brief LZ4 compression wrapper.
 *  @copydoc decompression_wrapper
 */
size_t lz4_decompression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size);

#endif // FEATURE_LZ4

/** @brief Wrapper around no algorithm.
 *  @copydoc compression_wrapper
 *  @note returned data == initial data (copied for the sake of consistency) and initial size == output size.
 */
size_t none_compression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size, int level);

/** @brief ZSTD compression wrapper.
 *  @copydoc decompression_wrapper
 *  @note returned data == initial data (copied for the sake of consistency) and initial size == output size.
 */
size_t none_decompression_wrapper(void *dst, size_t dst_capacity, void *src, size_t src_size);

#endif
