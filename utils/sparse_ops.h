/**
 * @file sparse_ops.h
 * @brief Sparse matrix operations for Hyperion
 *
 * This header defines operations for efficient storage and computation
 * with sparse matrices, enabling significant memory savings for large
 * models with many near-zero weights.
 */

#ifndef HYPERION_SPARSE_OPS_H
#define HYPERION_SPARSE_OPS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Compressed Sparse Row (CSR) format for sparse matrices
 */
typedef struct {
    float   *values;     /* Non-zero values */
    int32_t *colIndices; /* Column indices for each non-zero value */
    int32_t *rowPtrs;    /* Pointers to start of each row in values and colIndices */
    int32_t  rows;       /* Number of rows */
    int32_t  cols;       /* Number of columns */
    int32_t  nnz;        /* Number of non-zero elements */
} HyperionCSRMatrix;

/**
 * Compressed Sparse Column (CSC) format for sparse matrices
 */
typedef struct {
    float   *values;     /* Non-zero values */
    int32_t *rowIndices; /* Row indices for each non-zero value */
    int32_t *colPtrs;    /* Pointers to start of each column in values and rowIndices */
    int32_t  rows;       /* Number of rows */
    int32_t  cols;       /* Number of columns */
    int32_t  nnz;        /* Number of non-zero elements */
} HyperionCSCMatrix;

/**
 * Coordinate (COO) format for sparse matrices
 */
typedef struct {
    float   *values;     /* Non-zero values */
    int32_t *rowIndices; /* Row indices for each non-zero value */
    int32_t *colIndices; /* Column indices for each non-zero value */
    int32_t  rows;       /* Number of rows */
    int32_t  cols;       /* Number of columns */
    int32_t  nnz;        /* Number of non-zero elements */
} HyperionCOOMatrix;

/**
 * 4-bit quantized CSR matrix format
 */
typedef struct {
    uint8_t *qvalues;    /* Quantized non-zero values (4-bit packed) */
    int32_t *colIndices; /* Column indices for each non-zero value */
    int32_t *rowPtrs;    /* Pointers to start of each row in values and colIndices */
    float    scale;      /* Scale factor for quantization */
    float    zeroPoint;  /* Zero point for quantization */
    int32_t  rows;       /* Number of rows */
    int32_t  cols;       /* Number of columns */
    int32_t  nnz;        /* Number of non-zero elements */
} HyperionCSRMatrix4Bit;

/**
 * Create a CSR matrix from dense matrix data with a sparsity threshold
 *
 * @param dense Dense matrix data (row-major)
 * @param rows Number of rows
 * @param cols Number of columns
 * @param threshold Values with absolute magnitude below this threshold are treated as zero
 * @return CSR matrix (must be freed with tinyaiCSRMatrixFree)
 */
HyperionCSRMatrix *hyperionCreateCSRMatrixFromDense(const float *dense, int32_t rows, int32_t cols,
                                                float threshold);

/**
 * Create a 4-bit quantized CSR matrix from dense matrix data with sparsity threshold
 *
 * @param dense Dense matrix data (row-major)
 * @param rows Number of rows
 * @param cols Number of columns
 * @param threshold Values with absolute magnitude below this threshold are treated as zero
 * @return 4-bit quantized CSR matrix (must be freed with tinyaiCSRMatrix4BitFree)
 */
HyperionCSRMatrix4Bit *hyperionCreateCSRMatrix4BitFromDense(const float *dense, int32_t rows,
                                                        int32_t cols, float threshold);

/**
 * Convert a CSR matrix to dense format
 *
 * @param csr CSR matrix to convert
 * @param dense Output dense matrix (row-major, must be pre-allocated with rows*cols elements)
 * @return true on success, false on failure
 */
bool hyperionCSRMatrixToDense(const HyperionCSRMatrix *csr, float *dense);

/**
 * Convert a 4-bit quantized CSR matrix to dense format
 *
 * @param csr 4-bit quantized CSR matrix to convert
 * @param dense Output dense matrix (row-major, must be pre-allocated with rows*cols elements)
 * @return true on success, false on failure
 */
bool hyperionCSRMatrix4BitToDense(const HyperionCSRMatrix4Bit *csr, float *dense);

/**
 * Free memory used by a CSR matrix
 *
 * @param csr CSR matrix to free
 */
void hyperionCSRMatrixFree(HyperionCSRMatrix *csr);

/**
 * Free memory used by a 4-bit quantized CSR matrix
 *
 * @param csr 4-bit quantized CSR matrix to free
 */
void hyperionCSRMatrix4BitFree(HyperionCSRMatrix4Bit *csr);

/**
 * Perform sparse matrix-vector multiplication: y = A * x
 *
 * @param csr CSR matrix A
 * @param x Input vector x
 * @param y Output vector y (must be pre-allocated with csr->rows elements)
 * @return true on success, false on failure
 */
bool hyperionCSRMatrixVectorMul(const HyperionCSRMatrix *csr, const float *x, float *y);

/**
 * Perform 4-bit quantized sparse matrix-vector multiplication: y = A * x
 *
 * @param csr 4-bit quantized CSR matrix A
 * @param x Input vector x
 * @param y Output vector y (must be pre-allocated with csr->rows elements)
 * @return true on success, false on failure
 */
bool hyperionCSRMatrix4BitVectorMul(const HyperionCSRMatrix4Bit *csr, const float *x, float *y);

/**
 * Perform SIMD-accelerated sparse matrix-vector multiplication: y = A * x
 *
 * @param csr CSR matrix A
 * @param x Input vector x
 * @param y Output vector y (must be pre-allocated with csr->rows elements)
 * @return true on success, false on failure
 */
bool hyperionCSRMatrixVectorMulSIMD(const HyperionCSRMatrix *csr, const float *x, float *y);

/**
 * Perform SIMD-accelerated 4-bit quantized sparse matrix-vector multiplication: y = A * x
 *
 * @param csr 4-bit quantized CSR matrix A
 * @param x Input vector x
 * @param y Output vector y (must be pre-allocated with csr->rows elements)
 * @return true on success, false on failure
 */
bool hyperionCSRMatrix4BitVectorMulSIMD(const HyperionCSRMatrix4Bit *csr, const float *x, float *y);

/**
 * Calculate memory usage of CSR matrix in bytes
 *
 * @param csr CSR matrix
 * @return Memory usage in bytes
 */
size_t hyperionCSRMatrixMemoryUsage(const HyperionCSRMatrix *csr);

/**
 * Calculate memory usage of 4-bit quantized CSR matrix in bytes
 *
 * @param csr 4-bit quantized CSR matrix
 * @return Memory usage in bytes
 */
size_t hyperionCSRMatrix4BitMemoryUsage(const HyperionCSRMatrix4Bit *csr);

/**
 * Calculate compression ratio compared to dense matrix storage
 *
 * @param csr CSR matrix
 * @return Compression ratio (dense size / sparse size)
 */
float hyperionCSRMatrixCompressionRatio(const HyperionCSRMatrix *csr);

/**
 * Calculate compression ratio for 4-bit quantized CSR matrix compared to dense matrix storage
 *
 * @param csr 4-bit quantized CSR matrix
 * @return Compression ratio (dense size / sparse size)
 */
float hyperionCSRMatrix4BitCompressionRatio(const HyperionCSRMatrix4Bit *csr);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_SPARSE_OPS_H */
