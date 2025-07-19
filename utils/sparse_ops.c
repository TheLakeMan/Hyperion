/**
 * @file sparse_ops.c
 * @brief Implementation of sparse matrix operations for Hyperion
 */

#include "sparse_ops.h"
#include "memory.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Check if SIMD is available */
#if defined(__AVX__) || defined(__AVX2__)
#include <immintrin.h>
#define HYPERION_SIMD_AVX
#elif defined(__SSE__) || defined(__SSE2__) || defined(__SSE3__) || defined(__SSSE3__) ||          \
    defined(__SSE4_1__) || defined(__SSE4_2__)
#include <emmintrin.h>
#include <xmmintrin.h>
#define HYPERION_SIMD_SSE
#endif

/**
 * Create a CSR matrix from dense matrix data with a sparsity threshold
 */
HyperionCSRMatrix *hyperionCreateCSRMatrixFromDense(const float *dense, int32_t rows, int32_t cols,
                                                float threshold)
{
    if (!dense || rows <= 0 || cols <= 0) {
        return NULL;
    }

    /* First pass: count non-zero elements */
    int32_t nnz = 0;
    for (int32_t i = 0; i < rows; i++) {
        for (int32_t j = 0; j < cols; j++) {
            if (fabsf(dense[i * cols + j]) >= threshold) {
                nnz++;
            }
        }
    }

    /* Allocate CSR matrix */
    HyperionCSRMatrix *csr = (HyperionCSRMatrix *)malloc(sizeof(HyperionCSRMatrix));
    if (!csr) {
        return NULL;
    }

    csr->rows = rows;
    csr->cols = cols;
    csr->nnz  = nnz;

    /* Allocate arrays */
    csr->values     = (float *)malloc(nnz * sizeof(float));
    csr->colIndices = (int32_t *)malloc(nnz * sizeof(int32_t));
    csr->rowPtrs    = (int32_t *)malloc((rows + 1) * sizeof(int32_t));

    if (!csr->values || !csr->colIndices || !csr->rowPtrs) {
        hyperionCSRMatrixFree(csr);
        return NULL;
    }

    /* Second pass: fill in CSR matrix */
    csr->rowPtrs[0] = 0;
    int32_t idx     = 0;

    for (int32_t i = 0; i < rows; i++) {
        for (int32_t j = 0; j < cols; j++) {
            float val = dense[i * cols + j];
            if (fabsf(val) >= threshold) {
                csr->values[idx]     = val;
                csr->colIndices[idx] = j;
                idx++;
            }
        }
        csr->rowPtrs[i + 1] = idx;
    }

    return csr;
}

/**
 * Create a 4-bit quantized CSR matrix from dense matrix data with sparsity threshold
 */
HyperionCSRMatrix4Bit *hyperionCreateCSRMatrix4BitFromDense(const float *dense, int32_t rows,
                                                        int32_t cols, float threshold)
{
    if (!dense || rows <= 0 || cols <= 0) {
        return NULL;
    }

    /* First pass: count non-zero elements and find min/max values */
    int32_t nnz    = 0;
    float   minVal = FLT_MAX;
    float   maxVal = -FLT_MAX;

    for (int32_t i = 0; i < rows; i++) {
        for (int32_t j = 0; j < cols; j++) {
            float val = dense[i * cols + j];
            if (fabsf(val) >= threshold) {
                nnz++;
                if (val < minVal)
                    minVal = val;
                if (val > maxVal)
                    maxVal = val;
            }
        }
    }

    /* Allocate CSR matrix */
    HyperionCSRMatrix4Bit *csr = (HyperionCSRMatrix4Bit *)malloc(sizeof(HyperionCSRMatrix4Bit));
    if (!csr) {
        return NULL;
    }

    csr->rows = rows;
    csr->cols = cols;
    csr->nnz  = nnz;

    /* Calculate quantization parameters */
    float range    = maxVal - minVal;
    csr->scale     = range / 15.0f; /* 4-bit range is 0-15 */
    csr->zeroPoint = minVal;

    /* Allocate arrays */
    /* Each qvalue takes 4 bits, so we need nnz/2 bytes (rounded up) */
    size_t qvaluesSize = (nnz + 1) / 2;
    csr->qvalues       = (uint8_t *)malloc(qvaluesSize);
    csr->colIndices    = (int32_t *)malloc(nnz * sizeof(int32_t));
    csr->rowPtrs       = (int32_t *)malloc((rows + 1) * sizeof(int32_t));

    if (!csr->qvalues || !csr->colIndices || !csr->rowPtrs) {
        hyperionCSRMatrix4BitFree(csr);
        return NULL;
    }

    /* Initialize qvalues to zero */
    memset(csr->qvalues, 0, qvaluesSize);

    /* Second pass: fill in CSR matrix */
    csr->rowPtrs[0] = 0;
    int32_t idx     = 0;

    for (int32_t i = 0; i < rows; i++) {
        for (int32_t j = 0; j < cols; j++) {
            float val = dense[i * cols + j];
            if (fabsf(val) >= threshold) {
                /* Quantize to 4 bits */
                float   normalized = (val - csr->zeroPoint) / csr->scale;
                uint8_t qval       = (uint8_t)roundf(fmaxf(0.0f, fminf(normalized, 15.0f)));

                /* Pack two 4-bit values per byte */
                if (idx % 2 == 0) {
                    csr->qvalues[idx / 2] = qval;
                }
                else {
                    csr->qvalues[idx / 2] |= (qval << 4);
                }

                csr->colIndices[idx] = j;
                idx++;
            }
        }
        csr->rowPtrs[i + 1] = idx;
    }

    return csr;
}

/**
 * Convert a CSR matrix to dense format
 */
bool hyperionCSRMatrixToDense(const HyperionCSRMatrix *csr, float *dense)
{
    if (!csr || !dense) {
        return false;
    }

    /* Initialize dense matrix to zeros */
    memset(dense, 0, csr->rows * csr->cols * sizeof(float));

    /* Fill in non-zero values */
    for (int32_t i = 0; i < csr->rows; i++) {
        int32_t rowStart = csr->rowPtrs[i];
        int32_t rowEnd   = csr->rowPtrs[i + 1];

        for (int32_t j = rowStart; j < rowEnd; j++) {
            int32_t col                = csr->colIndices[j];
            dense[i * csr->cols + col] = csr->values[j];
        }
    }

    return true;
}

/**
 * Convert a 4-bit quantized CSR matrix to dense format
 */
bool hyperionCSRMatrix4BitToDense(const HyperionCSRMatrix4Bit *csr, float *dense)
{
    if (!csr || !dense) {
        return false;
    }

    /* Initialize dense matrix to zeros */
    memset(dense, 0, csr->rows * csr->cols * sizeof(float));

    /* Fill in non-zero values */
    for (int32_t i = 0; i < csr->rows; i++) {
        int32_t rowStart = csr->rowPtrs[i];
        int32_t rowEnd   = csr->rowPtrs[i + 1];

        for (int32_t j = rowStart; j < rowEnd; j++) {
            /* Extract 4-bit value */
            uint8_t qval;
            if (j % 2 == 0) {
                qval = csr->qvalues[j / 2] & 0x0F;
            }
            else {
                qval = (csr->qvalues[j / 2] >> 4) & 0x0F;
            }

            /* Dequantize */
            float val = qval * csr->scale + csr->zeroPoint;

            int32_t col                = csr->colIndices[j];
            dense[i * csr->cols + col] = val;
        }
    }

    return true;
}

/**
 * Free memory used by a CSR matrix
 */
void hyperionCSRMatrixFree(HyperionCSRMatrix *csr)
{
    if (!csr) {
        return;
    }

    if (csr->values) {
        free(csr->values);
    }

    if (csr->colIndices) {
        free(csr->colIndices);
    }

    if (csr->rowPtrs) {
        free(csr->rowPtrs);
    }

    free(csr);
}

/**
 * Free memory used by a 4-bit quantized CSR matrix
 */
void hyperionCSRMatrix4BitFree(HyperionCSRMatrix4Bit *csr)
{
    if (!csr) {
        return;
    }

    if (csr->qvalues) {
        free(csr->qvalues);
    }

    if (csr->colIndices) {
        free(csr->colIndices);
    }

    if (csr->rowPtrs) {
        free(csr->rowPtrs);
    }

    free(csr);
}

/**
 * Perform sparse matrix-vector multiplication: y = A * x
 */
bool hyperionCSRMatrixVectorMul(const HyperionCSRMatrix *csr, const float *x, float *y)
{
    if (!csr || !x || !y) {
        return false;
    }

    /* Initialize output vector to zero */
    memset(y, 0, csr->rows * sizeof(float));

    /* Perform CSR matrix-vector multiplication */
    for (int32_t i = 0; i < csr->rows; i++) {
        int32_t rowStart = csr->rowPtrs[i];
        int32_t rowEnd   = csr->rowPtrs[i + 1];

        for (int32_t j = rowStart; j < rowEnd; j++) {
            int32_t col = csr->colIndices[j];
            y[i] += csr->values[j] * x[col];
        }
    }

    return true;
}

/**
 * Perform 4-bit quantized sparse matrix-vector multiplication: y = A * x
 */
bool hyperionCSRMatrix4BitVectorMul(const HyperionCSRMatrix4Bit *csr, const float *x, float *y)
{
    if (!csr || !x || !y) {
        return false;
    }

    /* Initialize output vector to zero */
    memset(y, 0, csr->rows * sizeof(float));

    /* Perform CSR matrix-vector multiplication */
    for (int32_t i = 0; i < csr->rows; i++) {
        int32_t rowStart = csr->rowPtrs[i];
        int32_t rowEnd   = csr->rowPtrs[i + 1];

        for (int32_t j = rowStart; j < rowEnd; j++) {
            /* Extract 4-bit value */
            uint8_t qval;
            if (j % 2 == 0) {
                qval = csr->qvalues[j / 2] & 0x0F;
            }
            else {
                qval = (csr->qvalues[j / 2] >> 4) & 0x0F;
            }

            /* Dequantize */
            float val = qval * csr->scale + csr->zeroPoint;

            int32_t col = csr->colIndices[j];
            y[i] += val * x[col];
        }
    }

    return true;
}

#ifdef TINYAI_SIMD_AVX
/**
 * Perform SIMD-accelerated sparse matrix-vector multiplication: y = A * x (AVX version)
 */
bool hyperionCSRMatrixVectorMulSIMD(const HyperionCSRMatrix *csr, const float *x, float *y)

/**
 * Perform SIMD-accelerated 4-bit quantized sparse matrix-vector multiplication: y = A * x (AVX
 * version)
 */
bool hyperionCSRMatrix4BitVectorMulSIMD(const HyperionCSRMatrix4Bit *csr, const float *x, float *y)

#elif defined(TINYAI_SIMD_SSE)
/**
 * Perform SIMD-accelerated sparse matrix-vector multiplication: y = A * x (SSE version)
 */
bool hyperionCSRMatrixVectorMulSIMD(const HyperionCSRMatrix *csr, const float *x, float *y)

/**
 * Perform SIMD-accelerated 4-bit quantized sparse matrix-vector multiplication: y = A * x (SSE
 * version)
 */
bool hyperionCSRMatrix4BitVectorMulSIMD(const HyperionCSRMatrix4Bit *csr, const float *x, float *y)
#else
/* Non-SIMD fallback implementations */
bool hyperionCSRMatrixVectorMulSIMD(const HyperionCSRMatrix *csr, const float *x, float *y)
{
    return hyperionCSRMatrixVectorMul(csr, x, y);
}

bool hyperionCSRMatrix4BitVectorMulSIMD(const HyperionCSRMatrix4Bit *csr, const float *x, float *y)
{
    return hyperionCSRMatrix4BitVectorMul(csr, x, y);
}
#endif

/**
 * Calculate memory usage of CSR matrix in bytes
 */
size_t hyperionCSRMatrixMemoryUsage(const HyperionCSRMatrix *csr)
{
    if (!csr) {
        return 0;
    }

    size_t memoryUsage = 0;

    /* Size of the struct itself */
    memoryUsage += sizeof(HyperionCSRMatrix);

    /* Size of arrays */
    memoryUsage += csr->nnz * sizeof(float);          /* values */
    memoryUsage += csr->nnz * sizeof(int32_t);        /* colIndices */
    memoryUsage += (csr->rows + 1) * sizeof(int32_t); /* rowPtrs */

    return memoryUsage;
}

/**
 * Calculate memory usage of 4-bit quantized CSR matrix in bytes
 */
size_t hyperionCSRMatrix4BitMemoryUsage(const HyperionCSRMatrix4Bit *csr)
{
    if (!csr) {
        return 0;
    }

    size_t memoryUsage = 0;

    /* Size of the struct itself */
    memoryUsage += sizeof(HyperionCSRMatrix4Bit);

    /* Size of arrays */
    memoryUsage += (csr->nnz + 1) / 2;                /* qvalues (4-bit packed) */
    memoryUsage += csr->nnz * sizeof(int32_t);        /* colIndices */
    memoryUsage += (csr->rows + 1) * sizeof(int32_t); /* rowPtrs */

    return memoryUsage;
}

/**
 * Calculate compression ratio compared to dense matrix storage
 */
float hyperionCSRMatrixCompressionRatio(const HyperionCSRMatrix *csr)
{
    if (!csr) {
        return 0.0f;
    }

    /* Size of dense matrix in bytes */
    size_t denseSize = csr->rows * csr->cols * sizeof(float);

    /* Size of CSR matrix in bytes */
    size_t sparseSize = hyperionCSRMatrixMemoryUsage(csr);

    return (float)denseSize / (float)sparseSize;
}

/**
 * Calculate compression ratio for 4-bit quantized CSR matrix compared to dense matrix storage
 */
float hyperionCSRMatrix4BitCompressionRatio(const HyperionCSRMatrix4Bit *csr)
{
    if (!csr) {
        return 0.0f;
    }

    /* Size of dense matrix in bytes */
    size_t denseSize = csr->rows * csr->cols * sizeof(float);

    /* Size of 4-bit quantized CSR matrix in bytes */
    size_t sparseSize = hyperionCSRMatrix4BitMemoryUsage(csr);

    return (float)denseSize / (float)sparseSize;
}
