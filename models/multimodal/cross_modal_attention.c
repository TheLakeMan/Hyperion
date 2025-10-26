#include "cross_modal_attention.h"
#include "../../core/memory.h"
#include "../../utils/simd_ops.h"
#include "../../utils/quantize.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/**
 * Cross-modal attention module structure
 */
struct CrossModalAttention {
    CrossModalAttnConfig config;
    
    /* Weight matrices for multi-head attention */
    float *queryWeights;    /* Query projection weights */
    float *keyWeights;      /* Key projection weights */
    float *valueWeights;    /* Value projection weights */
    float *outputWeights;   /* Output projection weights */
    
    /* Bias vectors */
    float *queryBias;
    float *keyBias;
    float *valueBias;
    float *outputBias;
    
    /* Layer normalization parameters */
    float *layerNormGamma;
    float *layerNormBeta;
    
    /* Quantized weights (if quantization enabled) */
    void *quantizedWeights;
    
    /* Temporary buffers for computation */
    float *tempQuery;
    float *tempKey;
    float *tempValue;
    float *attnScores;
    float *attnWeights;
    
    /* Memory management */
    size_t weightMemory;
    size_t activationMemory;
    bool initialized;
};

/* Helper function to initialize weights */
static bool initializeWeights(CrossModalAttention *attn) {
    const CrossModalAttnConfig *config = &attn->config;
    int totalDim = config->numHeads * config->headDim;
    
    /* Allocate weight matrices */
    size_t weightSize = totalDim * totalDim * sizeof(float);
    attn->queryWeights = (float*)hyperionAlloc(weightSize);
    attn->keyWeights = (float*)hyperionAlloc(weightSize);
    attn->valueWeights = (float*)hyperionAlloc(weightSize);
    attn->outputWeights = (float*)hyperionAlloc(weightSize);
    
    if (!attn->queryWeights || !attn->keyWeights || 
        !attn->valueWeights || !attn->outputWeights) {
        return false;
    }
    
    /* Initialize with Xavier/Glorot initialization */
    float scale = sqrtf(2.0f / (totalDim + totalDim));
    for (int i = 0; i < totalDim * totalDim; i++) {
        attn->queryWeights[i] = (float)(rand() / (double)RAND_MAX - 0.5) * 2.0f * scale;
        attn->keyWeights[i] = (float)(rand() / (double)RAND_MAX - 0.5) * 2.0f * scale;
        attn->valueWeights[i] = (float)(rand() / (double)RAND_MAX - 0.5) * 2.0f * scale;
        attn->outputWeights[i] = (float)(rand() / (double)RAND_MAX - 0.5) * 2.0f * scale;
    }
    
    /* Allocate bias vectors */
    size_t biasSize = totalDim * sizeof(float);
    attn->queryBias = (float*)hyperionCalloc(totalDim, sizeof(float));
    attn->keyBias = (float*)hyperionCalloc(totalDim, sizeof(float));
    attn->valueBias = (float*)hyperionCalloc(totalDim, sizeof(float));
    attn->outputBias = (float*)hyperionCalloc(totalDim, sizeof(float));
    
    if (!attn->queryBias || !attn->keyBias || 
        !attn->valueBias || !attn->outputBias) {
        return false;
    }
    
    attn->weightMemory = 4 * weightSize + 4 * biasSize;
    
    /* Layer normalization parameters */
    if (config->useLayerNorm) {
        attn->layerNormGamma = (float*)hyperionAlloc(biasSize);
        attn->layerNormBeta = (float*)hyperionCalloc(totalDim, sizeof(float));
        
        if (!attn->layerNormGamma || !attn->layerNormBeta) {
            return false;
        }
        
        /* Initialize gamma to 1.0 */
        for (int i = 0; i < totalDim; i++) {
            attn->layerNormGamma[i] = 1.0f;
        }
        
        attn->weightMemory += 2 * biasSize;
    }
    
    return true;
}

/* Helper function to allocate temporary buffers */
static bool allocateTempBuffers(CrossModalAttention *attn) {
    const CrossModalAttnConfig *config = &attn->config;
    int totalDim = config->numHeads * config->headDim;
    int maxSeqLen = config->maxSeqLen;
    
    /* Temporary projection buffers */
    size_t projSize = maxSeqLen * totalDim * sizeof(float);
    attn->tempQuery = (float*)hyperionAlloc(projSize);
    attn->tempKey = (float*)hyperionAlloc(projSize);
    attn->tempValue = (float*)hyperionAlloc(projSize);
    
    if (!attn->tempQuery || !attn->tempKey || !attn->tempValue) {
        return false;
    }
    
    /* Attention score and weight buffers */
    size_t attnSize = maxSeqLen * maxSeqLen * config->numHeads * sizeof(float);
    attn->attnScores = (float*)hyperionAlloc(attnSize);
    attn->attnWeights = (float*)hyperionAlloc(attnSize);
    
    if (!attn->attnScores || !attn->attnWeights) {
        return false;
    }
    
    attn->activationMemory = 3 * projSize + 2 * attnSize;
    return true;
}

/* Softmax function with numerical stability */
static void softmax(float *input, int length) {
    /* Find maximum for numerical stability */
    float maxVal = input[0];
    for (int i = 1; i < length; i++) {
        if (input[i] > maxVal) {
            maxVal = input[i];
        }
    }
    
    /* Compute exponentials and sum */
    float sum = 0.0f;
    for (int i = 0; i < length; i++) {
        input[i] = expf(input[i] - maxVal);
        sum += input[i];
    }
    
    /* Normalize */
    float invSum = 1.0f / sum;
    for (int i = 0; i < length; i++) {
        input[i] *= invSum;
    }
}

/* Layer normalization */
static void layerNorm(const float *input, float *output, int dim,
                     const float *gamma, const float *beta) {
    /* Compute mean */
    float mean = 0.0f;
    for (int i = 0; i < dim; i++) {
        mean += input[i];
    }
    mean /= dim;
    
    /* Compute variance */
    float variance = 0.0f;
    for (int i = 0; i < dim; i++) {
        float diff = input[i] - mean;
        variance += diff * diff;
    }
    variance /= dim;
    
    /* Apply normalization */
    float invStd = 1.0f / sqrtf(variance + 1e-6f);
    for (int i = 0; i < dim; i++) {
        output[i] = (input[i] - mean) * invStd * gamma[i] + beta[i];
    }
}

CrossModalAttention *hyperionCrossModalAttnCreate(const CrossModalAttnConfig *config) {
    if (!config || config->numHeads <= 0 || config->headDim <= 0 || config->maxSeqLen <= 0) {
        return NULL;
    }
    
    CrossModalAttention *attn = (CrossModalAttention*)hyperionCalloc(1, sizeof(CrossModalAttention));
    if (!attn) {
        return NULL;
    }
    
    /* Copy configuration */
    attn->config = *config;
    
    /* Initialize weights */
    if (!initializeWeights(attn)) {
        hyperionCrossModalAttnFree(attn);
        return NULL;
    }
    
    /* Allocate temporary buffers */
    if (!allocateTempBuffers(attn)) {
        hyperionCrossModalAttnFree(attn);
        return NULL;
    }
    
    attn->initialized = true;
    return attn;
}

void hyperionCrossModalAttnFree(CrossModalAttention *attn) {
    if (!attn) return;
    
    /* Free weight matrices */
    hyperionFree(attn->queryWeights);
    hyperionFree(attn->keyWeights);
    hyperionFree(attn->valueWeights);
    hyperionFree(attn->outputWeights);
    
    /* Free bias vectors */
    hyperionFree(attn->queryBias);
    hyperionFree(attn->keyBias);
    hyperionFree(attn->valueBias);
    hyperionFree(attn->outputBias);
    
    /* Free layer norm parameters */
    hyperionFree(attn->layerNormGamma);
    hyperionFree(attn->layerNormBeta);
    
    /* Free quantized weights */
    hyperionFree(attn->quantizedWeights);
    
    /* Free temporary buffers */
    hyperionFree(attn->tempQuery);
    hyperionFree(attn->tempKey);
    hyperionFree(attn->tempValue);
    hyperionFree(attn->attnScores);
    hyperionFree(attn->attnWeights);
    
    hyperionFree(attn);
}

bool hyperionCrossModalAttnCompute(CrossModalAttention *attn,
                                 const float *queryFeatures, const float *keyFeatures, 
                                 const float *valueFeatures,
                                 int queryDim, int keyDim, int valueDim,
                                 int queryLen, int keyLen,
                                 float *output, int outputDim,
                                 const AttentionMask *mask) {
    if (!attn || !attn->initialized) return false;
    if (!queryFeatures || !keyFeatures || !valueFeatures || !output) return false;
    
    const CrossModalAttnConfig *config = &attn->config;
    int totalDim = config->numHeads * config->headDim;
    int headDim = config->headDim;
    
    /* Project inputs to query, key, value */
    /* This is a simplified version - in practice would use proper matrix multiplication */
    for (int seq = 0; seq < queryLen; seq++) {
        for (int dim = 0; dim < totalDim; dim++) {
            attn->tempQuery[seq * totalDim + dim] = 
                queryFeatures[seq * queryDim + (dim % queryDim)] + attn->queryBias[dim];
        }
    }
    
    for (int seq = 0; seq < keyLen; seq++) {
        for (int dim = 0; dim < totalDim; dim++) {
            attn->tempKey[seq * totalDim + dim] = 
                keyFeatures[seq * keyDim + (dim % keyDim)] + attn->keyBias[dim];
            attn->tempValue[seq * totalDim + dim] = 
                valueFeatures[seq * valueDim + (dim % valueDim)] + attn->valueBias[dim];
        }
    }
    
    /* Multi-head attention computation */
    float scale = 1.0f / sqrtf((float)headDim);
    
    for (int head = 0; head < config->numHeads; head++) {
        int headOffset = head * headDim;
        
        /* Compute attention scores for this head */
        for (int q = 0; q < queryLen; q++) {
            for (int k = 0; k < keyLen; k++) {
                float score = 0.0f;
                
                /* Dot product between query and key for this head */
                for (int d = 0; d < headDim; d++) {
                    float queryVal = attn->tempQuery[q * totalDim + headOffset + d];
                    float keyVal = attn->tempKey[k * totalDim + headOffset + d];
                    score += queryVal * keyVal;
                }
                
                score *= scale;
                
                /* Apply mask if provided */
                if (mask && mask->mask && q < mask->rows && k < mask->cols) {
                    if (!mask->mask[q * mask->cols + k]) {
                        score = -1e9f;  /* Large negative value */
                    }
                }
                
                attn->attnScores[head * queryLen * keyLen + q * keyLen + k] = score;
            }
            
            /* Apply softmax to attention scores for this query position */
            softmax(&attn->attnScores[head * queryLen * keyLen + q * keyLen], keyLen);
        }
        
        /* Compute attended output for this head */
        for (int q = 0; q < queryLen; q++) {
            for (int d = 0; d < headDim; d++) {
                float attended = 0.0f;
                
                for (int k = 0; k < keyLen; k++) {
                    float weight = attn->attnScores[head * queryLen * keyLen + q * keyLen + k];
                    float value = attn->tempValue[k * totalDim + headOffset + d];
                    attended += weight * value;
                }
                
                /* Store in output (simplified - should use proper projection) */
                if (q < queryLen && headOffset + d < outputDim) {
                    output[q * outputDim + headOffset + d] = attended;
                }
            }
        }
    }
    
    /* Apply layer normalization if enabled */
    if (config->useLayerNorm && attn->layerNormGamma && attn->layerNormBeta) {
        for (int seq = 0; seq < queryLen; seq++) {
            layerNorm(&output[seq * outputDim], &output[seq * outputDim],
                     outputDim, attn->layerNormGamma, attn->layerNormBeta);
        }
    }
    
    return true;
}

bool hyperionCrossModalAttnBidirectional(CrossModalAttention *attn,
                                       const float *features1, const float *features2,
                                       int dim1, int dim2, int len1, int len2,
                                       float *output1, float *output2,
                                       const AttentionMask *mask) {
    if (!attn || !features1 || !features2 || !output1 || !output2) {
        return false;
    }
    
    /* Compute attention from modality 1 to modality 2 */
    bool success1 = hyperionCrossModalAttnCompute(attn, features1, features2, features2,
                                                dim1, dim2, dim2, len1, len2,
                                                output1, dim1, mask);
    
    /* Compute attention from modality 2 to modality 1 */
    bool success2 = hyperionCrossModalAttnCompute(attn, features2, features1, features1,
                                                dim2, dim1, dim1, len2, len1,
                                                output2, dim2, mask);
    
    return success1 && success2;
}

AttentionMask *hyperionAttnMaskCreate(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;
    
    AttentionMask *mask = (AttentionMask*)hyperionCalloc(1, sizeof(AttentionMask));
    if (!mask) return NULL;
    
    mask->mask = (bool*)hyperionCalloc(rows * cols, sizeof(bool));
    if (!mask->mask) {
        hyperionFree(mask);
        return NULL;
    }
    
    mask->rows = rows;
    mask->cols = cols;
    
    /* Initialize to all true (no masking) */
    for (int i = 0; i < rows * cols; i++) {
        mask->mask[i] = true;
    }
    
    return mask;
}

void hyperionAttnMaskFree(AttentionMask *mask) {
    if (!mask) return;
    hyperionFree(mask->mask);
    hyperionFree(mask);
}

bool hyperionAttnMaskSetCausal(AttentionMask *mask) {
    if (!mask || !mask->mask) return false;
    
    /* Set lower triangular mask */
    for (int i = 0; i < mask->rows; i++) {
        for (int j = 0; j < mask->cols; j++) {
            mask->mask[i * mask->cols + j] = (j <= i);
        }
    }
    
    return true;
}

TemporalContext *hyperionTemporalContextCreate(int maxLength, int hiddenDim) {
    if (maxLength <= 0 || hiddenDim <= 0) return NULL;
    
    TemporalContext *context = (TemporalContext*)hyperionCalloc(1, sizeof(TemporalContext));
    if (!context) return NULL;
    
    context->hiddenStates = (float*)hyperionCalloc(maxLength * hiddenDim, sizeof(float));
    context->positions = (int*)hyperionCalloc(maxLength, sizeof(int));
    
    if (!context->hiddenStates || !context->positions) {
        hyperionTemporalContextFree(context);
        return NULL;
    }
    
    context->maxLength = maxLength;
    context->sequenceLength = 0;
    
    return context;
}

void hyperionTemporalContextFree(TemporalContext *context) {
    if (!context) return;
    hyperionFree(context->hiddenStates);
    hyperionFree(context->positions);
    hyperionFree(context);
}

bool hyperionCrossModalAttnGetMemoryUsage(const CrossModalAttention *attn,
                                        size_t *weightMemory, size_t *activationMemory) {
    if (!attn || !weightMemory || !activationMemory) return false;
    
    *weightMemory = attn->weightMemory;
    *activationMemory = attn->activationMemory;
    return true;
}

bool hyperionCrossModalAttnEnableSIMD(CrossModalAttention *attn, bool enable) {
    if (!attn) return false;
    attn->config.useSIMD = enable;
    return true;
}

bool hyperionCrossModalAttnSetQuantization(CrossModalAttention *attn, bool enable) {
    if (!attn) return false;
    attn->config.useQuantization = enable;
    return true;
}