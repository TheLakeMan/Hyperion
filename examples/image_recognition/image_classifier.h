/**
 * @file image_classifier.h
 * @brief Header for image classification in Hyperion
 */

#ifndef TINYAI_IMAGE_CLASSIFIER_H
#define TINYAI_IMAGE_CLASSIFIER_H

#include "../../models/image/image_model.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Prediction result structure
 */
typedef struct {
    int   class_id;   /* Class index */
    float confidence; /* Confidence score (0-1) */
    char *label;      /* Class label (owned by classifier) */
} HyperionPrediction;

/**
 * Image classifier configuration
 */
typedef struct {
    /* Model paths */
    const char *modelPath;   /* Path to model structure */
    const char *weightsPath; /* Path to model weights */
    const char *labelsPath;  /* Path to class labels file */

    /* Classification parameters */
    int   inputSize;           /* Input image size in pixels */
    int   topK;                /* Number of top predictions to return */
    float confidenceThreshold; /* Minimum confidence for predictions */

    /* Optimization options */
    bool useQuantization; /* Whether to use quantization */
    bool useSIMD;         /* Whether to use SIMD acceleration */
} HyperionClassifierConfig;

/**
 * Image classifier
 */
typedef struct HyperionImageClassifier HyperionImageClassifier;

/**
 * Create a new image classifier
 *
 * @param config Classifier configuration
 * @return New classifier or NULL on error
 */
HyperionImageClassifier *hyperionClassifierCreate(const HyperionClassifierConfig *config);

/**
 * Free an image classifier
 *
 * @param classifier Classifier to free
 */
void hyperionClassifierFree(HyperionImageClassifier *classifier);

/**
 * Classify an image file
 *
 * @param classifier Classifier to use
 * @param imagePath Path to image file
 * @param predictions Output array to store predictions
 * @param maxPredictions Maximum number of predictions to store
 * @param numPredictions Output parameter for number of predictions stored
 * @return True on success, false on failure
 */
bool hyperionClassifyImage(HyperionImageClassifier *classifier, const char *imagePath,
                         HyperionPrediction *predictions, int maxPredictions, int *numPredictions);

/**
 * Classify an in-memory image
 *
 * @param classifier Classifier to use
 * @param image Image data
 * @param predictions Output array to store predictions
 * @param maxPredictions Maximum number of predictions to store
 * @param numPredictions Output parameter for number of predictions stored
 * @return True on success, false on failure
 */
bool hyperionClassifyImageData(HyperionImageClassifier *classifier, const HyperionImage *image,
                             HyperionPrediction *predictions, int maxPredictions,
                             int *numPredictions);

/**
 * Get the last inference time in milliseconds
 *
 * @param classifier Classifier
 * @return Inference time in milliseconds or -1 if not available
 */
double hyperionClassifierGetInferenceTime(const HyperionImageClassifier *classifier);

/**
 * Get memory usage statistics
 *
 * @param classifier Classifier
 * @param modelMemory Output parameter for model memory (in bytes)
 * @param totalMemory Output parameter for total memory (in bytes)
 * @return True on success, false on failure
 */
bool hyperionClassifierGetMemoryUsage(const HyperionImageClassifier *classifier, size_t *modelMemory,
                                    size_t *totalMemory);

/**
 * Format prediction as a string
 *
 * @param prediction Prediction to format
 * @param buffer Output buffer to store formatted string
 * @param bufferSize Size of output buffer
 * @return True on success, false on failure
 */
bool hyperionFormatPrediction(const HyperionPrediction *prediction, char *buffer, size_t bufferSize);

/**
 * Enable or disable SIMD acceleration
 *
 * @param classifier Classifier
 * @param enable Whether to enable SIMD
 * @return True on success, false on failure
 */
bool hyperionClassifierEnableSIMD(HyperionImageClassifier *classifier, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_IMAGE_CLASSIFIER_H */
