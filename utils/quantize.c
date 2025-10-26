/**
 * Hyperion Quantization Utilities Implementation
 * 
 * This file implements the quantization utilities for Hyperion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "../core/memory.h"
#include "../core/io.h"
#include "quantize.h"

/* ----------------- Internal Constants and Variables ----------------- */

/* Activation function lookup tables */
#define ACTIVATION_TABLE_SIZE 8192
static float *activationTableReLU = NULL;
static float *activationTableSigmoid = NULL;
static float *activationTableTanh = NULL;
static float *activationTableGELU = NULL;

/* Activation range for lookup tables */
#define ACTIVATION_MIN -8.0f
#define ACTIVATION_MAX 8.0f

/* ----------------- Matrix Creation and Destruction ----------------- */

/**
 * Create a 4-bit quantized matrix
 */
HyperionMatrix4bit* hyperionCreateMatrix4bit(uint32_t rows, uint32_t cols) {
    HyperionMatrix4bit *matrix = (HyperionMatrix4bit*)HYPERION_MALLOC(sizeof(HyperionMatrix4bit));
    if (!matrix) {
        return NULL;
    }
    
    /* Calculate data size (4-bit values, 2 per byte) */
    size_t dataSize = (rows * cols + 1) / 2; /* Ceiling division */
    
    matrix->data = (uint8_t*)HYPERION_MALLOC(dataSize);
    if (!matrix->data) {
        HYPERION_FREE(matrix);
        return NULL;
    }
    
    /* Initialize the matrix */
    memset(matrix->data, 0, dataSize);
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->scale = 1.0f;
    matrix->zeroPoint = 0.0f;
    
    return matrix;
}

/**
 * Destroy a 4-bit quantized matrix
 */
void hyperionDestroyMatrix4bit(HyperionMatrix4bit *matrix) {
    if (!matrix) {
        return;
    }
    
    if (matrix->data) {
        HYPERION_FREE(matrix->data);
    }
    
    HYPERION_FREE(matrix);
}

/**
 * Create an 8-bit quantized matrix
 */
HyperionMatrix8bit* hyperionCreateMatrix8bit(uint32_t rows, uint32_t cols) {
    HyperionMatrix8bit *matrix = (HyperionMatrix8bit*)HYPERION_MALLOC(sizeof(HyperionMatrix8bit));
    if (!matrix) {
        return NULL;
    }
    
    matrix->data = (int8_t*)HYPERION_MALLOC(rows * cols);
    if (!matrix->data) {
        HYPERION_FREE(matrix);
        return NULL;
    }
    
    /* Initialize the matrix */
    memset(matrix->data, 0, rows * cols);
    matrix->rows = rows;
    matrix->cols = cols;
    matrix->scale = 1.0f;
    matrix->zeroPoint = 0.0f;
    
    return matrix;
}

/**
 * Destroy an 8-bit quantized matrix
 */
void hyperionDestroyMatrix8bit(HyperionMatrix8bit *matrix) {
    if (!matrix) {
        return;
    }
    
    if (matrix->data) {
        HYPERION_FREE(matrix->data);
    }
    
    HYPERION_FREE(matrix);
}

/**
 * Create a FP32 matrix
 */
HyperionMatrixFP32* hyperionCreateMatrixFP32(uint32_t rows, uint32_t cols) {
    HyperionMatrixFP32 *matrix = (HyperionMatrixFP32*)HYPERION_MALLOC(sizeof(HyperionMatrixFP32));
    if (!matrix) {
        return NULL;
    }
    
    matrix->data = (float*)HYPERION_MALLOC(rows * cols * sizeof(float));
    if (!matrix->data) {
        HYPERION_FREE(matrix);
        return NULL;
    }
    
    /* Initialize the matrix */
    memset(matrix->data, 0, rows * cols * sizeof(float));
    matrix->rows = rows;
    matrix->cols = cols;
    
    return matrix;
}

/**
 * Destroy a FP32 matrix
 */
void hyperionDestroyMatrixFP32(HyperionMatrixFP32 *matrix) {
    if (!matrix) {
        return;
    }
    
    if (matrix->data) {
        HYPERION_FREE(matrix->data);
    }
    
    HYPERION_FREE(matrix);
}

/* ----------------- Quantization and Dequantization ----------------- */

/**
 * Quantize a FP32 matrix to 4-bit
 */
HyperionMatrix4bit* hyperionQuantizeFP32To4bit(const HyperionMatrixFP32 *input) {
    if (!input || !input->data) {
        return NULL;
    }
    
    HyperionMatrix4bit *output = hyperionCreateMatrix4bit(input->rows, input->cols);
    if (!output) {
        return NULL;
    }
    
    /* Find min and max values */
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    
    size_t size = input->rows * input->cols;
    for (size_t i = 0; i < size; i++) {
        if (input->data[i] < minVal) minVal = input->data[i];
        if (input->data[i] > maxVal) maxVal = input->data[i];
    }
    
    /* Compute scale and zero point */
    output->zeroPoint = minVal;
    output->scale = (maxVal - minVal) / 15.0f; /* 15 is the max value in 4 bits */
    
    if (output->scale == 0) {
        /* All values are the same */
        output->scale = 1.0f;
    }
    
    /* Quantize the values */
    for (size_t i = 0; i < size; i += 2) {
        /* Quantize first value */
        int val1 = (int)((input->data[i] - output->zeroPoint) / output->scale + 0.5f);
        if (val1 < 0) val1 = 0;
        if (val1 > 15) val1 = 15;
        
        /* Quantize second value (or use 0 if at the end) */
        int val2 = 0;
        if (i + 1 < size) {
            val2 = (int)((input->data[i + 1] - output->zeroPoint) / output->scale + 0.5f);
            if (val2 < 0) val2 = 0;
            if (val2 > 15) val2 = 15;
        }
        
        /* Pack two 4-bit values into one byte */
        output->data[i / 2] = (val1 << 4) | val2;
    }
    
    return output;
}

/**
 * Quantize a FP32 matrix to 8-bit
 */
HyperionMatrix8bit* hyperionQuantizeFP32To8bit(const HyperionMatrixFP32 *input) {
    if (!input || !input->data) {
        return NULL;
    }
    
    HyperionMatrix8bit *output = hyperionCreateMatrix8bit(input->rows, input->cols);
    if (!output) {
        return NULL;
    }
    
    /* Find min and max values */
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    
    size_t size = input->rows * input->cols;
    for (size_t i = 0; i < size; i++) {
        if (input->data[i] < minVal) minVal = input->data[i];
        if (input->data[i] > maxVal) maxVal = input->data[i];
    }
    
    /* Compute scale and zero point */
    output->zeroPoint = minVal;
    output->scale = (maxVal - minVal) / 254.0f; /* Use -127 to 127 range for better precision */
    
    if (output->scale == 0) {
        /* All values are the same */
        output->scale = 1.0f;
    }
    
    /* Quantize the values */
    for (size_t i = 0; i < size; i++) {
        int val = (int)((input->data[i] - output->zeroPoint) / output->scale + 0.5f);
        if (val < -127) val = -127;
        if (val > 127) val = 127;
        
        output->data[i] = (int8_t)val;
    }
    
    return output;
}

/**
 * Dequantize a 4-bit matrix to FP32
 */
HyperionMatrixFP32* hyperionDequantize4bitToFP32(const HyperionMatrix4bit *input) {
    if (!input || !input->data) {
        return NULL;
    }
    
    HyperionMatrixFP32 *output = hyperionCreateMatrixFP32(input->rows, input->cols);
    if (!output) {
        return NULL;
    }
    
    size_t size = input->rows * input->cols;
    for (size_t i = 0; i < size; i += 2) {
        /* Unpack first value */
        int val1 = (input->data[i / 2] >> 4) & 0x0F;
        output->data[i] = val1 * input->scale + input->zeroPoint;
        
        /* Unpack second value (if not at the end) */
        if (i + 1 < size) {
            int val2 = input->data[i / 2] & 0x0F;
            output->data[i + 1] = val2 * input->scale + input->zeroPoint;
        }
    }
    
    return output;
}

/**
 * Dequantize an 8-bit matrix to FP32
 */
HyperionMatrixFP32* hyperionDequantize8bitToFP32(const HyperionMatrix8bit *input) {
    if (!input || !input->data) {
        return NULL;
    }
    
    HyperionMatrixFP32 *output = hyperionCreateMatrixFP32(input->rows, input->cols);
    if (!output) {
        return NULL;
    }
    
    size_t size = input->rows * input->cols;
    for (size_t i = 0; i < size; i++) {
        output->data[i] = input->data[i] * input->scale + input->zeroPoint;
    }
    
    return output;
}

/* ----------------- Matrix Operations ----------------- */

/**
 * Matrix multiplication: C = A * B
 */
int hyperionMatrixMultiply(const void *a, const void *b, void *c, HyperionPrecision precision) {
    /* Implementation varies based on precision */
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const HyperionMatrixFP32 *A = (const HyperionMatrixFP32 *)a;
            const HyperionMatrixFP32 *B = (const HyperionMatrixFP32 *)b;
            HyperionMatrixFP32 *C = (HyperionMatrixFP32 *)c;
            
            if (!A || !B || !C || !A->data || !B->data || !C->data) {
                return -1;
            }
            
            if (A->cols != B->rows || C->rows != A->rows || C->cols != B->cols) {
                return -1;  /* Incompatible dimensions */
            }
            
            /* Compute matrix multiplication */
            for (uint32_t i = 0; i < A->rows; i++) {
                for (uint32_t j = 0; j < B->cols; j++) {
                    float sum = 0.0f;
                    for (uint32_t k = 0; k < A->cols; k++) {
                        sum += A->data[i * A->cols + k] * B->data[k * B->cols + j];
                    }
                    C->data[i * C->cols + j] = sum;
                }
            }
            
            return 0;
        }
        
        case HYPERION_PRECISION_INT8: {
            /* Simplified implementation for 8-bit quantized matrices */
            /* For a real implementation, we would use SIMD instructions and optimized code */
            const HyperionMatrix8bit *A = (const HyperionMatrix8bit *)a;
            const HyperionMatrix8bit *B = (const HyperionMatrix8bit *)b;
            HyperionMatrix8bit *C = (HyperionMatrix8bit *)c;
            
            if (!A || !B || !C || !A->data || !B->data || !C->data) {
                return -1;
            }
            
            if (A->cols != B->rows || C->rows != A->rows || C->cols != B->cols) {
                return -1;  /* Incompatible dimensions */
            }
            
            /* Compute scale for output */
            C->scale = A->scale * B->scale;
            C->zeroPoint = 0.0f;
            
            /* Compute matrix multiplication */
            for (uint32_t i = 0; i < A->rows; i++) {
                for (uint32_t j = 0; j < B->cols; j++) {
                    int32_t sum = 0;
                    for (uint32_t k = 0; k < A->cols; k++) {
                        sum += (int32_t)A->data[i * A->cols + k] * (int32_t)B->data[k * B->cols + j];
                    }
                    
                    /* Clip to INT8 range */
                    if (sum < -127) sum = -127;
                    if (sum > 127) sum = 127;
                    
                    C->data[i * C->cols + j] = (int8_t)sum;
                }
            }
            
            return 0;
        }
        
        case HYPERION_PRECISION_INT4: {
            /* Simplified implementation for 4-bit quantized matrices */
            /* This is a naive implementation for demonstration purposes */
            const HyperionMatrix4bit *A = (const HyperionMatrix4bit *)a;
            const HyperionMatrix4bit *B = (const HyperionMatrix4bit *)b;
            HyperionMatrix4bit *C = (HyperionMatrix4bit *)c;
            
            if (!A || !B || !C || !A->data || !B->data || !C->data) {
                return -1;
            }
            
            if (A->cols != B->rows || C->rows != A->rows || C->cols != B->cols) {
                return -1;  /* Incompatible dimensions */
            }
            
            /* For a real implementation, we would dequantize, compute, and requantize */
            /* This is too complex for this simplified version */
            
            /* Instead, we'll just dequantize to FP32, compute, and requantize */
            HyperionMatrixFP32 *Afp32 = hyperionDequantize4bitToFP32(A);
            HyperionMatrixFP32 *Bfp32 = hyperionDequantize4bitToFP32(B);
            HyperionMatrixFP32 *Cfp32 = hyperionCreateMatrixFP32(C->rows, C->cols);
            
            if (!Afp32 || !Bfp32 || !Cfp32) {
                if (Afp32) hyperionDestroyMatrixFP32(Afp32);
                if (Bfp32) hyperionDestroyMatrixFP32(Bfp32);
                if (Cfp32) hyperionDestroyMatrixFP32(Cfp32);
                return -1;
            }
            
            /* Compute in FP32 */
            hyperionMatrixMultiply(Afp32, Bfp32, Cfp32, HYPERION_PRECISION_FP32);
            
            /* Requantize */
            HyperionMatrix4bit *Cnew = hyperionQuantizeFP32To4bit(Cfp32);
            if (!Cnew) {
                hyperionDestroyMatrixFP32(Afp32);
                hyperionDestroyMatrixFP32(Bfp32);
                hyperionDestroyMatrixFP32(Cfp32);
                return -1;
            }
            
            /* Copy results back to C */
            size_t dataSize = (C->rows * C->cols + 1) / 2;
            memcpy(C->data, Cnew->data, dataSize);
            C->scale = Cnew->scale;
            C->zeroPoint = Cnew->zeroPoint;
            
            /* Clean up */
            hyperionDestroyMatrixFP32(Afp32);
            hyperionDestroyMatrixFP32(Bfp32);
            hyperionDestroyMatrixFP32(Cfp32);
            hyperionDestroyMatrix4bit(Cnew);
            
            return 0;
        }
        
        default:
            return -1;  /* Unknown precision */
    }
}

/**
 * Matrix addition: C = A + B
 */
int hyperionMatrixAdd(const void *a, const void *b, void *c, HyperionPrecision precision) {
    /* Implementation varies based on precision */
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const HyperionMatrixFP32 *A = (const HyperionMatrixFP32 *)a;
            const HyperionMatrixFP32 *B = (const HyperionMatrixFP32 *)b;
            HyperionMatrixFP32 *C = (HyperionMatrixFP32 *)c;
            
            if (!A || !B || !C || !A->data || !B->data || !C->data) {
                return -1;
            }
            
            if (A->rows != B->rows || A->cols != B->cols ||
                C->rows != A->rows || C->cols != A->cols) {
                return -1;  /* Incompatible dimensions */
            }
            
            /* Compute matrix addition */
            size_t size = A->rows * A->cols;
            for (size_t i = 0; i < size; i++) {
                C->data[i] = A->data[i] + B->data[i];
            }
            
            return 0;
        }
        
        /* Other precision implementations follow a similar pattern */
        /* For brevity, they're not included here */
        
        default:
            return -1;  /* Unknown precision */
    }
}

/**
 * Apply activation function to matrix
 */
int hyperionMatrixActivation(const void *input, void *output, int activation, HyperionPrecision precision) {
    /* Implementation varies based on precision */
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const HyperionMatrixFP32 *in = (const HyperionMatrixFP32 *)input;
            HyperionMatrixFP32 *out = (HyperionMatrixFP32 *)output;
            
            if (!in || !out || !in->data || !out->data) {
                return -1;
            }
            
            if (in->rows != out->rows || in->cols != out->cols) {
                return -1;  /* Incompatible dimensions */
            }
            
            /* Apply activation function */
            size_t size = in->rows * in->cols;
            switch (activation) {
                case 0:  /* None */
                    memcpy(out->data, in->data, size * sizeof(float));
                    break;
                
                case 1:  /* ReLU */
                    for (size_t i = 0; i < size; i++) {
                        out->data[i] = hyperionActivationReLU(in->data[i]);
                    }
                    break;
                
                case 2:  /* Sigmoid */
                    for (size_t i = 0; i < size; i++) {
                        out->data[i] = hyperionActivationSigmoid(in->data[i]);
                    }
                    break;
                
                case 3:  /* Tanh */
                    for (size_t i = 0; i < size; i++) {
                        out->data[i] = hyperionActivationTanh(in->data[i]);
                    }
                    break;
                
                case 4:  /* GELU */
                    for (size_t i = 0; i < size; i++) {
                        out->data[i] = hyperionActivationGELU(in->data[i]);
                    }
                    break;
                
                default:
                    return -1;  /* Unknown activation */
            }
            
            return 0;
        }
        
        /* Other precision implementations follow a similar pattern */
        /* For brevity, they're not included here */
        
        default:
            return -1;  /* Unknown precision */
    }
}

/* ----------------- Vector Operations ----------------- */

/**
 * Vector dot product
 */
float hyperionVectorDot(const void *a, const void *b, uint32_t length, HyperionPrecision precision) {
    /* Implementation varies based on precision */
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const float *A = (const float *)a;
            const float *B = (const float *)b;
            
            if (!A || !B) {
                return 0.0f;
            }
            
            /* Compute dot product */
            float sum = 0.0f;
            for (uint32_t i = 0; i < length; i++) {
                sum += A[i] * B[i];
            }
            
            return sum;
        }
        
        /* Other precision implementations follow a similar pattern */
        /* For brevity, they're not included here */
        
        default:
            return 0.0f;  /* Unknown precision */
    }
}

/**
 * Vector L2 norm (Euclidean distance)
 */
float hyperionVectorL2Norm(const void *a, uint32_t length, HyperionPrecision precision) {
    /* Implementation varies based on precision */
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const float *A = (const float *)a;
            
            if (!A) {
                return 0.0f;
            }
            
            /* Compute L2 norm */
            float sum = 0.0f;
            for (uint32_t i = 0; i < length; i++) {
                sum += A[i] * A[i];
            }
            
            return sqrtf(sum);
        }
        
        /* Other precision implementations follow a similar pattern */
        /* For brevity, they're not included here */
        
        default:
            return 0.0f;  /* Unknown precision */
    }
}

/**
 * Vector cosine similarity
 */
float hyperionVectorCosineSimilarity(const void *a, const void *b, uint32_t length, HyperionPrecision precision) {
    /* Implementation varies based on precision */
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const float *A = (const float *)a;
            const float *B = (const float *)b;
            
            if (!A || !B) {
                return 0.0f;
            }
            
            /* Compute cosine similarity */
            float dot = hyperionVectorDot(A, B, length, precision);
            float normA = hyperionVectorL2Norm(A, length, precision);
            float normB = hyperionVectorL2Norm(B, length, precision);
            
            if (normA == 0.0f || normB == 0.0f) {
                return 0.0f;
            }
            
            return dot / (normA * normB);
        }
        
        /* Other precision implementations follow a similar pattern */
        /* For brevity, they're not included here */
        
        default:
            return 0.0f;  /* Unknown precision */
    }
}

/* ----------------- Activation Functions ----------------- */

/**
 * ReLU activation function
 */
float hyperionActivationReLU(float x) {
    return x > 0.0f ? x : 0.0f;
}

/**
 * Sigmoid activation function
 */
float hyperionActivationSigmoid(float x) {
    /* Check for lookup table */
    if (activationTableSigmoid) {
        /* Convert x to table index */
        int index = (int)((x - ACTIVATION_MIN) / (ACTIVATION_MAX - ACTIVATION_MIN) * ACTIVATION_TABLE_SIZE);
        if (index < 0) index = 0;
        if (index >= ACTIVATION_TABLE_SIZE) index = ACTIVATION_TABLE_SIZE - 1;
        
        return activationTableSigmoid[index];
    }
    
    /* Compute directly */
    if (x < -10.0f) return 0.0f;
    if (x > 10.0f) return 1.0f;
    return 1.0f / (1.0f + expf(-x));
}

/**
 * Tanh activation function
 */
float hyperionActivationTanh(float x) {
    /* Check for lookup table */
    if (activationTableTanh) {
        /* Convert x to table index */
        int index = (int)((x - ACTIVATION_MIN) / (ACTIVATION_MAX - ACTIVATION_MIN) * ACTIVATION_TABLE_SIZE);
        if (index < 0) index = 0;
        if (index >= ACTIVATION_TABLE_SIZE) index = ACTIVATION_TABLE_SIZE - 1;
        
        return activationTableTanh[index];
    }
    
    /* Compute directly */
    if (x < -5.0f) return -1.0f;
    if (x > 5.0f) return 1.0f;
    float ex = expf(x);
    float enx = expf(-x);
    return (ex - enx) / (ex + enx);
}

/**
 * GELU activation function
 */
float hyperionActivationGELU(float x) {
    /* Check for lookup table */
    if (activationTableGELU) {
        /* Convert x to table index */
        int index = (int)((x - ACTIVATION_MIN) / (ACTIVATION_MAX - ACTIVATION_MIN) * ACTIVATION_TABLE_SIZE);
        if (index < 0) index = 0;
        if (index >= ACTIVATION_TABLE_SIZE) index = ACTIVATION_TABLE_SIZE - 1;
        
        return activationTableGELU[index];
    }
    
    /* Compute directly (approximate) */
    return 0.5f * x * (1.0f + hyperionActivationTanh(0.7978845608f * (x + 0.044715f * x * x * x)));
}

/**
 * Initialize activation function lookup tables
 */
int hyperionInitActivationTables() {
    /* Allocate tables */
    activationTableReLU = (float*)HYPERION_MALLOC(ACTIVATION_TABLE_SIZE * sizeof(float));
    activationTableSigmoid = (float*)HYPERION_MALLOC(ACTIVATION_TABLE_SIZE * sizeof(float));
    activationTableTanh = (float*)HYPERION_MALLOC(ACTIVATION_TABLE_SIZE * sizeof(float));
    activationTableGELU = (float*)HYPERION_MALLOC(ACTIVATION_TABLE_SIZE * sizeof(float));
    
    if (!activationTableReLU || !activationTableSigmoid ||
        !activationTableTanh || !activationTableGELU) {
        hyperionCleanupActivationTables();
        return -1;
    }
    
    /* Fill tables */
    for (int i = 0; i < ACTIVATION_TABLE_SIZE; i++) {
        float x = ACTIVATION_MIN + (ACTIVATION_MAX - ACTIVATION_MIN) * i / ACTIVATION_TABLE_SIZE;
        
        /* ReLU */
        activationTableReLU[i] = x > 0.0f ? x : 0.0f;
        
        /* Sigmoid */
        activationTableSigmoid[i] = 1.0f / (1.0f + expf(-x));
        
        /* Tanh */
        {
            float ex = expf(x);
            float enx = expf(-x);
            activationTableTanh[i] = (ex - enx) / (ex + enx);
        }
        
        /* GELU (approximate) */
        activationTableGELU[i] = 0.5f * x * (1.0f + activationTableTanh[i]); 
    }
    
    return 0;
}

/**
 * Clean up activation function lookup tables
 */
void hyperionCleanupActivationTables() {
    if (activationTableReLU) {
        HYPERION_FREE(activationTableReLU);
        activationTableReLU = NULL;
    }
    
    if (activationTableSigmoid) {
        HYPERION_FREE(activationTableSigmoid);
        activationTableSigmoid = NULL;
    }
    
    if (activationTableTanh) {
        HYPERION_FREE(activationTableTanh);
        activationTableTanh = NULL;
    }
    
    if (activationTableGELU) {
        HYPERION_FREE(activationTableGELU);
        activationTableGELU = NULL;
    }
}

/* ----------------- Utility Functions ----------------- */

/**
 * Find minimum and maximum values in FP32 array
 */
void hyperionFindMinMax(const float *data, size_t size, float *min, float *max) {
    if (!data || !min || !max || size == 0) {
        return;
    }
    
    float minVal = FLT_MAX;
    float maxVal = -FLT_MAX;
    
    for (size_t i = 0; i < size; i++) {
        if (data[i] < minVal) minVal = data[i];
        if (data[i] > maxVal) maxVal = data[i];
    }
    
    *min = minVal;
    *max = maxVal;
}

void hyperionQuantizeWeights(const float *input, uint8_t *output, size_t count, int bits) {
    if (!input || !output || count == 0) {
        return;
    }

    if (bits != 4 && bits != 8) {
        bits = 8;
    }

    float maxAbs = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float val = fabsf(input[i]);
        if (val > maxAbs) {
            maxAbs = val;
        }
    }

    if (maxAbs < 1e-8f) {
        size_t bytes = (bits == 4) ? (count + 1) / 2 : count;
        memset(output, 0, bytes);
        return;
    }

    const int maxLevel = (1 << (bits - 1)) - 1;
    const float scale  = maxLevel / maxAbs;

    if (bits == 8) {
        for (size_t i = 0; i < count; ++i) {
            int q = (int)lrintf(input[i] * scale);
            if (q > maxLevel) q = maxLevel;
            if (q < -maxLevel - 1) q = -maxLevel - 1;
            output[i] = (uint8_t)(q & 0xFF);
        }
    } else {
        const int bias = 1 << (bits - 1);
        size_t outIdx = 0;
        for (size_t i = 0; i < count; i += 2) {
            int q0 = (int)lrintf(input[i] * scale);
            if (q0 > maxLevel) q0 = maxLevel;
            if (q0 < -maxLevel - 1) q0 = -maxLevel - 1;
            uint8_t nibble0 = (uint8_t)((q0 + bias) & 0x0F);

            uint8_t nibble1 = (uint8_t)bias;
            if (i + 1 < count) {
                int q1 = (int)lrintf(input[i + 1] * scale);
                if (q1 > maxLevel) q1 = maxLevel;
                if (q1 < -maxLevel - 1) q1 = -maxLevel - 1;
                nibble1 = (uint8_t)((q1 + bias) & 0x0F);
            }

            output[outIdx++] = (uint8_t)((nibble0 << 4) | (nibble1 & 0x0F));
        }
    }
}

/**
 * Save a quantized matrix to a file
 */
int hyperionSaveQuantizedMatrix(const void *matrix, const char *path, HyperionPrecision precision) {
    if (!matrix || !path) {
        return -1;
    }
    
    HyperionFile *file = hyperionOpenFile(path, HYPERION_FILE_WRITE | HYPERION_FILE_BINARY | HYPERION_FILE_CREATE);
    if (!file) {
        return -1;
    }
    
    /* Write header with magic number, precision, and dimensions */
    uint32_t magic = 0x4D51544E; /* "TQNM" - Hyperion Quantized Matrix */
    hyperionWriteFile(file, &magic, sizeof(magic));
    hyperionWriteFile(file, &precision, sizeof(precision));
    
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            const HyperionMatrixFP32 *mat = (const HyperionMatrixFP32 *)matrix;
            hyperionWriteFile(file, &mat->rows, sizeof(mat->rows));
            hyperionWriteFile(file, &mat->cols, sizeof(mat->cols));
            
            /* Write the data */
            size_t dataSize = mat->rows * mat->cols * sizeof(float);
            hyperionWriteFile(file, mat->data, dataSize);
            break;
        }
        
        case HYPERION_PRECISION_INT8: {
            const HyperionMatrix8bit *mat = (const HyperionMatrix8bit *)matrix;
            hyperionWriteFile(file, &mat->rows, sizeof(mat->rows));
            hyperionWriteFile(file, &mat->cols, sizeof(mat->cols));
            hyperionWriteFile(file, &mat->scale, sizeof(mat->scale));
            hyperionWriteFile(file, &mat->zeroPoint, sizeof(mat->zeroPoint));
            
            /* Write the data */
            size_t dataSize = mat->rows * mat->cols;
            hyperionWriteFile(file, mat->data, dataSize);
            break;
        }
        
        case HYPERION_PRECISION_INT4: {
            const HyperionMatrix4bit *mat = (const HyperionMatrix4bit *)matrix;
            hyperionWriteFile(file, &mat->rows, sizeof(mat->rows));
            hyperionWriteFile(file, &mat->cols, sizeof(mat->cols));
            hyperionWriteFile(file, &mat->scale, sizeof(mat->scale));
            hyperionWriteFile(file, &mat->zeroPoint, sizeof(mat->zeroPoint));
            
            /* Write the data (4-bit packed, 2 values per byte) */
            size_t dataSize = (mat->rows * mat->cols + 1) / 2;
            hyperionWriteFile(file, mat->data, dataSize);
            break;
        }
        
        default:
            hyperionCloseFile(file);
            return -1;  /* Unknown precision */
    }
    
    hyperionCloseFile(file);
    return 0;
}

/**
 * Load a quantized matrix from a file
 */
void* hyperionLoadQuantizedMatrix(const char *path, HyperionPrecision precision) {
    if (!path) {
        return NULL;
    }
    
    HyperionFile *file = hyperionOpenFile(path, HYPERION_FILE_READ | HYPERION_FILE_BINARY);
    if (!file) {
        return NULL;
    }
    
    /* Read and verify header */
    uint32_t magic;
    HyperionPrecision filePrecision;
    
    if (hyperionReadFile(file, &magic, sizeof(magic)) != sizeof(magic) ||
        magic != 0x4D51544E) {
        hyperionCloseFile(file);
        return NULL;  /* Invalid magic number */
    }
    
    if (hyperionReadFile(file, &filePrecision, sizeof(filePrecision)) != sizeof(filePrecision) ||
        filePrecision != precision) {
        hyperionCloseFile(file);
        return NULL;  /* Precision mismatch */
    }
    
    /* Read data based on precision */
    void *result = NULL;
    
    switch (precision) {
        case HYPERION_PRECISION_FP32: {
            uint32_t rows, cols;
            
            if (hyperionReadFile(file, &rows, sizeof(rows)) != sizeof(rows) ||
                hyperionReadFile(file, &cols, sizeof(cols)) != sizeof(cols)) {
                hyperionCloseFile(file);
                return NULL;
            }
            
            HyperionMatrixFP32 *mat = hyperionCreateMatrixFP32(rows, cols);
            if (!mat) {
                hyperionCloseFile(file);
                return NULL;
            }
            
            /* Read the data */
            size_t dataSize = rows * cols * sizeof(float);
            if (hyperionReadFile(file, mat->data, dataSize) != dataSize) {
                hyperionDestroyMatrixFP32(mat);
                hyperionCloseFile(file);
                return NULL;
            }
            
            result = mat;
            break;
        }
        
        case HYPERION_PRECISION_INT8: {
            uint32_t rows, cols;
            float scale, zeroPoint;
            
            if (hyperionReadFile(file, &rows, sizeof(rows)) != sizeof(rows) ||
                hyperionReadFile(file, &cols, sizeof(cols)) != sizeof(cols) ||
                hyperionReadFile(file, &scale, sizeof(scale)) != sizeof(scale) ||
                hyperionReadFile(file, &zeroPoint, sizeof(zeroPoint)) != sizeof(zeroPoint)) {
                hyperionCloseFile(file);
                return NULL;
            }
            
            HyperionMatrix8bit *mat = hyperionCreateMatrix8bit(rows, cols);
            if (!mat) {
                hyperionCloseFile(file);
                return NULL;
            }
            
            mat->scale = scale;
            mat->zeroPoint = zeroPoint;
            
            /* Read the data */
            size_t dataSize = rows * cols;
            if (hyperionReadFile(file, mat->data, dataSize) != dataSize) {
                hyperionDestroyMatrix8bit(mat);
                hyperionCloseFile(file);
                return NULL;
            }
            
            result = mat;
            break;
        }
        
        case HYPERION_PRECISION_INT4: {
            uint32_t rows, cols;
            float scale, zeroPoint;
            
            if (hyperionReadFile(file, &rows, sizeof(rows)) != sizeof(rows) ||
                hyperionReadFile(file, &cols, sizeof(cols)) != sizeof(cols) ||
                hyperionReadFile(file, &scale, sizeof(scale)) != sizeof(scale) ||
                hyperionReadFile(file, &zeroPoint, sizeof(zeroPoint)) != sizeof(zeroPoint)) {
                hyperionCloseFile(file);
                return NULL;
            }
            
            HyperionMatrix4bit *mat = hyperionCreateMatrix4bit(rows, cols);
            if (!mat) {
                hyperionCloseFile(file);
                return NULL;
            }
            
            mat->scale = scale;
            mat->zeroPoint = zeroPoint;
            
            /* Read the data */
            size_t dataSize = (rows * cols + 1) / 2;
            if (hyperionReadFile(file, mat->data, dataSize) != dataSize) {
                hyperionDestroyMatrix4bit(mat);
                hyperionCloseFile(file);
                return NULL;
            }
            
            result = mat;
            break;
        }
        
        default:
            break;  /* Unknown precision */
    }
    
    hyperionCloseFile(file);
    return result;
}