#include "advanced_quantization.h"
#include "../core/memory.h"
#include "simd_ops.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <stdint.h>

/**
 * Advanced quantization context structure
 */
struct AdvancedQuantization {
    AdvancedQuantConfig config;
    
    /* Statistics for calibration */
    QuantStats *layerStats;
    int numLayers;
    
    /* Running statistics for dynamic quantization */
    float *runningMeans;
    float *runningVars;
    int *sampleCounts;
    
    /* Mixed precision optimization */
    float *sensitivityScores;
    HyperionQuantBitWidth *optimalBitWidths;
    
    /* Memory usage tracking */
    size_t originalMemory;
    size_t quantizedMemory;
    
    bool initialized;
};

typedef struct {
    float scale;
    int zeroPoint;
    HyperionQuantBitWidth bitWidth;
    float logOffset;
} LayerQuantParams;

static size_t computeQuantizedBufferSize(size_t elementCount, HyperionQuantBitWidth bitWidth) {
    switch (bitWidth) {
        case HYPERION_QUANT_16BIT:
            return elementCount * sizeof(uint16_t);
        case HYPERION_QUANT_4BIT:
            return (elementCount + 1) / 2;
        default:
            return elementCount;
    }
}

static float g_lastLogOffset = 0.0f;

/* Helper function to get number of quantization levels for bit-width */
static int getQuantLevels(HyperionQuantBitWidth bitWidth) {
    switch (bitWidth) {
        case HYPERION_QUANT_1BIT: return 2;
        case HYPERION_QUANT_2BIT: return 4;
        case HYPERION_QUANT_3BIT: return 8;
        case HYPERION_QUANT_4BIT: return 16;
        case HYPERION_QUANT_8BIT: return 256;
        case HYPERION_QUANT_16BIT: return 65536;
        default: return 256; /* Default to 8-bit */
    }
}

/* Helper function to get bytes per element for bit-width */
static size_t getBytesPerElement(HyperionQuantBitWidth bitWidth) {
    switch (bitWidth) {
        case HYPERION_QUANT_1BIT: return 1; /* Packed */
        case HYPERION_QUANT_2BIT: return 1; /* Packed */
        case HYPERION_QUANT_3BIT: return 1; /* Packed */
        case HYPERION_QUANT_4BIT: return 1; /* Packed */
        case HYPERION_QUANT_8BIT: return 1;
        case HYPERION_QUANT_16BIT: return 2;
        default: return 1;
    }
}

/* Comparison function for qsort */
static int compareFloats(const void *a, const void *b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

/* Compute min/max values with percentile clipping */
static void computeClippedMinMax(const float *input, size_t size,
                               float *minVal, float *maxVal, float percentile) {
    if (!input || size == 0) return;
    
    /* Create sorted copy for percentile computation */
    float *sorted = (float*)hyperionAlloc(size * sizeof(float));
    memcpy(sorted, input, size * sizeof(float));
    
    /* Simple bubble sort for small arrays, qsort for larger */
    if (size < 1000) {
        for (size_t i = 0; i < size - 1; i++) {
            for (size_t j = 0; j < size - i - 1; j++) {
                if (sorted[j] > sorted[j + 1]) {
                    float temp = sorted[j];
                    sorted[j] = sorted[j + 1];
                    sorted[j + 1] = temp;
                }
            }
        }
    } else {
        /* Use qsort for larger arrays */
        qsort(sorted, size, sizeof(float), compareFloats);
    }
    
    /* Compute percentile indices */
    size_t lowIdx = (size_t)(percentile * size / 100.0f);
    size_t highIdx = (size_t)((100.0f - percentile) * size / 100.0f);
    
    if (lowIdx >= size) lowIdx = 0;
    if (highIdx >= size) highIdx = size - 1;
    
    *minVal = sorted[lowIdx];
    *maxVal = sorted[highIdx];
    
    hyperionFree(sorted);
}

AdvancedQuantization *hyperionAdvancedQuantCreate(const AdvancedQuantConfig *config) {
    if (!config) return NULL;
    
    AdvancedQuantization *quant = (AdvancedQuantization*)hyperionCalloc(1, sizeof(AdvancedQuantization));
    if (!quant) return NULL;
    
    /* Copy configuration */
    quant->config = *config;
    
    /* Initialize statistics arrays if mixed precision is used */
    if (config->mixedPrecision.numLayers > 0) {
        quant->numLayers = config->mixedPrecision.numLayers;
        
        quant->layerStats = (QuantStats*)hyperionCalloc(quant->numLayers, sizeof(QuantStats));
        quant->sensitivityScores = (float*)hyperionCalloc(quant->numLayers, sizeof(float));
        quant->optimalBitWidths = (HyperionQuantBitWidth*)hyperionCalloc(quant->numLayers, sizeof(HyperionQuantBitWidth));
        
        if (!quant->layerStats || !quant->sensitivityScores || !quant->optimalBitWidths) {
            hyperionAdvancedQuantFree(quant);
            return NULL;
        }
        
        /* Initialize with default bit-widths */
        for (int i = 0; i < quant->numLayers; i++) {
            quant->optimalBitWidths[i] = config->defaultBitWidth;
        }
    }
    
    /* Initialize dynamic quantization statistics */
    if (config->dynamicQuant.useRunningStats && quant->numLayers > 0) {
        quant->runningMeans = (float*)hyperionCalloc(quant->numLayers, sizeof(float));
        quant->runningVars = (float*)hyperionCalloc(quant->numLayers, sizeof(float));
        quant->sampleCounts = (int*)hyperionCalloc(quant->numLayers, sizeof(int));
        
        if (!quant->runningMeans || !quant->runningVars || !quant->sampleCounts) {
            hyperionAdvancedQuantFree(quant);
            return NULL;
        }
    }
    
    quant->initialized = true;
    return quant;
}

void hyperionAdvancedQuantFree(AdvancedQuantization *quant) {
    if (!quant) return;
    
    /* Free statistics */
    if (quant->layerStats) {
        for (int i = 0; i < quant->numLayers; i++) {
            hyperionFree(quant->layerStats[i].histogram);
        }
        hyperionFree(quant->layerStats);
    }
    
    hyperionFree(quant->sensitivityScores);
    hyperionFree(quant->optimalBitWidths);
    hyperionFree(quant->runningMeans);
    hyperionFree(quant->runningVars);
    hyperionFree(quant->sampleCounts);
    
    hyperionFree(quant);
}

bool hyperionComputeQuantStats(const float *input, size_t size, QuantStats *stats) {
    if (!input || size == 0 || !stats) return false;
    
    /* Compute basic statistics */
    float sum = 0.0f, sumSq = 0.0f;
    float minVal = input[0], maxVal = input[0];
    
    for (size_t i = 0; i < size; i++) {
        float val = input[i];
        sum += val;
        sumSq += val * val;
        
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    stats->min = minVal;
    stats->max = maxVal;
    stats->mean = sum / size;
    stats->variance = (sumSq / size) - (stats->mean * stats->mean);
    
    /* Compute percentiles */
    computeClippedMinMax(input, size, &stats->percentile_1, &stats->percentile_99, 1.0f);
    
    /* Compute histogram if requested */
    if (stats->histogramBins > 0) {
        if (!stats->histogram) {
            stats->histogram = (float*)hyperionCalloc(stats->histogramBins, sizeof(float));
        }
        
        if (stats->histogram) {
            float range = maxVal - minVal;
            if (range > 0) {
                for (size_t i = 0; i < size; i++) {
                    int bin = (int)((input[i] - minVal) / range * (stats->histogramBins - 1));
                    if (bin >= 0 && bin < stats->histogramBins) {
                        stats->histogram[bin] += 1.0f;
                    }
                }
                
                /* Normalize histogram */
                for (int i = 0; i < stats->histogramBins; i++) {
                    stats->histogram[i] /= size;
                }
            }
        }
    }
    
    return true;
}

bool hyperionAsymmetricQuantize(const float *input, size_t size,
                               HyperionQuantBitWidth bitWidth,
                               void *quantized, float *scale, int *zeroPoint) {
    if (!input || !quantized || !scale || !zeroPoint || size == 0) return false;
    
    /* Compute min/max with outlier clipping */
    float minVal, maxVal;
    computeClippedMinMax(input, size, &minVal, &maxVal, 1.0f);
    
    int quantLevels = getQuantLevels(bitWidth);
    float range = maxVal - minVal;
    
    if (range <= 0) {
        *scale = 1.0f;
        *zeroPoint = 0;
        memset(quantized, 0, size * getBytesPerElement(bitWidth));
        return true;
    }
    
    /* Compute scale and zero point */
    *scale = range / (quantLevels - 1);
    *zeroPoint = (int)roundf(-minVal / *scale);
    
    /* Clamp zero point to valid range */
    if (*zeroPoint < 0) *zeroPoint = 0;
    if (*zeroPoint >= quantLevels) *zeroPoint = quantLevels - 1;
    
    /* Quantize values */
    uint8_t *quant8 = (uint8_t*)quantized;
    uint16_t *quant16 = (uint16_t*)quantized;
    
    for (size_t i = 0; i < size; i++) {
        int quantVal = (int)roundf(input[i] / *scale) + *zeroPoint;
        
        /* Clamp to valid range */
        if (quantVal < 0) quantVal = 0;
        if (quantVal >= quantLevels) quantVal = quantLevels - 1;
        
        switch (bitWidth) {
            case HYPERION_QUANT_8BIT:
                quant8[i] = (uint8_t)quantVal;
                break;
            case HYPERION_QUANT_16BIT:
                quant16[i] = (uint16_t)quantVal;
                break;
            case HYPERION_QUANT_4BIT:
                /* Pack two 4-bit values into one byte */
                if (i % 2 == 0) {
                    quant8[i/2] = (uint8_t)(quantVal & 0xF);
                } else {
                    quant8[i/2] |= (uint8_t)((quantVal & 0xF) << 4);
                }
                break;
            default:
                quant8[i] = (uint8_t)quantVal;
        }
    }
    
    return true;
}

bool hyperionBinaryQuantize(const float *input, size_t size,
                           uint8_t *quantized, float *scale) {
    if (!input || !quantized || !scale || size == 0) return false;
    
    /* Compute mean for threshold */
    float sum = 0.0f;
    for (size_t i = 0; i < size; i++) {
        sum += fabsf(input[i]);
    }
    *scale = sum / size;
    
    /* Binary quantization: +1 for positive, -1 for negative */
    size_t packedSize = (size + 7) / 8; /* Pack 8 bits per byte */
    memset(quantized, 0, packedSize);
    
    for (size_t i = 0; i < size; i++) {
        if (input[i] >= 0) {
            /* Set bit for +1 */
            quantized[i / 8] |= (1 << (i % 8));
        }
        /* Bit remains 0 for -1 */
    }
    
    return true;
}

bool hyperionTernaryQuantize(const float *input, size_t size, float threshold,
                            int8_t *quantized, float *scale) {
    if (!input || !quantized || !scale || size == 0) return false;
    
    /* Compute scale from non-zero values */
    float sum = 0.0f;
    int count = 0;
    
    for (size_t i = 0; i < size; i++) {
        if (fabsf(input[i]) > threshold) {
            sum += fabsf(input[i]);
            count++;
        }
    }
    
    *scale = (count > 0) ? (sum / count) : 1.0f;
    
    /* Ternary quantization: -1, 0, +1 */
    for (size_t i = 0; i < size; i++) {
        if (input[i] > threshold) {
            quantized[i] = 1;
        } else if (input[i] < -threshold) {
            quantized[i] = -1;
        } else {
            quantized[i] = 0;
        }
    }
    
    return true;
}

bool hyperionDynamicQuantizeActivations(AdvancedQuantization *quant,
                                       const float *activations, size_t size,
                                       HyperionQuantBitWidth bitWidth,
                                       void *quantizedActivations,
                                       float *scale, int *zeroPoint) {
    if (!quant || !activations || !quantizedActivations || !scale || !zeroPoint) {
        return false;
    }
    
    /* Compute statistics for current activations */
    QuantStats stats = {0};
    stats.histogramBins = 0; /* Don't compute histogram for speed */
    
    if (!hyperionComputeQuantStats(activations, size, &stats)) {
        return false;
    }
    
    /* Use asymmetric quantization for activations */
    return hyperionAsymmetricQuantize(activations, size, bitWidth,
                                    quantizedActivations, scale, zeroPoint);
}

bool hyperionFakeQuantize(const float *input, size_t size,
                         HyperionQuantBitWidth bitWidth,
                         float scale, int zeroPoint, float *fakeQuantized) {
    if (!input || !fakeQuantized || size == 0) return false;
    
    int quantLevels = getQuantLevels(bitWidth);
    
    for (size_t i = 0; i < size; i++) {
        /* Quantize */
        int quantVal = (int)roundf(input[i] / scale) + zeroPoint;
        
        /* Clamp */
        if (quantVal < 0) quantVal = 0;
        if (quantVal >= quantLevels) quantVal = quantLevels - 1;
        
        /* Dequantize back to float */
        fakeQuantized[i] = scale * (quantVal - zeroPoint);
    }
    
    return true;
}

bool hyperionDequantize(const void *quantized, size_t size,
                       HyperionQuantBitWidth bitWidth,
                       float scale, int zeroPoint, float *output) {
    if (!quantized || !output || size == 0) return false;
    
    const uint8_t *quant8 = (const uint8_t*)quantized;
    const uint16_t *quant16 = (const uint16_t*)quantized;
    
    for (size_t i = 0; i < size; i++) {
        int quantVal = 0;
        
        switch (bitWidth) {
            case HYPERION_QUANT_8BIT:
                quantVal = quant8[i];
                break;
            case HYPERION_QUANT_16BIT:
                quantVal = quant16[i];
                break;
            case HYPERION_QUANT_4BIT:
                if (i % 2 == 0) {
                    quantVal = quant8[i/2] & 0xF;
                } else {
                    quantVal = (quant8[i/2] >> 4) & 0xF;
                }
                break;
            default:
                quantVal = quant8[i];
        }
        
        output[i] = scale * (quantVal - zeroPoint);
    }
    
    return true;
}

static int bitWidthToBits(HyperionQuantBitWidth bitWidth) {
    switch (bitWidth) {
        case HYPERION_QUANT_1BIT: return 1;
        case HYPERION_QUANT_2BIT: return 2;
        case HYPERION_QUANT_3BIT: return 3;
        case HYPERION_QUANT_4BIT: return 4;
        case HYPERION_QUANT_16BIT: return 16;
        default: return 8;
    }
}

bool hyperionAdaptiveBitWidthSelection(AdvancedQuantization *quant,
                                      const float **weights, const size_t *weightSizes,
                                      int numLayers, const float **calibrationData,
                                      int numSamples, size_t sampleSize,
                                      HyperionQuantBitWidth *optimalBitWidths) {
    if (!quant || !weights || !weightSizes || !optimalBitWidths || numLayers <= 0) {
        return false;
    }

    float *sensitivity = (float*)hyperionCalloc(numLayers, sizeof(float));
    if (!sensitivity) {
        return false;
    }

    float *calibrationVariance = NULL;
    if (calibrationData && numSamples > 0 && sampleSize > 0) {
        calibrationVariance = (float*)hyperionCalloc(numLayers, sizeof(float));
    }

    double totalOriginalBits = 0.0;
    double totalQuantBits = 0.0;

    for (int i = 0; i < numLayers; i++) {
        size_t layerSize = weightSizes[i];
        const float *layerWeights = weights[i];
        if (!layerWeights || layerSize == 0) {
            continue;
        }

        double sumAbs = 0.0;
        for (size_t j = 0; j < layerSize; j++) {
            sumAbs += fabsf(layerWeights[j]);
        }

        float avgAbs = (layerSize > 0) ? (float)(sumAbs / (double)layerSize) : 0.0f;
        sensitivity[i] = avgAbs;

        if (quant->sensitivityScores && i < quant->numLayers) {
            quant->sensitivityScores[i] = avgAbs;
        }

        HyperionQuantBitWidth bitWidth;
        if (avgAbs > 0.75f) {
            bitWidth = HYPERION_QUANT_8BIT;
        } else if (avgAbs > 0.4f) {
            bitWidth = HYPERION_QUANT_4BIT;
        } else if (avgAbs > 0.2f) {
            bitWidth = HYPERION_QUANT_3BIT;
        } else {
            bitWidth = HYPERION_QUANT_2BIT;
        }

        optimalBitWidths[i] = bitWidth;
        if (quant->optimalBitWidths && i < quant->numLayers) {
            quant->optimalBitWidths[i] = bitWidth;
        }

        totalOriginalBits += (double)layerSize * 32.0;
        totalQuantBits += (double)layerSize * bitWidthToBits(bitWidth);

        if (calibrationVariance) {
            double sumSq = 0.0;
            for (int s = 0; s < numSamples; s++) {
                const float *sample = calibrationData[s];
                if (!sample) continue;
                for (size_t v = 0; v < sampleSize; v++) {
                    float value = sample[v];
                    sumSq += value * value;
                }
            }
            calibrationVariance[i] = (float)(sumSq / (double)(numSamples * sampleSize));
        }
    }

    if (quant->config.mixedPrecision.memoryBudget > 0.0f && totalOriginalBits > 0.0) {
        double targetBits = totalOriginalBits * quant->config.mixedPrecision.memoryBudget;
        if (targetBits < 1.0) {
            targetBits = totalOriginalBits * 0.5;
        }

        while (totalQuantBits > targetBits) {
            int bestIndex = -1;
            float lowestSensitivity = FLT_MAX;

            for (int i = 0; i < numLayers; i++) {
                int currentBits = bitWidthToBits(optimalBitWidths[i]);
                if (currentBits <= 2) continue;
                if (sensitivity[i] < lowestSensitivity) {
                    lowestSensitivity = sensitivity[i];
                    bestIndex = i;
                }
            }

            if (bestIndex < 0) {
                break;
            }

            int currentBits = bitWidthToBits(optimalBitWidths[bestIndex]);
            int newBits = (currentBits == 8) ? 4 : (currentBits == 4 ? 3 : 2);

            totalQuantBits -= (double)weightSizes[bestIndex] * (currentBits - newBits);

            switch (newBits) {
                case 4: optimalBitWidths[bestIndex] = HYPERION_QUANT_4BIT; break;
                case 3: optimalBitWidths[bestIndex] = HYPERION_QUANT_3BIT; break;
                default: optimalBitWidths[bestIndex] = HYPERION_QUANT_2BIT; break;
            }

            if (quant->optimalBitWidths && bestIndex < quant->numLayers) {
                quant->optimalBitWidths[bestIndex] = optimalBitWidths[bestIndex];
            }
        }
    }

    if (calibrationVariance) {
        for (int i = 0; i < numLayers; i++) {
            if (calibrationVariance[i] > 0.5f && optimalBitWidths[i] < HYPERION_QUANT_4BIT) {
                optimalBitWidths[i] = HYPERION_QUANT_4BIT;
                if (quant->optimalBitWidths && i < quant->numLayers) {
                    quant->optimalBitWidths[i] = optimalBitWidths[i];
                }
            }
        }
    }

    hyperionFree(calibrationVariance);
    hyperionFree(sensitivity);
    return true;
}

bool hyperionMixedPrecisionQuantize(AdvancedQuantization *quant,
                                   const float **weights, const size_t *weightSizes,
                                   int numLayers, void **quantizedWeights,
                                   void **quantParams) {
    if (!quant || !weights || !weightSizes || !quantizedWeights || !quantParams || numLayers <= 0) {
        return false;
    }

    for (int i = 0; i < numLayers; i++) {
        quantizedWeights[i] = NULL;
        quantParams[i] = NULL;
    }

    HyperionQuantBitWidth *localAssignments = NULL;
    const HyperionQuantBitWidth *bitSource = NULL;

    if (quant->config.mixedPrecision.autoAssign) {
        bool needsLocal = (!quant->optimalBitWidths || quant->numLayers < numLayers);
        if (needsLocal) {
            localAssignments = (HyperionQuantBitWidth*)hyperionCalloc(numLayers, sizeof(HyperionQuantBitWidth));
            if (!localAssignments) {
                return false;
            }
            if (!hyperionAdaptiveBitWidthSelection(quant, weights, weightSizes, numLayers,
                                                   NULL, 0, 0, localAssignments)) {
                hyperionFree(localAssignments);
                return false;
            }
            bitSource = localAssignments;
        } else {
            bitSource = quant->optimalBitWidths;
        }
    } else {
        bitSource = quant->optimalBitWidths;
    }

    quant->originalMemory = 0;
    quant->quantizedMemory = 0;

    for (int i = 0; i < numLayers; i++) {
        const float *layerWeights = weights[i];
        size_t layerSize = weightSizes[i];
        if (!layerWeights || layerSize == 0) {
            hyperionFree(localAssignments);
            for (int k = 0; k < i; k++) {
                hyperionFree(quantizedWeights[k]);
                hyperionFree(quantParams[k]);
            }
            return false;
        }

        HyperionQuantBitWidth bitWidth = quant->config.defaultBitWidth;
        if (bitSource && i < numLayers) {
            bitWidth = bitSource[i];
        }
        if (bitWidth == HYPERION_QUANT_MIXED || bitWidth == HYPERION_QUANT_DYNAMIC) {
            bitWidth = HYPERION_QUANT_4BIT;
        }

        size_t bufferSize = computeQuantizedBufferSize(layerSize, bitWidth);
        void *buffer = hyperionCalloc(bufferSize, sizeof(uint8_t));
        if (!buffer) {
            hyperionFree(localAssignments);
            for (int k = 0; k < i; k++) {
                hyperionFree(quantizedWeights[k]);
                hyperionFree(quantParams[k]);
            }
            return false;
        }

        float scale = 1.0f;
        int zeroPoint = 0;
        bool success;

        switch (quant->config.method) {
            case HYPERION_QUANT_LOG:
                success = hyperionLogQuantize(layerWeights, layerSize, bitWidth, buffer, &scale);
                zeroPoint = 0;
                break;
            case HYPERION_QUANT_SYMMETRIC:
            case HYPERION_QUANT_LINEAR:
            case HYPERION_QUANT_ASYMMETRIC:
            case HYPERION_QUANT_ADAPTIVE:
            case HYPERION_QUANT_LEARNED:
            default:
                success = hyperionAsymmetricQuantize(layerWeights, layerSize, bitWidth,
                                                     buffer, &scale, &zeroPoint);
                if (!success && bitWidth != HYPERION_QUANT_8BIT) {
                    /* Fallback to 8-bit quantization */
                    bitWidth = HYPERION_QUANT_8BIT;
                    bufferSize = computeQuantizedBufferSize(layerSize, bitWidth);
                    void *fallback = hyperionRealloc(buffer, bufferSize);
                    if (!fallback) {
                        hyperionFree(buffer);
                        hyperionFree(localAssignments);
                        for (int k = 0; k < i; k++) {
                            hyperionFree(quantizedWeights[k]);
                            hyperionFree(quantParams[k]);
                        }
                        return false;
                    }
                    buffer = fallback;
                    success = hyperionAsymmetricQuantize(layerWeights, layerSize, bitWidth,
                                                         buffer, &scale, &zeroPoint);
                }
                break;
        }

        if (!success) {
            hyperionFree(buffer);
            hyperionFree(localAssignments);
            for (int k = 0; k < i; k++) {
                hyperionFree(quantizedWeights[k]);
                hyperionFree(quantParams[k]);
            }
            return false;
        }

        LayerQuantParams *params = (LayerQuantParams*)hyperionCalloc(1, sizeof(LayerQuantParams));
        if (!params) {
            hyperionFree(buffer);
            hyperionFree(localAssignments);
            for (int k = 0; k < i; k++) {
                hyperionFree(quantizedWeights[k]);
                hyperionFree(quantParams[k]);
            }
            return false;
        }

        params->scale = scale;
        params->zeroPoint = zeroPoint;
        params->bitWidth = bitWidth;
        params->logOffset = (quant->config.method == HYPERION_QUANT_LOG) ? g_lastLogOffset : 0.0f;

        quantizedWeights[i] = buffer;
        quantParams[i] = params;

        quant->originalMemory += layerSize * sizeof(float);
        quant->quantizedMemory += bufferSize;
    }

    hyperionFree(localAssignments);
    return true;
}

bool hyperionLogQuantize(const float *input, size_t size,
                         HyperionQuantBitWidth bitWidth,
                         void *quantized, float *scale) {
    if (!input || !quantized || !scale || size == 0) return false;

    float minExp = FLT_MAX;
    float maxExp = -FLT_MAX;
    const float epsilon = 1e-8f;

    for (size_t i = 0; i < size; i++) {
        float value = fabsf(input[i]) + epsilon;
        float exponent = log2f(value);
        if (exponent < minExp) minExp = exponent;
        if (exponent > maxExp) maxExp = exponent;
    }

    int levels = getQuantLevels(bitWidth);
    if (levels < 2) levels = 2;

    float range = maxExp - minExp;
    if (range < epsilon) {
        range = epsilon;
    }

    float step = range / (float)(levels - 1);
    uint8_t *out = (uint8_t*)quantized;

    for (size_t i = 0; i < size; i++) {
        float value = fabsf(input[i]) + epsilon;
        float exponent = log2f(value);
        int index = (int)roundf((exponent - minExp) / step);
        if (index < 0) index = 0;
        if (index >= levels) index = levels - 1;
        uint8_t encoded = (uint8_t)index;
        if (input[i] < 0.0f) {
            encoded |= 0x80u;
        }
        out[i] = encoded;
    }

    *scale = step;
    g_lastLogOffset = minExp;
    return true;
}

bool hyperionCalibrateQuantization(AdvancedQuantization *quant,
                                   const float **calibrationData, int numSamples,
                                   size_t sampleSize, HyperionQuantBitWidth bitWidth,
                                   float *optimalScale, int *optimalZeroPoint) {
    if (!quant || !calibrationData || numSamples <= 0 || sampleSize == 0 ||
        !optimalScale || !optimalZeroPoint) {
        return false;
    }

    float globalMin = FLT_MAX;
    float globalMax = -FLT_MAX;

    for (int i = 0; i < numSamples; i++) {
        const float *sample = calibrationData[i];
        if (!sample) continue;
        for (size_t j = 0; j < sampleSize; j++) {
            float value = sample[j];
            if (value < globalMin) globalMin = value;
            if (value > globalMax) globalMax = value;
        }
    }

    if (globalMin >= globalMax) {
        *optimalScale = 1.0f;
        *optimalZeroPoint = 0;
        return true;
    }

    int levels = getQuantLevels(bitWidth);
    float scale = (globalMax - globalMin) / (float)(levels - 1);
    if (scale <= 0.0f) scale = 1.0f;

    int zeroPoint = (int)roundf(-globalMin / scale);
    if (zeroPoint < 0) zeroPoint = 0;
    if (zeroPoint >= levels) zeroPoint = levels - 1;

    *optimalScale = scale;
    *optimalZeroPoint = zeroPoint;
    return true;
}

bool hyperionQuantizedMatMul(const void *quantA, const void *quantB,
                             int M, int N, int K,
                             HyperionQuantBitWidth bitWidthA, HyperionQuantBitWidth bitWidthB,
                             float scaleA, float scaleB,
                             int zeroPointA, int zeroPointB,
                             void *quantC, float *scaleC, int *zeroPointC) {
    if (!quantA || !quantB || !quantC || !scaleC || !zeroPointC) return false;
    if (M <= 0 || N <= 0 || K <= 0) return false;

    size_t sizeA = (size_t)M * (size_t)K;
    size_t sizeB = (size_t)K * (size_t)N;
    size_t sizeC = (size_t)M * (size_t)N;

    float *dequantA = (float*)hyperionCalloc(sizeA, sizeof(float));
    float *dequantB = (float*)hyperionCalloc(sizeB, sizeof(float));
    float *result = (float*)hyperionCalloc(sizeC, sizeof(float));

    if (!dequantA || !dequantB || !result) {
        hyperionFree(dequantA);
        hyperionFree(dequantB);
        hyperionFree(result);
        return false;
    }

    hyperionDequantize(quantA, sizeA, bitWidthA, scaleA, zeroPointA, dequantA);
    hyperionDequantize(quantB, sizeB, bitWidthB, scaleB, zeroPointB, dequantB);

    for (int m = 0; m < M; m++) {
        for (int n = 0; n < N; n++) {
            float acc = 0.0f;
            for (int k = 0; k < K; k++) {
                acc += dequantA[m * K + k] * dequantB[k * N + n];
            }
            result[m * N + n] = acc;
        }
    }

    bool success = hyperionAsymmetricQuantize(result, sizeC, HYPERION_QUANT_8BIT,
                                              quantC, scaleC, zeroPointC);

    hyperionFree(dequantA);
    hyperionFree(dequantB);
    hyperionFree(result);

    return success;
}

bool hyperionQuantGetMemorySavings(const AdvancedQuantization *quant,
                                  size_t originalSize, size_t *quantizedSize,
                                  float *compressionRatio) {
    if (!quant || !quantizedSize || !compressionRatio) return false;
    
    /* Estimate quantized size based on bit-width */
    float avgBitsPerWeight = 32.0f; /* Original float32 */
    
    if (quant->config.defaultBitWidth != HYPERION_QUANT_MIXED) {
        switch (quant->config.defaultBitWidth) {
            case HYPERION_QUANT_1BIT: avgBitsPerWeight = 1.0f; break;
            case HYPERION_QUANT_2BIT: avgBitsPerWeight = 2.0f; break;
            case HYPERION_QUANT_3BIT: avgBitsPerWeight = 3.0f; break;
            case HYPERION_QUANT_4BIT: avgBitsPerWeight = 4.0f; break;
            case HYPERION_QUANT_8BIT: avgBitsPerWeight = 8.0f; break;
            case HYPERION_QUANT_16BIT: avgBitsPerWeight = 16.0f; break;
            default: avgBitsPerWeight = 8.0f;
        }
    } else {
        /* Mixed precision - compute weighted average */
        if (quant->optimalBitWidths && quant->numLayers > 0) {
            float totalBits = 0.0f;
            for (int i = 0; i < quant->numLayers; i++) {
                switch (quant->optimalBitWidths[i]) {
                    case HYPERION_QUANT_1BIT: totalBits += 1.0f; break;
                    case HYPERION_QUANT_2BIT: totalBits += 2.0f; break;
                    case HYPERION_QUANT_3BIT: totalBits += 3.0f; break;
                    case HYPERION_QUANT_4BIT: totalBits += 4.0f; break;
                    case HYPERION_QUANT_8BIT: totalBits += 8.0f; break;
                    case HYPERION_QUANT_16BIT: totalBits += 16.0f; break;
                    default: totalBits += 8.0f;
                }
            }
            avgBitsPerWeight = totalBits / quant->numLayers;
        } else {
            avgBitsPerWeight = 4.0f; /* Default mixed precision estimate */
        }
    }
    
    *quantizedSize = (size_t)(originalSize * avgBitsPerWeight / 32.0f);
    *compressionRatio = 32.0f / avgBitsPerWeight;
    
    return true;
}

bool hyperionQuantEnableSIMD(AdvancedQuantization *quant, bool enable) {
    if (!quant) return false;
    quant->config.useSIMD = enable;
    return true;
}

bool hyperionQuantBenchmark(AdvancedQuantization *quant, size_t dataSize,
                           int numIterations, float *avgTimeMs, float *throughputMBps) {
    if (!quant || !avgTimeMs || !throughputMBps || numIterations <= 0) return false;
    
    /* Create test data */
    float *testData = (float*)hyperionAlloc(dataSize * sizeof(float));
    uint8_t *quantizedData = (uint8_t*)hyperionAlloc(dataSize);
    
    if (!testData || !quantizedData) {
        hyperionFree(testData);
        hyperionFree(quantizedData);
        return false;
    }
    
    /* Initialize test data */
    for (size_t i = 0; i < dataSize; i++) {
        testData[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
    }
    
    /* Benchmark quantization */
    clock_t start = clock();
    
    for (int iter = 0; iter < numIterations; iter++) {
        float scale;
        int zeroPoint;
        hyperionAsymmetricQuantize(testData, dataSize, HYPERION_QUANT_8BIT,
                                 quantizedData, &scale, &zeroPoint);
    }
    
    clock_t end = clock();
    
    double totalTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    *avgTimeMs = (float)(totalTime * 1000.0 / numIterations);
    
    double dataPerIteration = dataSize * sizeof(float);
    *throughputMBps = (float)(dataPerIteration * numIterations / totalTime / (1024.0 * 1024.0));
    
    hyperionFree(testData);
    hyperionFree(quantizedData);
    
    return true;
}