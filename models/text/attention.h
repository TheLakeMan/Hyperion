/**
 * @file attention.h
 * @brief SIMD-accelerated attention mechanism for transformer models
 *
 * This file contains declarations for attention mechanisms with SIMD optimization.
 */

#ifndef HYPERION_ATTENTION_H
#define HYPERION_ATTENTION_H

#include "../../utils/quantize.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Attention parameters structure
 */
typedef struct {
    uint32_t batchSize;     /* Batch size (usually 1 for inference) */
    uint32_t seqLength;     /* Sequence length */
    uint32_t numHeads;      /* Number of attention heads */
    uint32_t headDim;       /* Dimension of each head */
    uint32_t hiddenDim;     /* Hidden dimension (numHeads * headDim) */
    bool     useCausalMask; /* Whether to use causal masking */
    float    scaleFactor;   /* Scale factor for QK^T product (usually 1/sqrt(headDim)) */
} HyperionAttentionParams;

/**
 * Self-attention structure
 */
typedef struct {
    HyperionAttentionParams params;        /* Attention parameters */
    HyperionMatrix4bit      queryWeight;   /* Query projection weights */
    HyperionMatrix4bit      keyWeight;     /* Key projection weights */
    HyperionMatrix4bit      valueWeight;   /* Value projection weights */
    HyperionMatrix4bit      outputWeight;  /* Output projection weights */
    float                *queryBias;     /* Query projection bias */
    float                *keyBias;       /* Key projection bias */
    float                *valueBias;     /* Value projection bias */
    float                *outputBias;    /* Output projection bias */
    float                *scratchMemory; /* Scratch memory for intermediate results */
} HyperionSelfAttention;

/**
 * Initialize self-attention structure
 *
 * @param attention Attention structure to initialize
 * @param params Attention parameters
 * @return 0 on success, -1 on error
 */
int hyperionInitSelfAttention(HyperionSelfAttention *attention, const HyperionAttentionParams *params);

/**
 * Free self-attention resources
 *
 * @param attention Attention structure to free
 */
void hyperionDestroySelfAttention(HyperionSelfAttention *attention);

/**
 * Set weights for self-attention
 *
 * @param attention Attention structure
 * @param queryWeight Query projection weights (4-bit quantized)
 * @param keyWeight Key projection weights (4-bit quantized)
 * @param valueWeight Value projection weights (4-bit quantized)
 * @param outputWeight Output projection weights (4-bit quantized)
 * @param queryBias Query projection bias
 * @param keyBias Key projection bias
 * @param valueBias Value projection bias
 * @param outputBias Output projection bias
 * @return 0 on success, -1 on error
 */
int hyperionSetAttentionWeights(HyperionSelfAttention *attention, const HyperionMatrix4bit *queryWeight,
                              const HyperionMatrix4bit *keyWeight,
                              const HyperionMatrix4bit *valueWeight,
                              const HyperionMatrix4bit *outputWeight, const float *queryBias,
                              const float *keyBias, const float *valueBias,
                              const float *outputBias);

/**
 * Perform self-attention operation with SIMD acceleration
 *
 * @param attention Attention structure
 * @param input Input tensor [seqLength x hiddenDim]
 * @param output Output tensor [seqLength x hiddenDim]
 * @return 0 on success, -1 on error
 */
int hyperionSelfAttentionForward(HyperionSelfAttention *attention, const float *input, float *output);

/**
 * SIMD-accelerated query-key-value projection
 *
 * @param input Input tensor [seqLength x hiddenDim]
 * @param queryWeight Query projection weights (4-bit quantized)
 * @param keyWeight Key projection weights (4-bit quantized)
 * @param valueWeight Value projection weights (4-bit quantized)
 * @param queryBias Query projection bias
 * @param keyBias Key projection bias
 * @param valueBias Value projection bias
 * @param query Output query tensor [seqLength x (numHeads*headDim)]
 * @param key Output key tensor [seqLength x (numHeads*headDim)]
 * @param value Output value tensor [seqLength x (numHeads*headDim)]
 * @param seqLength Sequence length
 * @param hiddenDim Hidden dimension
 * @param numHeads Number of attention heads
 * @param headDim Dimension of each head
 * @return 0 on success, -1 on error
 */
int hyperionSimdQKVProjection(const float *input, const HyperionMatrix4bit *queryWeight,
                            const HyperionMatrix4bit *keyWeight, const HyperionMatrix4bit *valueWeight,
                            const float *queryBias, const float *keyBias, const float *valueBias,
                            float *query, float *key, float *value, uint32_t seqLength,
                            uint32_t hiddenDim, uint32_t numHeads, uint32_t headDim);

/**
 * SIMD-accelerated attention score computation (Q*K^T)
 *
 * @param query Query tensor [seqLength x (numHeads*headDim)]
 * @param key Key tensor [seqLength x (numHeads*headDim)]
 * @param scores Output scores tensor [numHeads x seqLength x seqLength]
 * @param seqLength Sequence length
 * @param numHeads Number of attention heads
 * @param headDim Dimension of each head
 * @param scaleFactor Scale factor for QK^T product (usually 1/sqrt(headDim))
 * @param useCausalMask Whether to use causal masking
 * @return 0 on success, -1 on error
 */
int hyperionSimdAttentionScores(const float *query, const float *key, float *scores,
                              uint32_t seqLength, uint32_t numHeads, uint32_t headDim,
                              float scaleFactor, bool useCausalMask);

/**
 * SIMD-accelerated softmax computation for attention scores
 *
 * @param scores Attention scores [numHeads x seqLength x seqLength]
 * @param softmaxScores Output softmax scores [numHeads x seqLength x seqLength]
 * @param seqLength Sequence length
 * @param numHeads Number of attention heads
 * @return 0 on success, -1 on error
 */
int hyperionSimdAttentionSoftmax(const float *scores, float *softmaxScores, uint32_t seqLength,
                               uint32_t numHeads);

/**
 * SIMD-accelerated attention context computation (softmax(Q*K^T)*V)
 *
 * @param softmaxScores Softmax scores [numHeads x seqLength x seqLength]
 * @param value Value tensor [seqLength x (numHeads*headDim)]
 * @param context Output context tensor [seqLength x (numHeads*headDim)]
 * @param seqLength Sequence length
 * @param numHeads Number of attention heads
 * @param headDim Dimension of each head
 * @return 0 on success, -1 on error
 */
int hyperionSimdAttentionContext(const float *softmaxScores, const float *value, float *context,
                               uint32_t seqLength, uint32_t numHeads, uint32_t headDim);

/**
 * SIMD-accelerated output projection
 *
 * @param context Context tensor [seqLength x (numHeads*headDim)]
 * @param outputWeight Output projection weights (4-bit quantized)
 * @param outputBias Output projection bias
 * @param output Output tensor [seqLength x hiddenDim]
 * @param seqLength Sequence length
 * @param hiddenDim Hidden dimension
 * @return 0 on success, -1 on error
 */
int hyperionSimdOutputProjection(const float *context, const HyperionMatrix4bit *outputWeight,
                               const float *outputBias, float *output, uint32_t seqLength,
                               uint32_t hiddenDim);

/* Legacy function names for compatibility with benchmark tests */
int attention_compute_scores_simd(const float *query, const float *key, float *scores, 
                                int batchSize, int seqLength, int headDim);
int attention_softmax_simd(const float *scores, float *probs, int batchSize, int seqLength);
int attention_weighted_sum_simd(const float *probs, const float *value, float *output,
                              int batchSize, int seqLength, int headDim);
int attention_forward_simd(const float *query, const float *key, const float *value, float *output,
                         int batchSize, int seqLength, int headDim);
int attention_compute_scores_reference(const float *query, const float *key, float *scores,
                                     int batchSize, int seqLength, int headDim);
int attention_softmax_reference(const float *scores, float *probs, int batchSize, int seqLength);
int attention_weighted_sum_reference(const float *probs, const float *value, float *output,
                                   int batchSize, int seqLength, int headDim);
int attention_forward_reference(const float *query, const float *key, const float *value, float *output,
                              int batchSize, int seqLength, int headDim);

#endif /* HYPERION_ATTENTION_H */