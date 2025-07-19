/**
 * @file image_model.h
 * @brief Public API for image model functionality in Hyperion
 */

#ifndef HYPERION_IMAGE_MODEL_H
#define HYPERION_IMAGE_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Image format enumeration
 */
typedef enum {
    HYPERION_IMAGE_FORMAT_GRAYSCALE,
    HYPERION_IMAGE_FORMAT_RGB,
    HYPERION_IMAGE_FORMAT_BGR,
    HYPERION_IMAGE_FORMAT_RGBA
} HyperionImageFormat;

/**
 * Image model type enumeration
 */
typedef enum {
    HYPERION_IMAGE_MODEL_TINY_CNN,
    HYPERION_IMAGE_MODEL_MOBILENET,
    HYPERION_IMAGE_MODEL_EFFICIENTNET,
    HYPERION_IMAGE_MODEL_CUSTOM
} HyperionImageModelType;

/**
 * Image structure
 */
typedef struct {
    int               width;    /* Image width */
    int               height;   /* Image height */
    HyperionImageFormat format;     /* Image format */
    uint8_t          *data;     /* Pixel data */
    bool              ownsData; /* Whether we own the data (and should free it) */
} HyperionImage;

/**
 * Image classification result
 */
typedef struct {
    int         classId;    /* Class ID */
    float       confidence; /* Confidence score (0-1) */
    const char *label;      /* Class label (if available) */
} HyperionImageClassResult;

/**
 * Image preprocessing parameters
 */
typedef struct {
    int   targetWidth;  /* Target width for resizing */
    int   targetHeight; /* Target height for resizing */
    float meanR;        /* Mean value for red channel */
    float meanG;        /* Mean value for green channel */
    float meanB;        /* Mean value for blue channel */
    float stdR;         /* Standard deviation for red channel */
    float stdG;         /* Standard deviation for green channel */
    float stdB;         /* Standard deviation for blue channel */
    bool  centerCrop;   /* Whether to center crop */
    float cropRatio;    /* Ratio for center cropping */
} HyperionImagePreprocessParams;

/**
 * Image model parameters for creation
 */
typedef struct {
    HyperionImageModelType modelType;       /* Type of model to create */
    int                  inputWidth;      /* Input width */
    int                  inputHeight;     /* Input height */
    int                  inputChannels;   /* Input channels */
    int                  numClasses;      /* Number of output classes */
    const char          *weightsFile;     /* Path to weights file (optional) */
    const char          *labelsFile;      /* Path to labels file (optional) */
    bool                 useQuantization; /* Whether to use 4-bit quantization */
    bool                 useSIMD;         /* Whether to use SIMD acceleration */
    void                *customParams;    /* Custom parameters (for CUSTOM model type) */
} HyperionImageModelParams;

/**
 * Forward declaration of image model struct
 */
typedef struct HyperionImageModel HyperionImageModel;

/**
 * Create an image model
 * @param params Parameters for model creation
 * @return Newly allocated model, or NULL on failure
 */
HyperionImageModel *hyperionImageModelCreate(const HyperionImageModelParams *params);

/**
 * Free an image model
 * @param model The model to free
 */
void hyperionImageModelFree(HyperionImageModel *model);

/**
 * Classify an image
 * @param model The model to use for classification
 * @param image The image to classify
 * @param topK Number of top results to return
 * @param results Array to store results (must be pre-allocated for topK results)
 * @return Number of results on success, negative on failure
 */
int hyperionImageModelClassify(HyperionImageModel *model, const HyperionImage *image, int topK,
                             HyperionImageClassResult *results);

/**
 * Set custom memory pool for model
 * @param model The model to set memory pool for
 * @param memoryPool Memory pool to use
 * @return true on success, false on failure
 */
bool hyperionImageModelSetMemoryPool(HyperionImageModel *model, void *memoryPool);

/**
 * Enable or disable SIMD acceleration
 * @param model The model to configure
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionImageModelEnableSIMD(HyperionImageModel *model, bool enable);

/**
 * Get memory usage statistics
 * @param model The model to query
 * @param weightMemory Output parameter for weight memory (in bytes)
 * @param activationMemory Output parameter for activation memory (in bytes)
 * @return true on success, false on failure
 */
bool hyperionImageModelGetMemoryUsage(const HyperionImageModel *model, size_t *weightMemory,
                                    size_t *activationMemory);

/**
 * Get preprocessing parameters
 * @param model The model to query
 * @param params Output parameter for preprocessing parameters
 * @return true on success, false on failure
 */
bool hyperionImageModelGetPreprocessParams(const HyperionImageModel      *model,
                                         HyperionImagePreprocessParams *params);

/**
 * Print model summary
 * @param model The model to print summary for
 */
void hyperionImageModelPrintSummary(const HyperionImageModel *model);

/**
 * Get the number of weights in the model
 * @param model The model to query
 * @return Number of weights
 */
size_t hyperionImageModelGetNumWeights(const HyperionImageModel *model);

/**
 * Get the number of operations per forward pass
 * @param model The model to query
 * @return Number of operations
 */
size_t hyperionImageModelGetNumOperations(const HyperionImageModel *model);

#ifdef __cplusplus
}
#endif

/*
 * Include image utilities
 * This header contains declarations for functions such as:
 * - hyperionImageCreate
 * - hyperionImageFree
 * - hyperionImageCopy
 * - hyperionImageResize
 * and other image manipulation functions
 */
#include "image_utils.h"

#endif /* HYPERION_IMAGE_MODEL_H */