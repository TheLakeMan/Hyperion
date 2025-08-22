/**
 * @file model_loader.h
 * @brief Header for model weight loading utilities for Hyperion
 */

#ifndef HYPERION_MODEL_LOADER_H
#define HYPERION_MODEL_LOADER_H

#include "../models/image/image_model.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load model weights from a file
 * @param model The model to load weights into
 * @param filepath Path to the weights file
 * @param convertPrecision Whether to convert between precisions if needed
 * @return true on success, false on failure
 */
bool hyperionLoadModelWeights(HyperionImageModel *model, const char *filepath, bool convertPrecision);

/**
 * Save model weights to a file
 * @param model The model to save weights from
 * @param filepath Path to save the weights to
 * @return true on success, false on failure
 */
bool hyperionSaveModelWeights(HyperionImageModel *model, const char *filepath);

/**
 * Generate a Hyperion model weight file from a standard format model (e.g., ONNX, TFLite)
 * @param srcFilepath Source model file path
 * @param destFilepath Destination Hyperion weight file path
 * @param modelType Target model type
 * @param quantize Whether to quantize weights to 4-bit
 * @return true on success, false on failure
 */
bool hyperionConvertModelWeights(const char *srcFilepath, const char *destFilepath,
                               HyperionImageModelType modelType, bool quantize);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HYPERION_MODEL_LOADER_H */