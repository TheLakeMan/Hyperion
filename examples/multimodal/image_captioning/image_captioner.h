/**
 * @file image_captioner.h
 * @brief Header for multimodal image captioning in Hyperion
 */

#ifndef TINYAI_IMAGE_CAPTIONER_H
#define TINYAI_IMAGE_CAPTIONER_H

#include "../../../models/image/image_model.h"
#include "../../../models/multimodal/multimodal_model.h"
#include "../../../models/text/generate.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Fusion method for combining image and text features
 */
typedef enum {
    TINYAI_FUSION_CONCATENATION,  /* Concatenate features */
    TINYAI_FUSION_ADDITION,       /* Add features */
    TINYAI_FUSION_MULTIPLICATION, /* Multiply features */
    TINYAI_FUSION_ATTENTION       /* Use attention mechanism */
} HyperionFusionMethod;

/**
 * Image captioner configuration
 */
typedef struct {
    /* Model paths */
    const char *visionModelPath;     /* Path to vision model structure */
    const char *visionWeightsPath;   /* Path to vision model weights */
    const char *languageModelPath;   /* Path to language model structure */
    const char *languageWeightsPath; /* Path to language model weights */
    const char *tokenizerPath;       /* Path to tokenizer vocabulary */

    /* Generation parameters */
    int   maxTokens;   /* Maximum tokens in caption */
    int   beamWidth;   /* Beam search width (1 for greedy) */
    float temperature; /* Sampling temperature */

    /* Fusion settings */
    HyperionFusionMethod fusionMethod; /* Method to fuse image and text features */

    /* Optimization options */
    bool useQuantization; /* Whether to use quantization */
    bool useSIMD;         /* Whether to use SIMD acceleration */
} HyperionCaptionerConfig;

/**
 * Image captioner
 */
typedef struct HyperionImageCaptioner HyperionImageCaptioner;

/**
 * Callback for streaming text generation
 * @param token The generated token text
 * @param is_partial Whether this is a partial UTF-8 character
 * @param user_data User-provided data pointer
 * @return True to continue generation, false to stop
 */
typedef bool (*HyperionCaptionerStreamCallback)(const char *token, bool is_partial, void *user_data);

/**
 * Create a new image captioner
 *
 * @param config Captioner configuration
 * @return New captioner or NULL on error
 */
HyperionImageCaptioner *hyperionCaptionerCreate(const HyperionCaptionerConfig *config);

/**
 * Free an image captioner
 *
 * @param captioner Captioner to free
 */
void hyperionCaptionerFree(HyperionImageCaptioner *captioner);

/**
 * Generate a caption for an image file
 *
 * @param captioner Captioner to use
 * @param imagePath Path to image file
 * @param stream_callback Callback for streaming tokens (NULL for non-streaming)
 * @param user_data User data to pass to the callback
 * @return Generated caption (caller must free) or NULL on error
 */
char *hyperionGenerateCaption(HyperionImageCaptioner *captioner, const char *imagePath,
                            HyperionCaptionerStreamCallback stream_callback, void *user_data);

/**
 * Generate a caption for an in-memory image
 *
 * @param captioner Captioner to use
 * @param image Image data
 * @param stream_callback Callback for streaming tokens (NULL for non-streaming)
 * @param user_data User data to pass to the callback
 * @return Generated caption (caller must free) or NULL on error
 */
char *hyperionGenerateCaptionFromImage(HyperionImageCaptioner *captioner, const HyperionImage *image,
                                     HyperionCaptionerStreamCallback stream_callback,
                                     void                         *user_data);

/**
 * Get the encoding time for the vision model in milliseconds
 *
 * @param captioner Captioner
 * @return Encoding time in milliseconds or -1 if not available
 */
double hyperionCaptionerGetVisionEncodingTime(const HyperionImageCaptioner *captioner);

/**
 * Get the generation time for the text model in milliseconds
 *
 * @param captioner Captioner
 * @return Generation time in milliseconds or -1 if not available
 */
double hyperionCaptionerGetTextGenerationTime(const HyperionImageCaptioner *captioner);

/**
 * Get memory usage statistics
 *
 * @param captioner Captioner
 * @param visionModelMemory Output parameter for vision model memory (in bytes)
 * @param languageModelMemory Output parameter for language model memory (in bytes)
 * @param totalMemory Output parameter for total memory (in bytes)
 * @return True on success, false on failure
 */
bool hyperionCaptionerGetMemoryUsage(const HyperionImageCaptioner *captioner, size_t *visionModelMemory,
                                   size_t *languageModelMemory, size_t *totalMemory);

/**
 * Set fusion method for image and text features
 *
 * @param captioner Captioner
 * @param method Fusion method to use
 * @return True on success, false on failure
 */
bool hyperionCaptionerSetFusionMethod(HyperionImageCaptioner *captioner, HyperionFusionMethod method);

/**
 * Set generation parameters
 *
 * @param captioner Captioner
 * @param temperature Sampling temperature (0.0-1.5)
 * @param maxTokens Maximum tokens to generate
 * @param beamWidth Beam search width (1 for greedy search)
 * @return True on success, false on failure
 */
bool hyperionCaptionerSetGenerationParams(HyperionImageCaptioner *captioner, float temperature,
                                        int maxTokens, int beamWidth);

/**
 * Enable or disable SIMD acceleration
 *
 * @param captioner Captioner
 * @param enable Whether to enable SIMD
 * @return True on success, false on failure
 */
bool hyperionCaptionerEnableSIMD(HyperionImageCaptioner *captioner, bool enable);

/**
 * Convert fusion method string to enum
 *
 * @param methodStr String representation of fusion method
 * @return Fusion method enum value or TINYAI_FUSION_ATTENTION if not recognized
 */
HyperionFusionMethod hyperionGetFusionMethodFromString(const char *methodStr);

#ifdef __cplusplus
}
#endif

#endif /* TINYAI_IMAGE_CAPTIONER_H */
