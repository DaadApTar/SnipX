#ifndef COMPRESSION_H_
#define COMPRESSION_H_

#include <stdlib.h>
#include "build_features.h"

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
