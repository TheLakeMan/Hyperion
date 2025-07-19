#include "tensor.h"
#include "memory_pool.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Tensor structure
struct HyperionTensor {
    float                   *data;
    int                     *shape;
    int                      num_dims;
    HyperionTensorDataType     dtype;
    size_t                   size;
    bool                     is_allocated;
    struct HyperionMemoryPool *memory_pool;
};

// Create a new tensor
    struct HyperionTensor *tensor = (struct HyperionTensor *)malloc(sizeof(struct HyperionTensor));
    if (!tensor)
        return NULL;

    // Calculate total size
    size_t total_size = 1;
    for (int i = 0; i < num_dims; i++) {
        total_size *= shape[i];
    }

    // Allocate shape array
    tensor->shape = (int *)malloc(num_dims * sizeof(int));
    if (!tensor->shape) {
        free(tensor);
        return NULL;
    }
    memcpy(tensor->shape, shape, num_dims * sizeof(int));

    // Allocate data
    tensor->data = (float *)malloc(total_size * sizeof(float));
    if (!tensor->data) {
        free(tensor->shape);
        free(tensor);
        return NULL;
    }

    tensor->num_dims     = num_dims;
    tensor->dtype        = dtype;
    tensor->size         = total_size;
    tensor->is_allocated = true;
    tensor->memory_pool  = NULL;

    return tensor;
}

// Create a tensor using a memory pool
    struct HyperionTensor *tensor = (struct HyperionTensor *)malloc(sizeof(struct HyperionTensor));
    if (!tensor)
        return NULL;

    // Calculate total size
    size_t total_size = 1;
    for (int i = 0; i < num_dims; i++) {
        total_size *= shape[i];
    }

    // Allocate shape array
    tensor->shape = (int *)malloc(num_dims * sizeof(int));
    if (!tensor->shape) {
        free(tensor);
        return NULL;
    }
    memcpy(tensor->shape, shape, num_dims * sizeof(int));

    // Allocate data from pool
    tensor->data = (float *)hyperionMemoryPoolAlloc(pool, total_size * sizeof(float), 16);
    if (!tensor->data) {
        free(tensor->shape);
        free(tensor);
        return NULL;
    }

    tensor->num_dims     = num_dims;
    tensor->dtype        = dtype;
    tensor->size         = total_size;
    tensor->is_allocated = false;
    tensor->memory_pool  = pool;

    return tensor;
}

// Free tensor resources
void hyperionFreeTensor(struct HyperionTensor *tensor)
{
    if (!tensor)
        return;

    if (tensor->is_allocated) {
        free(tensor->data);
    }
    else if (tensor->memory_pool) {
        hyperionMemoryPoolFree(tensor->memory_pool, tensor->data);
    }

    free(tensor->shape);
    free(tensor);
}

// Get tensor shape
const int *hyperionGetTensorShape(const struct HyperionTensor *tensor)
{
    return tensor ? tensor->shape : NULL;
}

// Get tensor data type
HyperionTensorDataType hyperionGetTensorDataType(const struct HyperionTensor *tensor)
{
    return tensor ? tensor->dtype : HYPERION_DTYPE_FLOAT32;
}

// Get tensor size in bytes
size_t hyperionGetTensorSize(const struct HyperionTensor *tensor)
{
    if (!tensor)
        return 0;
    return tensor->size * sizeof(float);
}

// Copy tensor data
bool hyperionCopyTensor(const struct HyperionTensor *src, struct HyperionTensor *dst)
{
    if (!src || !dst || src->size != dst->size)
        return false;

    memcpy(dst->data, src->data, src->size * sizeof(float));
    return true;
}

// Fill tensor with value
void hyperionFillTensor(struct HyperionTensor *tensor, float value)
{
    if (!tensor)
        return;

    for (size_t i = 0; i < tensor->size; i++) {
        tensor->data[i] = value;
    }
}

// Add tensors
                      struct HyperionTensor *result)
{
    if (!a || !b || !result || a->size != b->size || a->size != result->size) {
        return false;
    }

    for (size_t i = 0; i < a->size; i++) {
        result->data[i] = a->data[i] + b->data[i];
    }

    return true;
}

// Multiply tensors
                           struct HyperionTensor *result)
{
    if (!a || !b || !result || a->size != b->size || a->size != result->size) {
        return false;
    }

    for (size_t i = 0; i < a->size; i++) {
        result->data[i] = a->data[i] * b->data[i];
    }

    return true;
}

// Matrix multiplication
                          struct HyperionTensor *result)
{
    if (!a || !b || !result)
        return false;

    // Check dimensions
    if (a->num_dims != 2 || b->num_dims != 2 || result->num_dims != 2) {
        return false;
    }

    int a_rows = a->shape[0];
    int a_cols = a->shape[1];
    int b_rows = b->shape[0];
    int b_cols = b->shape[1];

    if (a_cols != b_rows || result->shape[0] != a_rows || result->shape[1] != b_cols) {
        return false;
    }

    // Perform matrix multiplication
    for (int i = 0; i < a_rows; i++) {
        for (int j = 0; j < b_cols; j++) {
            float sum = 0.0f;
            for (int k = 0; k < a_cols; k++) {
                sum += a->data[i * a_cols + k] * b->data[k * b_cols + j];
            }
            result->data[i * b_cols + j] = sum;
        }
    }

    return true;
}

// Apply activation function
void hyperionApplyActivation(struct HyperionTensor *tensor, HyperionActivationType activation)
{
    if (!tensor)
        return;

    switch (activation) {
    case HYPERION_ACTIVATION_RELU:
        for (size_t i = 0; i < tensor->size; i++) {
            tensor->data[i] = fmaxf(0.0f, tensor->data[i]);
        }
        break;

    case HYPERION_ACTIVATION_SIGMOID:
        for (size_t i = 0; i < tensor->size; i++) {
            tensor->data[i] = 1.0f / (1.0f + expf(-tensor->data[i]));
        }
        break;

    case HYPERION_ACTIVATION_TANH:
        for (size_t i = 0; i < tensor->size; i++) {
            tensor->data[i] = tanhf(tensor->data[i]);
        }
        break;

    default:
        break;
    }
}

// Get tensor data pointer
float *hyperionGetTensorData(struct HyperionTensor *tensor) { return tensor ? tensor->data : NULL; }

// Get tensor element
float hyperionGetTensorElement(const struct HyperionTensor *tensor, const int *indices)
{
    if (!tensor || !indices)
        return 0.0f;

    // Calculate linear index
    int index  = 0;
    int stride = 1;
    for (int i = tensor->num_dims - 1; i >= 0; i--) {
        index += indices[i] * stride;
        stride *= tensor->shape[i];
    }

    return tensor->data[index];
}

// Set tensor element
void hyperionSetTensorElement(struct HyperionTensor *tensor, const int *indices, float value)
{
    if (!tensor || !indices)
        return;

    // Calculate linear index
    int index  = 0;
    int stride = 1;
    for (int i = tensor->num_dims - 1; i >= 0; i--) {
        index += indices[i] * stride;
        stride *= tensor->shape[i];
    }

    tensor->data[index] = value;
}