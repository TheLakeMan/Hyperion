/**
 * @file tensor.h
 * @brief Tensor operations for Hyperion
 *
 * This header defines tensor data structures and operations for
 * efficient computation in the Hyperion AI framework.
 */

#ifndef HYPERION_TENSOR_H
#define HYPERION_TENSOR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declarations
 */
struct HyperionTensor;
struct HyperionMemoryPool;

/**
 * Tensor data types
 */
typedef enum {
    HYPERION_DTYPE_FLOAT32,
    HYPERION_DTYPE_FLOAT16,
    HYPERION_DTYPE_INT32,
    HYPERION_DTYPE_INT16,
    HYPERION_DTYPE_INT8,
    HYPERION_DTYPE_UINT8
} HyperionTensorDataType;

/**
 * Activation function types
 */
typedef enum {
    HYPERION_ACTIVATION_NONE,
    HYPERION_ACTIVATION_RELU,
    HYPERION_ACTIVATION_SIGMOID,
    HYPERION_ACTIVATION_TANH,
    HYPERION_ACTIVATION_SOFTMAX,
    HYPERION_ACTIVATION_GELU
} HyperionActivationType;

/**
 * Create a new tensor
 *
 * @param shape Array of dimension sizes
 * @param num_dims Number of dimensions
 * @param dtype Data type of tensor elements
 * @return New tensor instance (must be freed with hyperionFreeTensor)
 */
struct HyperionTensor *hyperionCreateTensor(const int *shape, int num_dims, HyperionTensorDataType dtype);

/**
 * Create a tensor using a memory pool
 *
 * @param shape Array of dimension sizes
 * @param num_dims Number of dimensions
 * @param dtype Data type of tensor elements
 * @param pool Memory pool to allocate from
 * @return New tensor instance (must be freed with hyperionFreeTensor)
 */
struct HyperionTensor *hyperionCreateTensorFromPool(const int *shape, int num_dims, 
                                                   HyperionTensorDataType dtype, 
                                                   struct HyperionMemoryPool *pool);

/**
 * Free tensor resources
 *
 * @param tensor Tensor to free
 */
void hyperionFreeTensor(struct HyperionTensor *tensor);

/**
 * Get tensor shape
 *
 * @param tensor Tensor to query
 * @return Array of dimension sizes (read-only)
 */
const int *hyperionGetTensorShape(const struct HyperionTensor *tensor);

/**
 * Get tensor data type
 *
 * @param tensor Tensor to query
 * @return Data type of tensor elements
 */
HyperionTensorDataType hyperionGetTensorDataType(const struct HyperionTensor *tensor);

/**
 * Get tensor size in bytes
 *
 * @param tensor Tensor to query
 * @return Total size in bytes
 */
size_t hyperionGetTensorSize(const struct HyperionTensor *tensor);

/**
 * Copy tensor data
 *
 * @param src Source tensor
 * @param dst Destination tensor (must have same size)
 * @return true on success, false on failure
 */
bool hyperionCopyTensor(const struct HyperionTensor *src, struct HyperionTensor *dst);

/**
 * Fill tensor with a constant value
 *
 * @param tensor Tensor to fill
 * @param value Value to fill with
 */
void hyperionFillTensor(struct HyperionTensor *tensor, float value);

/**
 * Element-wise tensor addition: result = a + b
 *
 * @param a First input tensor
 * @param b Second input tensor
 * @param result Output tensor (must have same size as inputs)
 * @return true on success, false on failure
 */
bool hyperionAddTensors(const struct HyperionTensor *a, const struct HyperionTensor *b,
                       struct HyperionTensor *result);

/**
 * Element-wise tensor multiplication: result = a * b
 *
 * @param a First input tensor
 * @param b Second input tensor
 * @param result Output tensor (must have same size as inputs)
 * @return true on success, false on failure
 */
bool hyperionMultiplyTensors(const struct HyperionTensor *a, const struct HyperionTensor *b,
                            struct HyperionTensor *result);

/**
 * Matrix multiplication: result = a * b
 *
 * @param a First input matrix (2D tensor)
 * @param b Second input matrix (2D tensor)
 * @param result Output matrix (2D tensor)
 * @return true on success, false on failure
 */
bool hyperionMatrixMultiply(const struct HyperionTensor *a, const struct HyperionTensor *b,
                           struct HyperionTensor *result);

/**
 * Apply activation function to tensor
 *
 * @param tensor Tensor to modify (in-place operation)
 * @param activation Activation function to apply
 */
void hyperionApplyActivation(struct HyperionTensor *tensor, HyperionActivationType activation);

/**
 * Get tensor data pointer
 *
 * @param tensor Tensor to query
 * @return Pointer to tensor data (direct access)
 */
float *hyperionGetTensorData(struct HyperionTensor *tensor);

/**
 * Get tensor element at specified indices
 *
 * @param tensor Tensor to query
 * @param indices Array of indices for each dimension
 * @return Element value
 */
float hyperionGetTensorElement(const struct HyperionTensor *tensor, const int *indices);

/**
 * Set tensor element at specified indices
 *
 * @param tensor Tensor to modify
 * @param indices Array of indices for each dimension
 * @param value Value to set
 */
void hyperionSetTensorElement(struct HyperionTensor *tensor, const int *indices, float value);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_TENSOR_H */