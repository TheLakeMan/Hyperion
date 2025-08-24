/**
 * @file cross_modal_attention.h
 * @brief Advanced cross-modal attention mechanisms for Hyperion
 *
 * This header defines advanced attention mechanisms that enable sophisticated
 * interactions between different modalities (vision, text, audio) in multimodal
 * models. Includes multi-head cross-attention, temporal attention, and 
 * hierarchical fusion mechanisms.
 */

#ifndef HYPERION_CROSS_MODAL_ATTENTION_H
#define HYPERION_CROSS_MODAL_ATTENTION_H

#include "multimodal_model.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Cross-modal attention configuration
 */
typedef struct {
    int numHeads;           /* Number of attention heads */
    int headDim;            /* Dimension per attention head */
    int maxSeqLen;          /* Maximum sequence length */
    float dropoutRate;      /* Dropout rate for regularization */
    bool useLayerNorm;      /* Whether to use layer normalization */
    bool useResidual;       /* Whether to use residual connections */
    bool useQuantization;   /* Whether to use 4-bit quantization */
    bool useSIMD;          /* Whether to use SIMD acceleration */
} CrossModalAttnConfig;

/**
 * Multi-head cross-modal attention structure
 */
typedef struct CrossModalAttention CrossModalAttention;

/**
 * Attention mask for controlling which tokens can attend to which
 */
typedef struct {
    bool *mask;            /* Boolean mask array */
    int rows;             /* Number of rows (query length) */
    int cols;             /* Number of columns (key length) */
} AttentionMask;

/**
 * Temporal attention context for processing sequences
 */
typedef struct {
    float *hiddenStates;   /* Hidden states from previous time steps */
    int *positions;        /* Position encodings */
    int sequenceLength;    /* Current sequence length */
    int maxLength;         /* Maximum sequence length */
} TemporalContext;

/**
 * Create a cross-modal attention module
 * @param config Configuration parameters
 * @return Newly created attention module, or NULL on failure
 */
CrossModalAttention *hyperionCrossModalAttnCreate(const CrossModalAttnConfig *config);

/**
 * Free cross-modal attention module
 * @param attn Attention module to free
 */
void hyperionCrossModalAttnFree(CrossModalAttention *attn);

/**
 * Multi-head cross-modal attention computation
 * 
 * Computes attention between query modality and key/value modality
 * 
 * @param attn Cross-modal attention module
 * @param queryFeatures Query features (from one modality)
 * @param keyFeatures Key features (from another modality)  
 * @param valueFeatures Value features (same as key or different)
 * @param queryDim Dimension of query features
 * @param keyDim Dimension of key features
 * @param valueDim Dimension of value features
 * @param queryLen Length of query sequence
 * @param keyLen Length of key sequence
 * @param output Output buffer for attended features
 * @param outputDim Dimension of output features
 * @param mask Attention mask (optional, can be NULL)
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnCompute(CrossModalAttention *attn,
                                 const float *queryFeatures, const float *keyFeatures, 
                                 const float *valueFeatures,
                                 int queryDim, int keyDim, int valueDim,
                                 int queryLen, int keyLen,
                                 float *output, int outputDim,
                                 const AttentionMask *mask);

/**
 * Bidirectional cross-modal attention
 * 
 * Computes attention in both directions between two modalities
 * 
 * @param attn Cross-modal attention module
 * @param features1 Features from modality 1
 * @param features2 Features from modality 2
 * @param dim1 Dimension of modality 1 features
 * @param dim2 Dimension of modality 2 features
 * @param len1 Sequence length of modality 1
 * @param len2 Sequence length of modality 2
 * @param output1 Output for modality 1 (attended by modality 2)
 * @param output2 Output for modality 2 (attended by modality 1)
 * @param mask Attention mask (optional)
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnBidirectional(CrossModalAttention *attn,
                                       const float *features1, const float *features2,
                                       int dim1, int dim2, int len1, int len2,
                                       float *output1, float *output2,
                                       const AttentionMask *mask);

/**
 * Temporal cross-modal attention for video/sequential data
 * 
 * @param attn Cross-modal attention module
 * @param currentFeatures Current time step features
 * @param temporalContext Temporal context from previous steps
 * @param modalityFeatures Features from other modality
 * @param currentDim Dimension of current features
 * @param modalityDim Dimension of other modality features
 * @param modalityLen Length of other modality sequence
 * @param output Output attended features
 * @param outputDim Output dimension
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnTemporal(CrossModalAttention *attn,
                                  const float *currentFeatures,
                                  const TemporalContext *temporalContext,
                                  const float *modalityFeatures,
                                  int currentDim, int modalityDim, int modalityLen,
                                  float *output, int outputDim);

/**
 * Hierarchical cross-modal attention
 * 
 * Performs attention at multiple levels of granularity
 * 
 * @param attn Cross-modal attention module
 * @param queryFeatures Query features at different levels
 * @param keyFeatures Key features at different levels
 * @param numLevels Number of hierarchical levels
 * @param dimensions Dimensions at each level
 * @param lengths Sequence lengths at each level
 * @param output Output fused features
 * @param outputDim Output dimension
 * @param levelWeights Weights for combining different levels (optional)
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnHierarchical(CrossModalAttention *attn,
                                       const float **queryFeatures,
                                       const float **keyFeatures,
                                       int numLevels, const int *dimensions,
                                       const int *lengths,
                                       float *output, int outputDim,
                                       const float *levelWeights);

/**
 * Create attention mask
 * @param rows Number of rows (query length)
 * @param cols Number of columns (key length)
 * @return Newly created attention mask, or NULL on failure
 */
AttentionMask *hyperionAttnMaskCreate(int rows, int cols);

/**
 * Free attention mask
 * @param mask Attention mask to free
 */
void hyperionAttnMaskFree(AttentionMask *mask);

/**
 * Set causal mask (lower triangular)
 * @param mask Attention mask to modify
 * @return true on success, false on failure
 */
bool hyperionAttnMaskSetCausal(AttentionMask *mask);

/**
 * Set padding mask
 * @param mask Attention mask to modify
 * @param paddingPositions Array of padding positions to mask
 * @param numPadding Number of padding positions
 * @return true on success, false on failure
 */
bool hyperionAttnMaskSetPadding(AttentionMask *mask, const int *paddingPositions, int numPadding);

/**
 * Create temporal context
 * @param maxLength Maximum sequence length
 * @param hiddenDim Dimension of hidden states
 * @return Newly created temporal context, or NULL on failure
 */
TemporalContext *hyperionTemporalContextCreate(int maxLength, int hiddenDim);

/**
 * Free temporal context
 * @param context Temporal context to free
 */
void hyperionTemporalContextFree(TemporalContext *context);

/**
 * Update temporal context with new time step
 * @param context Temporal context to update
 * @param newHidden New hidden state
 * @param position Position encoding
 * @param hiddenDim Dimension of hidden state
 * @return true on success, false on failure
 */
bool hyperionTemporalContextUpdate(TemporalContext *context, const float *newHidden,
                                 int position, int hiddenDim);

/**
 * Get memory usage of cross-modal attention module
 * @param attn Cross-modal attention module
 * @param weightMemory Output for weight memory usage (bytes)
 * @param activationMemory Output for activation memory usage (bytes)
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnGetMemoryUsage(const CrossModalAttention *attn,
                                        size_t *weightMemory, size_t *activationMemory);

/**
 * Enable/disable SIMD acceleration
 * @param attn Cross-modal attention module
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnEnableSIMD(CrossModalAttention *attn, bool enable);

/**
 * Set quantization for weights
 * @param attn Cross-modal attention module
 * @param enable Whether to enable quantization
 * @return true on success, false on failure
 */
bool hyperionCrossModalAttnSetQuantization(CrossModalAttention *attn, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_CROSS_MODAL_ATTENTION_H */