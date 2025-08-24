/**
 * @file advanced_quantization.h
 * @brief Advanced quantization techniques for Hyperion beyond 4-bit quantization
 *
 * This module provides sophisticated quantization strategies including mixed precision,
 * dynamic quantization, adaptive bit-width selection, and quantization-aware training
 * support for ultra-efficient neural network inference.
 */

#ifndef HYPERION_ADVANCED_QUANTIZATION_H
#define HYPERION_ADVANCED_QUANTIZATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Quantization bit-width options
 */
typedef enum {
    HYPERION_QUANT_1BIT,    /* Binary quantization */
    HYPERION_QUANT_2BIT,    /* 2-bit quantization */
    HYPERION_QUANT_3BIT,    /* 3-bit quantization */
    HYPERION_QUANT_4BIT,    /* 4-bit quantization (existing) */
    HYPERION_QUANT_8BIT,    /* 8-bit quantization */
    HYPERION_QUANT_16BIT,   /* 16-bit quantization */
    HYPERION_QUANT_MIXED,   /* Mixed precision */
    HYPERION_QUANT_DYNAMIC  /* Dynamic quantization */
} HyperionQuantBitWidth;

/**
 * Quantization methods
 */
typedef enum {
    HYPERION_QUANT_LINEAR,      /* Linear/uniform quantization */
    HYPERION_QUANT_LOG,         /* Logarithmic quantization */
    HYPERION_QUANT_ASYMMETRIC,  /* Asymmetric quantization */
    HYPERION_QUANT_SYMMETRIC,   /* Symmetric quantization */
    HYPERION_QUANT_ADAPTIVE,    /* Adaptive quantization */
    HYPERION_QUANT_LEARNED      /* Learned quantization parameters */
} HyperionQuantMethod;

/**
 * Mixed precision configuration
 */
typedef struct {
    HyperionQuantBitWidth *layerBitWidths;  /* Bit-width for each layer */
    int numLayers;                          /* Number of layers */
    float *sensitivityScores;               /* Sensitivity of each layer to quantization */
    float memoryBudget;                     /* Memory budget constraint (0.0-1.0) */
    float accuracyThreshold;                /* Minimum accuracy threshold */
    bool autoAssign;                        /* Auto-assign bit-widths based on sensitivity */
} MixedPrecisionConfig;

/**
 * Dynamic quantization configuration
 */
typedef struct {
    float activationThreshold;    /* Threshold for activation quantization */
    int calibrationSamples;      /* Number of samples for calibration */
    bool adaptToInput;           /* Adapt quantization to input statistics */
    bool useRunningStats;        /* Use running statistics for quantization */
    float momentumFactor;        /* Momentum for running statistics */
} DynamicQuantConfig;

/**
 * Quantization statistics
 */
typedef struct {
    float min, max;              /* Min/max values */
    float mean, variance;        /* Mean and variance */
    float *histogram;            /* Value histogram */
    int histogramBins;           /* Number of histogram bins */
    float percentile_1, percentile_99; /* 1st and 99th percentiles */
} QuantStats;

/**
 * Advanced quantization context
 */
typedef struct AdvancedQuantization AdvancedQuantization;

/**
 * Quantization configuration
 */
typedef struct {
    HyperionQuantMethod method;          /* Quantization method */
    HyperionQuantBitWidth defaultBitWidth; /* Default bit-width */
    MixedPrecisionConfig mixedPrecision; /* Mixed precision settings */
    DynamicQuantConfig dynamicQuant;     /* Dynamic quantization settings */
    bool useCalibration;                 /* Use calibration data */
    bool useSIMD;                       /* Use SIMD acceleration */
    float compressionRatio;             /* Target compression ratio */
} AdvancedQuantConfig;

/**
 * Create advanced quantization context
 * @param config Quantization configuration
 * @return Newly created quantization context, or NULL on failure
 */
AdvancedQuantization *hyperionAdvancedQuantCreate(const AdvancedQuantConfig *config);

/**
 * Free advanced quantization context
 * @param quant Quantization context to free
 */
void hyperionAdvancedQuantFree(AdvancedQuantization *quant);

/**
 * Mixed precision quantization
 * 
 * Quantizes weights using different bit-widths for different layers
 * 
 * @param quant Quantization context
 * @param weights Array of weight matrices for each layer
 * @param weightSizes Size of each weight matrix
 * @param numLayers Number of layers
 * @param quantizedWeights Output quantized weights
 * @param quantParams Output quantization parameters
 * @return true on success, false on failure
 */
bool hyperionMixedPrecisionQuantize(AdvancedQuantization *quant,
                                  const float **weights, const size_t *weightSizes,
                                  int numLayers, void **quantizedWeights,
                                  void **quantParams);

/**
 * Dynamic activation quantization
 * 
 * Quantizes activations dynamically based on runtime statistics
 * 
 * @param quant Quantization context
 * @param activations Input activations
 * @param size Size of activation tensor
 * @param bitWidth Target bit-width
 * @param quantizedActivations Output quantized activations
 * @param scale Output quantization scale
 * @param zeroPoint Output zero point
 * @return true on success, false on failure
 */
bool hyperionDynamicQuantizeActivations(AdvancedQuantization *quant,
                                       const float *activations, size_t size,
                                       HyperionQuantBitWidth bitWidth,
                                       void *quantizedActivations,
                                       float *scale, int *zeroPoint);

/**
 * Adaptive bit-width selection
 * 
 * Automatically selects optimal bit-widths for layers based on sensitivity analysis
 * 
 * @param quant Quantization context
 * @param weights Array of weight matrices
 * @param weightSizes Sizes of weight matrices
 * @param numLayers Number of layers
 * @param calibrationData Calibration dataset
 * @param numSamples Number of calibration samples
 * @param sampleSize Size of each calibration sample
 * @param optimalBitWidths Output optimal bit-widths for each layer
 * @return true on success, false on failure
 */
bool hyperionAdaptiveBitWidthSelection(AdvancedQuantization *quant,
                                     const float **weights, const size_t *weightSizes,
                                     int numLayers, const float **calibrationData,
                                     int numSamples, size_t sampleSize,
                                     HyperionQuantBitWidth *optimalBitWidths);

/**
 * Logarithmic quantization
 * 
 * Uses logarithmic scale for quantization, suitable for weights with wide dynamic range
 * 
 * @param input Input floating-point values
 * @param size Number of values
 * @param bitWidth Target bit-width
 * @param quantized Output quantized values
 * @param scale Output quantization scale
 * @return true on success, false on failure
 */
bool hyperionLogQuantize(const float *input, size_t size,
                        HyperionQuantBitWidth bitWidth,
                        void *quantized, float *scale);

/**
 * Asymmetric quantization
 * 
 * Uses asymmetric quantization with separate min/max values
 * 
 * @param input Input floating-point values
 * @param size Number of values
 * @param bitWidth Target bit-width
 * @param quantized Output quantized values
 * @param scale Output quantization scale
 * @param zeroPoint Output zero point
 * @return true on success, false on failure
 */
bool hyperionAsymmetricQuantize(const float *input, size_t size,
                               HyperionQuantBitWidth bitWidth,
                               void *quantized, float *scale, int *zeroPoint);

/**
 * Binary quantization (1-bit)
 * 
 * Quantizes weights to +1/-1 values
 * 
 * @param input Input floating-point weights
 * @param size Number of weights
 * @param quantized Output binary weights (packed)
 * @param scale Output scaling factor
 * @return true on success, false on failure
 */
bool hyperionBinaryQuantize(const float *input, size_t size,
                           uint8_t *quantized, float *scale);

/**
 * Ternary quantization (2-bit effective)
 * 
 * Quantizes weights to {-1, 0, +1} values
 * 
 * @param input Input floating-point weights
 * @param size Number of weights
 * @param threshold Threshold for zero quantization
 * @param quantized Output ternary weights
 * @param scale Output scaling factor
 * @return true on success, false on failure
 */
bool hyperionTernaryQuantize(const float *input, size_t size, float threshold,
                            int8_t *quantized, float *scale);

/**
 * Compute quantization statistics
 * 
 * @param input Input floating-point values
 * @param size Number of values
 * @param stats Output statistics structure
 * @return true on success, false on failure
 */
bool hyperionComputeQuantStats(const float *input, size_t size, QuantStats *stats);

/**
 * Calibration-based quantization parameter estimation
 * 
 * @param quant Quantization context
 * @param calibrationData Calibration dataset
 * @param numSamples Number of calibration samples
 * @param sampleSize Size of each sample
 * @param bitWidth Target bit-width
 * @param optimalScale Output optimal scale
 * @param optimalZeroPoint Output optimal zero point
 * @return true on success, false on failure
 */
bool hyperionCalibrateQuantization(AdvancedQuantization *quant,
                                 const float **calibrationData, int numSamples,
                                 size_t sampleSize, HyperionQuantBitWidth bitWidth,
                                 float *optimalScale, int *optimalZeroPoint);

/**
 * Quantization-aware training support
 * 
 * Simulates quantization during training for better accuracy
 * 
 * @param input Input floating-point values
 * @param size Number of values
 * @param bitWidth Target bit-width
 * @param scale Quantization scale
 * @param zeroPoint Zero point
 * @param fakeQuantized Output fake-quantized values (still float)
 * @return true on success, false on failure
 */
bool hyperionFakeQuantize(const float *input, size_t size,
                         HyperionQuantBitWidth bitWidth,
                         float scale, int zeroPoint, float *fakeQuantized);

/**
 * Dequantize values back to floating-point
 * 
 * @param quantized Quantized input values
 * @param size Number of values
 * @param bitWidth Bit-width used for quantization
 * @param scale Quantization scale
 * @param zeroPoint Zero point
 * @param output Output floating-point values
 * @return true on success, false on failure
 */
bool hyperionDequantize(const void *quantized, size_t size,
                       HyperionQuantBitWidth bitWidth,
                       float scale, int zeroPoint, float *output);

/**
 * Quantized matrix multiplication
 * 
 * Performs matrix multiplication on quantized matrices
 * 
 * @param quantA Quantized matrix A
 * @param quantB Quantized matrix B
 * @param M, N, K Matrix dimensions (A: MxK, B: KxN, C: MxN)
 * @param bitWidthA Bit-width of matrix A
 * @param bitWidthB Bit-width of matrix B
 * @param scaleA, scaleBQuantization scales
 * @param zeroPointA, zeroPointB Zero points
 * @param quantC Output quantized matrix C
 * @param scaleC Output scale for matrix C
 * @param zeroPointC Output zero point for matrix C
 * @return true on success, false on failure
 */
bool hyperionQuantizedMatMul(const void *quantA, const void *quantB,
                            int M, int N, int K,
                            HyperionQuantBitWidth bitWidthA, HyperionQuantBitWidth bitWidthB,
                            float scaleA, float scaleB,
                            int zeroPointA, int zeroPointB,
                            void *quantC, float *scaleC, int *zeroPointC);

/**
 * Get memory savings from quantization
 * 
 * @param quant Quantization context
 * @param originalSize Original model size in bytes
 * @param quantizedSize Output quantized model size in bytes
 * @param compressionRatio Output compression ratio
 * @return true on success, false on failure
 */
bool hyperionQuantGetMemorySavings(const AdvancedQuantization *quant,
                                  size_t originalSize, size_t *quantizedSize,
                                  float *compressionRatio);

/**
 * Enable/disable SIMD acceleration for quantization
 * 
 * @param quant Quantization context
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionQuantEnableSIMD(AdvancedQuantization *quant, bool enable);

/**
 * Benchmark quantization performance
 * 
 * @param quant Quantization context
 * @param dataSize Size of test data
 * @param numIterations Number of benchmark iterations
 * @param avgTimeMs Output average time in milliseconds
 * @param throughputMBps Output throughput in MB/s
 * @return true on success, false on failure
 */
bool hyperionQuantBenchmark(AdvancedQuantization *quant, size_t dataSize,
                           int numIterations, float *avgTimeMs, float *throughputMBps);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_ADVANCED_QUANTIZATION_H */