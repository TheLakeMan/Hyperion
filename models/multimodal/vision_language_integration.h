/**
 * @file vision_language_integration.h
 * @brief Enhanced vision-language integration for Hyperion multimodal models
 *
 * This module provides sophisticated integration between visual and textual
 * modalities, including region-aware attention, spatial reasoning, and
 * hierarchical visual understanding for tasks like image captioning,
 * visual question answering, and visual reasoning.
 */

#ifndef HYPERION_VISION_LANGUAGE_INTEGRATION_H
#define HYPERION_VISION_LANGUAGE_INTEGRATION_H

#include "cross_modal_attention.h"
#include "../image/image_model.h"
#include "../text/generate.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Visual attention region
 */
typedef struct {
    int x, y, width, height;  /* Bounding box coordinates */
    float confidence;         /* Confidence score for region */
    int featureIndex;        /* Index into feature map */
} VisualRegion;

/**
 * Spatial relationship types
 */
typedef enum {
    SPATIAL_LEFT_OF,
    SPATIAL_RIGHT_OF,
    SPATIAL_ABOVE,
    SPATIAL_BELOW,
    SPATIAL_INSIDE,
    SPATIAL_CONTAINS,
    SPATIAL_ADJACENT,
    SPATIAL_OVERLAPS
} SpatialRelation;

/**
 * Visual reasoning context
 */
typedef struct {
    VisualRegion *regions;    /* Detected visual regions */
    int numRegions;          /* Number of regions */
    float *regionFeatures;   /* Features for each region */
    int featureDim;         /* Dimension of region features */
    SpatialRelation *relations; /* Spatial relationships between regions */
    int numRelations;       /* Number of relationships */
} VisualReasoningContext;

/**
 * Vision-language integration module
 */
typedef struct VisionLanguageIntegration VisionLanguageIntegration;

/**
 * Configuration for vision-language integration
 */
typedef struct {
    int visualFeatureDim;     /* Dimension of visual features */
    int textFeatureDim;       /* Dimension of text features */
    int fusedDim;            /* Dimension of fused representation */
    int maxRegions;          /* Maximum number of visual regions */
    int maxTextLength;       /* Maximum text sequence length */
    bool useRegionAttention; /* Whether to use region-level attention */
    bool useSpatialReasoning; /* Whether to incorporate spatial relationships */
    bool useHierarchical;    /* Whether to use hierarchical features */
    CrossModalAttnConfig attnConfig; /* Cross-modal attention configuration */
} VisionLanguageConfig;

/**
 * Create vision-language integration module
 * @param config Configuration parameters
 * @return Newly created module, or NULL on failure
 */
VisionLanguageIntegration *hyperionVisionLanguageCreate(const VisionLanguageConfig *config);

/**
 * Free vision-language integration module
 * @param vl Module to free
 */
void hyperionVisionLanguageFree(VisionLanguageIntegration *vl);

/**
 * Enhanced image captioning with region awareness
 * 
 * @param vl Vision-language module
 * @param imageFeatures Global image features
 * @param regionContext Visual reasoning context with regions
 * @param previousTokens Previous generated tokens (for continuation)
 * @param numPrevTokens Number of previous tokens
 * @param outputTokens Output buffer for generated tokens
 * @param maxOutputTokens Maximum number of tokens to generate
 * @param temperature Sampling temperature
 * @return Number of tokens generated, or -1 on failure
 */
int hyperionVisionLanguageCaptionAdvanced(VisionLanguageIntegration *vl,
                                        const float *imageFeatures,
                                        const VisualReasoningContext *regionContext,
                                        const int *previousTokens, int numPrevTokens,
                                        int *outputTokens, int maxOutputTokens,
                                        float temperature);

/**
 * Visual question answering with spatial reasoning
 * 
 * @param vl Vision-language module
 * @param imageFeatures Global image features
 * @param regionContext Visual reasoning context
 * @param questionTokens Question tokens
 * @param numQuestionTokens Number of question tokens
 * @param answerTokens Output buffer for answer tokens
 * @param maxAnswerTokens Maximum answer length
 * @param confidence Output confidence score
 * @return Number of answer tokens generated, or -1 on failure
 */
int hyperionVisionLanguageVQA(VisionLanguageIntegration *vl,
                             const float *imageFeatures,
                             const VisualReasoningContext *regionContext,
                             const int *questionTokens, int numQuestionTokens,
                             int *answerTokens, int maxAnswerTokens,
                             float *confidence);

/**
 * Region-aware visual grounding
 * 
 * Links text spans to visual regions in the image
 * 
 * @param vl Vision-language module
 * @param imageFeatures Global image features
 * @param regionContext Visual reasoning context
 * @param textTokens Input text tokens
 * @param numTextTokens Number of text tokens
 * @param groundingScores Output grounding scores [numTextTokens x numRegions]
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageGrounding(VisionLanguageIntegration *vl,
                                   const float *imageFeatures,
                                   const VisualReasoningContext *regionContext,
                                   const int *textTokens, int numTextTokens,
                                   float *groundingScores);

/**
 * Spatial relationship reasoning
 * 
 * @param vl Vision-language module
 * @param regionContext Visual reasoning context
 * @param queryRegion1 Index of first region
 * @param queryRegion2 Index of second region
 * @param relationScores Output scores for each spatial relation type
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageSpatialReasoning(VisionLanguageIntegration *vl,
                                          const VisualReasoningContext *regionContext,
                                          int queryRegion1, int queryRegion2,
                                          float *relationScores);

/**
 * Hierarchical visual understanding
 * 
 * Processes visual information at multiple scales/levels
 * 
 * @param vl Vision-language module
 * @param pyramidFeatures Features at multiple scales [numLevels][featureDim]
 * @param numLevels Number of pyramid levels
 * @param levelDims Feature dimensions at each level
 * @param textTokens Input text tokens
 * @param numTextTokens Number of text tokens
 * @param fusedOutput Output fused representation
 * @param outputDim Dimension of output
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageHierarchical(VisionLanguageIntegration *vl,
                                       const float **pyramidFeatures, int numLevels,
                                       const int *levelDims,
                                       const int *textTokens, int numTextTokens,
                                       float *fusedOutput, int outputDim);

/**
 * Create visual reasoning context
 * @param maxRegions Maximum number of regions
 * @param featureDim Dimension of region features
 * @return Newly created context, or NULL on failure
 */
VisualReasoningContext *hyperionVisualReasoningContextCreate(int maxRegions, int featureDim);

/**
 * Free visual reasoning context
 * @param context Context to free
 */
void hyperionVisualReasoningContextFree(VisualReasoningContext *context);

/**
 * Add region to visual reasoning context
 * @param context Visual reasoning context
 * @param region Region to add
 * @param features Features for the region
 * @return true on success, false on failure
 */
bool hyperionVisualReasoningContextAddRegion(VisualReasoningContext *context,
                                           const VisualRegion *region,
                                           const float *features);

/**
 * Compute spatial relationships between regions
 * @param context Visual reasoning context
 * @return true on success, false on failure
 */
bool hyperionVisualReasoningContextComputeRelations(VisualReasoningContext *context);

/**
 * Visual attention pooling over regions
 * 
 * @param vl Vision-language module
 * @param regionFeatures Features for all regions [numRegions x featureDim]
 * @param textFeatures Text features for attention
 * @param numRegions Number of regions
 * @param regionFeatureDim Dimension of region features
 * @param textFeatureDim Dimension of text features
 * @param pooledFeatures Output pooled features
 * @param pooledDim Dimension of pooled features
 * @param attentionWeights Output attention weights (optional)
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageVisualAttentionPool(VisionLanguageIntegration *vl,
                                             const float *regionFeatures,
                                             const float *textFeatures,
                                             int numRegions, int regionFeatureDim,
                                             int textFeatureDim,
                                             float *pooledFeatures, int pooledDim,
                                             float *attentionWeights);

/**
 * Text-guided visual attention
 * 
 * @param vl Vision-language module
 * @param imageFeatures Spatial image features [height x width x channels]
 * @param height Image height
 * @param width Image width
 * @param channels Number of channels
 * @param textTokens Text tokens for guidance
 * @param numTextTokens Number of text tokens
 * @param attentionMap Output spatial attention map [height x width]
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageTextGuidedAttention(VisionLanguageIntegration *vl,
                                             const float *imageFeatures,
                                             int height, int width, int channels,
                                             const int *textTokens, int numTextTokens,
                                             float *attentionMap);

/**
 * Get memory usage of vision-language module
 * @param vl Vision-language module
 * @param weightMemory Output for weight memory usage
 * @param activationMemory Output for activation memory usage
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageGetMemoryUsage(const VisionLanguageIntegration *vl,
                                        size_t *weightMemory, size_t *activationMemory);

/**
 * Enable/disable SIMD acceleration
 * @param vl Vision-language module
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageEnableSIMD(VisionLanguageIntegration *vl, bool enable);

/**
 * Set quantization for weights
 * @param vl Vision-language module
 * @param enable Whether to enable quantization
 * @return true on success, false on failure
 */
bool hyperionVisionLanguageSetQuantization(VisionLanguageIntegration *vl, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_VISION_LANGUAGE_INTEGRATION_H */