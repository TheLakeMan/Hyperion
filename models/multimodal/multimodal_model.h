/**
 * @file multimodal_model.h
 * @brief Public API for multimodal model functionality in Hyperion
 *
 * This header defines the public API for multimodal models in Hyperion,
 * which can process inputs from multiple modalities (text, image, etc.)
 * and produce outputs that combine information from all modalities.
 */

#ifndef HYPERION_MULTIMODAL_MODEL_H
#define HYPERION_MULTIMODAL_MODEL_H

#include "../image/image_model.h"
#include "../text/generate.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Multimodal model type enumeration
 */
typedef enum {
    HYPERION_MULTIMODAL_FUSION,     /* Simple fusion of modalities */
    HYPERION_MULTIMODAL_CROSS_ATTN, /* Cross-attention between modalities */
    HYPERION_MULTIMODAL_CUSTOM      /* Custom multimodal architecture */
} HyperionMultimodalModelType;

/**
 * Multimodal fusion method enumeration
 */
typedef enum {
    HYPERION_FUSION_CONCAT,   /* Concatenation of features */
    HYPERION_FUSION_ADD,      /* Addition of features */
    HYPERION_FUSION_MULTIPLY, /* Multiplication of features */
    HYPERION_FUSION_ATTENTION /* Attention-based fusion */
} HyperionFusionMethod;

/**
 * Supported input modalities
 */
typedef enum {
    HYPERION_MODALITY_TEXT,  /* Text modality */
    HYPERION_MODALITY_IMAGE, /* Image modality */
    HYPERION_MODALITY_AUDIO  /* Audio modality (future support) */
} HyperionModality;

/**
 * Input modality configuration
 */
typedef struct {
    HyperionModality modality; /* Type of modality */
    union {
        struct {
            int maxTokens; /* Maximum number of tokens for text */
            int embedDim;  /* Embedding dimension for text */
        } text;
        struct {
            int width;    /* Image width */
            int height;   /* Image height */
            int channels; /* Image channels */
        } image;
        struct {
            int sampleRate; /* Audio sample rate */
            int duration;   /* Audio duration in seconds */
        } audio;
    } config;
} HyperionModalityConfig;

/**
 * Forward declaration of multimodal model struct
 */
typedef struct HyperionMultimodalModel HyperionMultimodalModel;

/**
 * Multimodal input container
 */
typedef struct {
    void *textInput;  /* Text input (token IDs) */
    int   textLength; /* Number of tokens */

    HyperionImage *imageInput; /* Image input */

    void *audioInput;  /* Audio input (for future use) */
    int   audioLength; /* Audio length */
} HyperionMultimodalInput;

/**
 * Multimodal output container
 */
typedef struct {
    float *embeddings; /* Fused embeddings */
    int    embedDim;   /* Embedding dimension */
    int    length;     /* Number of embedding vectors */

    float *textLogits; /* Text output logits (if applicable) */
    int    vocabSize;  /* Size of vocabulary (if applicable) */

    float *imageFeatures; /* Image features (if applicable) */
    int    numClasses;    /* Number of image classes (if applicable) */
} HyperionMultimodalOutput;

/**
 * Multimodal model parameters for creation
 */
typedef struct {
    HyperionMultimodalModelType modelType;       /* Type of multimodal model */
    HyperionModalityConfig     *modalityConfigs; /* Array of modality configurations */
    int                       numModalities;   /* Number of modalities */
    HyperionFusionMethod        fusionMethod;    /* Method for fusing modalities */
    int                       fusionDim;       /* Dimension of fused representation */
    int                       numLayers;       /* Number of fusion layers */
    const char               *weightsFile;     /* Path to weights file (optional) */
    bool                      useQuantization; /* Whether to use 4-bit quantization */
    bool                      useSIMD;         /* Whether to use SIMD acceleration */
    void                     *customParams;    /* Custom parameters */
} HyperionMultimodalModelParams;

/**
 * Create a multimodal model
 * @param params Parameters for model creation
 * @return Newly allocated model, or NULL on failure
 */
HyperionMultimodalModel *hyperionMultimodalModelCreate(const HyperionMultimodalModelParams *params);

/**
 * Free a multimodal model
 * @param model The model to free
 */
void hyperionMultimodalModelFree(HyperionMultimodalModel *model);

/**
 * Process multimodal input
 * @param model The model to use
 * @param input Multimodal input containing different modalities
 * @param output Output structure to store results (must be pre-allocated)
 * @return true on success, false on failure
 */
bool hyperionMultimodalModelProcess(HyperionMultimodalModel *model, const HyperionMultimodalInput *input,
                                  HyperionMultimodalOutput *output);

/**
 * Initialize multimodal input
 * @param input Multimodal input structure to initialize
 * @return true on success, false on failure
 */
bool hyperionMultimodalInputInit(HyperionMultimodalInput *input);

/**
 * Free multimodal input
 * @param input Multimodal input to free
 * @param freeContents Whether to free the contained inputs
 */
void hyperionMultimodalInputFree(HyperionMultimodalInput *input, bool freeContents);

/**
 * Initialize multimodal output
 * @param output Multimodal output structure to initialize
 * @param embedDim Dimension of embeddings
 * @param length Number of embedding vectors
 * @param vocabSize Size of vocabulary (0 if not applicable)
 * @param numClasses Number of image classes (0 if not applicable)
 * @return true on success, false on failure
 */
bool hyperionMultimodalOutputInit(HyperionMultimodalOutput *output, int embedDim, int length,
                                int vocabSize, int numClasses);

/**
 * Free multimodal output
 * @param output Multimodal output to free
 */
void hyperionMultimodalOutputFree(HyperionMultimodalOutput *output);

/**
 * Set custom memory pool for model
 * @param model The model to set memory pool for
 * @param memoryPool Memory pool to use
 * @return true on success, false on failure
 */
bool hyperionMultimodalModelSetMemoryPool(HyperionMultimodalModel *model, void *memoryPool);

/**
 * Enable or disable SIMD acceleration
 * @param model The model to configure
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionMultimodalModelEnableSIMD(HyperionMultimodalModel *model, bool enable);

/**
 * Get memory usage statistics
 * @param model The model to query
 * @param weightMemory Output parameter for weight memory (in bytes)
 * @param activationMemory Output parameter for activation memory (in bytes)
 * @return true on success, false on failure
 */
bool hyperionMultimodalModelGetMemoryUsage(const HyperionMultimodalModel *model, size_t *weightMemory,
                                         size_t *activationMemory);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_MULTIMODAL_MODEL_H */