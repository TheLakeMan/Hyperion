/**
 * @file quantize_mixed.h
 * @brief Mixed precision quantization utilities for Hyperion
 *
 * This header provides utilities for mixed precision quantization,
 * allowing different parts of a model to use different bit-widths
 * based on their sensitivity and importance.
 */

#ifndef HYPERION_QUANTIZE_MIXED_H
#define HYPERION_QUANTIZE_MIXED_H

#include "../models/image/image_model.h"
#include "quantize.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Supported quantization precisions
 */
typedef enum {
    HYPERION_MIXED_PREC_FP32, /* Full precision floating point (32-bit) */
    HYPERION_MIXED_PREC_FP16, /* Half precision floating point (16-bit) */
    HYPERION_MIXED_PREC_INT8, /* 8-bit integer quantization */
    HYPERION_MIXED_PREC_INT4, /* 4-bit integer quantization */
    HYPERION_MIXED_PREC_INT2  /* 2-bit integer quantization */
} HyperionMixedPrecType;

/**
 * Mixed precision quantization configuration for a layer
 */
typedef struct {
    HyperionMixedPrecType weightPrecision; /* Precision for weights */
    HyperionMixedPrecType biasPrecision;   /* Precision for biases */
    HyperionMixedPrecType activPrecision;  /* Precision for activations */
    float               weightThreshold; /* Per-layer weight clipping threshold */
    float               biasThreshold;   /* Per-layer bias clipping threshold */
    float               activThreshold;  /* Per-layer activation clipping threshold */
} HyperionLayerQuantConfig;

/**
 * Matrix with mixed precision elements
 */
typedef struct {
    void               *data;      /* Pointer to matrix data */
    size_t              dataSize;  /* Size of data in bytes */
    int                 rows;      /* Number of rows */
    int                 cols;      /* Number of columns */
    HyperionMixedPrecType precision; /* Precision of the data */
    float               scale;     /* Scale factor for quantization */
    float               zeroPoint; /* Zero point for quantization */
} HyperionMixedPrecMatrix;

/**
 * Mixed precision model configuration
 */
typedef struct {
    int                     numLayers;          /* Number of layers in the model */
    HyperionLayerQuantConfig *layerConfigs;       /* Per-layer quantization configs */
    bool                    perChannelQuantize; /* Whether to use per-channel quantization */
    bool                    useSymmetric;       /* Whether to use symmetric quantization */
    int                     calibrationSize;    /* Calibration dataset size */
    float                  *calibrationData;    /* Representative data for calibration */
} HyperionMixedPrecConfig;

/**
 * Create a mixed precision matrix from floating point data
 *
 * @param data Source floating point data
 * @param rows Number of rows
 * @param cols Number of columns
 * @param precision Target precision for the matrix
 * @param threshold Clipping threshold for quantization (0.0 for auto)
 * @return Quantized mixed precision matrix (NULL on failure)
 */
HyperionMixedPrecMatrix *hyperionCreateMixedPrecMatrix(const float *data, int rows, int cols,
                                                   HyperionMixedPrecType precision, float threshold);

/**
 * Free a mixed precision matrix
 *
 * @param matrix Matrix to free
 */
void hyperionFreeMixedPrecMatrix(HyperionMixedPrecMatrix *matrix);

/**
 * Convert a mixed precision matrix to floating point
 *
 * @param matrix Mixed precision matrix to convert
 * @param output Output floating point array (must be pre-allocated)
 * @return true on success, false on failure
 */
bool hyperionMixedPrecToFloat(const HyperionMixedPrecMatrix *matrix, float *output);

/**
 * Determine optimal precision for each layer using sensitivity analysis
 *
 * @param modelPath Path to original model file
 * @param calibrationData Representative input data for calibration
 * @param calibrationSize Number of calibration samples
 * @param config Output quantization configuration
 * @return true on success, false on failure
 */
bool hyperionDetermineOptimalPrecision(const char *modelPath, const float *calibrationData,
                                     int calibrationSize, HyperionMixedPrecConfig *config);

/**
 * Apply mixed precision quantization to a model
 *
 * @param srcModelPath Path to source model file
 * @param dstModelPath Path to save quantized model
 * @param config Mixed precision configuration
 * @return true on success, false on failure
 */
bool hyperionQuantizeModelMixedPrecision(const char *srcModelPath, const char *dstModelPath,
                                       const HyperionMixedPrecConfig *config);

/**
 * Matrix multiplication with mixed precision matrices
 *
 * @param a First matrix
 * @param b Second matrix
 * @param output Output matrix (must be pre-allocated)
 * @return true on success, false on failure
 */
bool hyperionMixedPrecMatMul(const HyperionMixedPrecMatrix *a, const HyperionMixedPrecMatrix *b,
                           HyperionMixedPrecMatrix *output);

/**
 * Free a mixed precision model configuration
 *
 * @param config Configuration to free
 */
void hyperionFreeMixedPrecConfig(HyperionMixedPrecConfig *config);

/**
 * Get the size in bits for a given precision type
 *
 * @param precision Precision type
 * @return Size in bits (0 if invalid)
 */
int hyperionGetPrecisionBits(HyperionMixedPrecType precision);

/**
 * Calculate memory usage for a mixed precision matrix
 *
 * @param matrix Mixed precision matrix
 * @return Memory usage in bytes
 */
size_t hyperionGetMixedPrecMatrixMemoryUsage(const HyperionMixedPrecMatrix *matrix);

/**
 * Create a default mixed precision configuration
 *
 * @param numLayers Number of layers in the model
 * @return Default configuration (NULL on failure)
 */
HyperionMixedPrecConfig *hyperionCreateDefaultMixedPrecConfig(int numLayers);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HYPERION_QUANTIZE_MIXED_H */