/**
 * @file memory_efficient_tensor.h
 * @brief Memory-efficient tensor operations for Hyperion
 *
 * This header provides utilities for performing tensor operations with minimal
 * memory overhead, including in-place operations, memory pooling, and
 * streaming operations.
 */

#ifndef HYPERION_MEMORY_EFFICIENT_TENSOR_H
#define HYPERION_MEMORY_EFFICIENT_TENSOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tensor data type
 */
typedef enum {
    HYPERION_TENSOR_FLOAT32 = 0,
    HYPERION_TENSOR_FLOAT16 = 1,
    HYPERION_TENSOR_INT8    = 2,
    HYPERION_TENSOR_INT16   = 3,
    HYPERION_TENSOR_INT32   = 4
} HyperionTensorType;

/**
 * @brief Memory allocation strategy
 */
typedef enum {
    HYPERION_MEMORY_STATIC = 0, // Pre-allocated memory
    HYPERION_MEMORY_POOLED = 1, // Memory pooling
    HYPERION_MEMORY_STREAM = 2  // Streaming memory
} HyperionMemoryStrategy;

/**
 * @brief Tensor shape information
 */
typedef struct {
    size_t *dims;       // Array of dimension sizes
    size_t  num_dims;   // Number of dimensions
    size_t  total_size; // Total number of elements
} HyperionTensorShape;

/**
 * @brief Memory-efficient tensor
 */
typedef struct {
    void                *data;          // Pointer to tensor data
    HyperionTensorType     type;          // Data type
    HyperionTensorShape    shape;         // Shape information
    HyperionMemoryStrategy strategy;      // Memory allocation strategy
    size_t               memory_usage;  // Current memory usage
    bool                 is_contiguous; // Whether data is contiguous
    void                *memory_pool;   // Memory pool for pooled strategy
    size_t               pool_size;     // Size of memory pool
} HyperionMemoryEfficientTensor;

/**
 * @brief Create a memory-efficient tensor
 *
 * @param shape Tensor shape
 * @param type Data type
 * @param strategy Memory allocation strategy
 * @return Pointer to created tensor or NULL on failure
 */
HyperionMemoryEfficientTensor *hyperionCreateMemoryEfficientTensor(const HyperionTensorShape *shape,
                                                               HyperionTensorType         type,
                                                               HyperionMemoryStrategy     strategy);

/**
 * @brief Free a memory-efficient tensor
 *
 * @param tensor Tensor to free
 */
void hyperionFreeMemoryEfficientTensor(HyperionMemoryEfficientTensor *tensor);

/**
 * @brief Perform in-place tensor addition
 *
 * @param dest Destination tensor
 * @param src Source tensor
 * @return true if successful, false on failure
 */
bool hyperionTensorAddInPlace(HyperionMemoryEfficientTensor       *dest,
                            const HyperionMemoryEfficientTensor *src);

/**
 * @brief Perform in-place tensor multiplication
 *
 * @param dest Destination tensor
 * @param src Source tensor
 * @return true if successful, false on failure
 */
bool hyperionTensorMulInPlace(HyperionMemoryEfficientTensor       *dest,
                            const HyperionMemoryEfficientTensor *src);

/**
 * @brief Perform streaming tensor operation
 *
 * @param dest Destination tensor
 * @param src Source tensor
 * @param operation Operation to perform
 * @param chunk_size Size of chunks for streaming
 * @return true if successful, false on failure
 */
bool hyperionTensorStreamOperation(HyperionMemoryEfficientTensor       *dest,
                                 const HyperionMemoryEfficientTensor *src,
                                 void (*operation)(void *, const void *, size_t),
                                 size_t chunk_size);

/**
 * @brief Allocate memory from pool
 *
 * @param tensor Tensor using memory pool
 * @param size Size to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void *hyperionTensorPoolAlloc(HyperionMemoryEfficientTensor *tensor, size_t size);

/**
 * @brief Free memory to pool
 *
 * @param tensor Tensor using memory pool
 * @param ptr Pointer to memory to free
 */
void hyperionTensorPoolFree(HyperionMemoryEfficientTensor *tensor, void *ptr);

/**
 * @brief Get tensor memory usage
 *
 * @param tensor Tensor to check
 * @return Current memory usage in bytes
 */
size_t hyperionGetTensorMemoryUsage(const HyperionMemoryEfficientTensor *tensor);

/**
 * @brief Optimize tensor memory layout
 *
 * @param tensor Tensor to optimize
 * @return true if successful, false on failure
 */
bool hyperionOptimizeTensorMemory(HyperionMemoryEfficientTensor *tensor);

/**
 * @brief Convert tensor to contiguous memory layout
 *
 * @param tensor Tensor to convert
 * @return true if successful, false on failure
 */
bool hyperionMakeTensorContiguous(HyperionMemoryEfficientTensor *tensor);

/**
 * @brief Resize tensor memory pool
 *
 * @param tensor Tensor using memory pool
 * @param new_size New pool size
 * @return true if successful, false on failure
 */
bool hyperionResizeTensorPool(HyperionMemoryEfficientTensor *tensor, size_t new_size);

/**
 * @brief Get tensor data pointer
 *
 * @param tensor Tensor to get data from
 * @return Pointer to tensor data
 */
void *hyperionGetTensorData(const HyperionMemoryEfficientTensor *tensor);

/**
 * @brief Get tensor shape
 *
 * @param tensor Tensor to get shape from
 * @return Tensor shape
 */
HyperionTensorShape hyperionGetTensorShape(const HyperionMemoryEfficientTensor *tensor);

/**
 * @brief Set tensor data
 *
 * @param tensor Tensor to set data for
 * @param data Data to set
 * @param size Size of data in bytes
 * @return true if successful, false on failure
 */
bool hyperionSetTensorData(HyperionMemoryEfficientTensor *tensor, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_MEMORY_EFFICIENT_TENSOR_H */