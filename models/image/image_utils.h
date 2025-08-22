/**
 * @file image_utils.h
 * @brief Image utility functions for Hyperion
 */

#ifndef HYPERION_IMAGE_UTILS_H
#define HYPERION_IMAGE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declarations for types */
#ifndef HYPERION_IMAGE_MODEL_H
#include "image_model.h"
#else

/* Forward declarations if image_model.h is already included */
typedef enum HyperionImageFormat             HyperionImageFormat;
typedef struct HyperionImage                 HyperionImage;
typedef struct HyperionImagePreprocessParams HyperionImagePreprocessParams;

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new image
 * @param width Image width
 * @param height Image height
 * @param format Image format
 * @return Newly allocated image, or NULL on failure
 */
HyperionImage *hyperionImageCreate(int width, int height, HyperionImageFormat format);

/**
 * Free an image
 * @param image The image to free
 */
void hyperionImageFree(HyperionImage *image);

/**
 * Create a copy of an image
 * @param image The image to copy
 * @return Newly allocated copy of image, or NULL on failure
 */
HyperionImage *hyperionImageCopy(const HyperionImage *image);

/**
 * Resize an image using bilinear interpolation
 * @param image The image to resize
 * @param newWidth New width
 * @param newHeight New height
 * @return Newly allocated resized image, or NULL on failure
 */
HyperionImage *hyperionImageResize(const HyperionImage *image, int newWidth, int newHeight);

/**
 * Convert image to grayscale
 * @param image The image to convert
 * @return Newly allocated grayscale image, or NULL on failure
 */
HyperionImage *hyperionImageToGrayscale(const HyperionImage *image);

/**
 * Convert image to float array
 * @param image The image to convert
 * @param output Float array to store result (must be pre-allocated)
 * @param normalize Whether to normalize to 0-1 range
 * @return true on success, false on failure
 */
bool hyperionImageToFloatArray(const HyperionImage *image, float *output, bool normalize);

/**
 * Set default preprocessing parameters
 * @param params The parameters structure to initialize
 */
void hyperionImagePreprocessParamsDefault(HyperionImagePreprocessParams *params);

/**
 * Preprocess an image for model input
 * @param image The image to preprocess
 * @param params Preprocessing parameters
 * @return Newly allocated preprocessed image, or NULL on failure
 */
HyperionImage *hyperionImagePreprocess(const HyperionImage                 *image,
                                   const HyperionImagePreprocessParams *params);

/**
 * Load an image from a file
 * @param filepath Path to the image file
 * @return Newly allocated image, or NULL on failure
 */
HyperionImage *hyperionImageLoadFromFile(const char *filepath);

/**
 * Save an image to a file
 * @param image The image to save
 * @param filepath Path where to save the image
 * @param format Output format (jpg, png, bmp, tga)
 * @return true on success, false on failure
 */
bool hyperionImageSaveToFile(const HyperionImage *image, const char *filepath, const char *format);

/**
 * Apply a simple convolution filter to an image
 * @param image The image to filter
 * @param kernel The convolution kernel
 * @param kernelSize The size of the kernel (must be odd)
 * @return Newly allocated filtered image, or NULL on failure
 */
HyperionImage *hyperionImageApplyFilter(const HyperionImage *image, const float *kernel, int kernelSize);

/**
 * Extract a crop from an image
 * @param image The source image
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of crop
 * @param height Height of crop
 * @return Newly allocated cropped image, or NULL on failure
 */
HyperionImage *hyperionImageCrop(const HyperionImage *image, int x, int y, int width, int height);

/**
 * Rotate an image
 * @param image The image to rotate
 * @param angleDegrees Rotation angle in degrees
 * @return Newly allocated rotated image, or NULL on failure
 */
HyperionImage *hyperionImageRotate(const HyperionImage *image, float angleDegrees);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_IMAGE_UTILS_H */
