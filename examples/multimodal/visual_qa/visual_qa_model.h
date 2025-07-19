/**
 * @file visual_qa_model.h
 * @brief Header for visual question answering using multimodal model in Hyperion
 */

#ifndef TINYAI_VISUAL_QA_MODEL_H
#define TINYAI_VISUAL_QA_MODEL_H

#include "../../../models/multimodal/multimodal_model.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Visual QA model configuration
 */
typedef struct {
    int   imageWidth;        /* Input image width */
    int   imageHeight;       /* Input image height */
    int   maxQuestionLength; /* Maximum token length for question */
    int   maxAnswerLength;   /* Maximum token length for answer */
    int   textEmbedDim;      /* Text embedding dimension */
    int   imageFeatureDim;   /* Image feature dimension */
    int   fusionDim;         /* Dimension of fused representation */
    bool  useQuantization;   /* Whether to use 4-bit quantization */
    bool  useSIMD;           /* Whether to use SIMD acceleration */
    char *weightsFile;       /* Path to weights file */
    char *vocabFile;         /* Path to vocabulary file */
} HyperionVisualQAConfig;

/**
 * Visual QA model handle
 */
typedef struct HyperionVisualQAModel HyperionVisualQAModel;

/**
 * Create a visual question answering model
 *
 * @param config Configuration for the model
 * @return Model handle, or NULL on failure
 */
HyperionVisualQAModel *hyperionVisualQAModelCreate(const HyperionVisualQAConfig *config);

/**
 * Free a visual question answering model
 *
 * @param model Model to free
 */
void hyperionVisualQAModelFree(HyperionVisualQAModel *model);

/**
 * Answer a question about an image
 *
 * @param model Model to use
 * @param imagePath Path to the image file
 * @param question Question text
 * @param answer Buffer to store the generated answer
 * @param maxLength Maximum length of the answer buffer
 * @return true on success, false on failure
 */
bool hyperionVisualQAGenerateAnswer(HyperionVisualQAModel *model, const char *imagePath,
                                  const char *question, char *answer, int maxLength);

/**
 * Answer a question about an image directly from image data
 *
 * @param model Model to use
 * @param image Image to analyze
 * @param question Question text
 * @param answer Buffer to store the generated answer
 * @param maxLength Maximum length of the answer buffer
 * @return true on success, false on failure
 */
bool hyperionVisualQAGenerateAnswerFromImage(HyperionVisualQAModel *model, const HyperionImage *image,
                                           const char *question, char *answer, int maxLength);

/**
 * Get model's memory usage statistics
 *
 * @param model Model to query
 * @param weightMemory Output parameter for weight memory (in bytes)
 * @param activationMemory Output parameter for activation memory (in bytes)
 * @return true on success, false on failure
 */
bool hyperionVisualQAModelGetMemoryUsage(const HyperionVisualQAModel *model, size_t *weightMemory,
                                       size_t *activationMemory);

/**
 * Enable or disable SIMD acceleration
 *
 * @param model Model to configure
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionVisualQAModelEnableSIMD(HyperionVisualQAModel *model, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_VISUAL_QA_MODEL_H */
