/**
 * Hyperion Image Model Tests
 */

#include "../models/image/image_model.h"
#include "../utils/benchmark.h"
#include "../utils/model_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic assertion helper (consistent with other test files)
#define ASSERT(condition, message)                                                                 \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Assertion Failed: %s (%s:%d)\n", message, __FILE__, __LINE__);        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

// Test creating and destroying image models
void test_model_create_destroy()
{
    printf("  Testing image model creation/destruction...\n");

    // Create a TinyCNN model
    HyperionImageModelParams params = {.modelType       = TINYAI_IMAGE_MODEL_TINY_CNN,
                                     .inputWidth      = 224,
                                     .inputHeight     = 224,
                                     .inputChannels   = 3,
                                     .numClasses      = 10,
                                     .weightsFile     = NULL,
                                     .labelsFile      = NULL,
                                     .useQuantization = true,
                                     .useSIMD         = true,
                                     .customParams    = NULL};

    HyperionImageModel *model = hyperionImageModelCreate(&params);
    ASSERT(model != NULL, "hyperionImageModelCreate() should return non-NULL");

    // Get memory usage stats
    size_t weightMemory, activationMemory;
    bool   result = hyperionImageModelGetMemoryUsage(model, &weightMemory, &activationMemory);
    ASSERT(result, "hyperionImageModelGetMemoryUsage() should succeed");
    ASSERT(weightMemory > 0, "Weight memory should be greater than 0");
    ASSERT(activationMemory > 0, "Activation memory should be greater than 0");

    hyperionImageModelFree(model);
    printf("    PASS\n");
}

// Test creating different model architectures
void test_different_architectures()
{
    printf("  Testing different model architectures...\n");

    // Common parameters
    const int inputWidth    = 224;
    const int inputHeight   = 224;
    const int inputChannels = 3;
    const int numClasses    = 1000;

    // Test TinyCNN
    HyperionImageModelParams tinyCnnParams = {.modelType       = TINYAI_IMAGE_MODEL_TINY_CNN,
                                            .inputWidth      = inputWidth,
                                            .inputHeight     = inputHeight,
                                            .inputChannels   = inputChannels,
                                            .numClasses      = numClasses,
                                            .weightsFile     = NULL,
                                            .labelsFile      = NULL,
                                            .useQuantization = true,
                                            .useSIMD         = true,
                                            .customParams    = NULL};

    HyperionImageModel *tinyCnnModel = hyperionImageModelCreate(&tinyCnnParams);
    ASSERT(tinyCnnModel != NULL, "TinyCNN model creation should succeed");
    hyperionImageModelFree(tinyCnnModel);

    // Test MobileNet
    HyperionImageModelParams mobileNetParams = {.modelType       = TINYAI_IMAGE_MODEL_MOBILENET,
                                              .inputWidth      = inputWidth,
                                              .inputHeight     = inputHeight,
                                              .inputChannels   = inputChannels,
                                              .numClasses      = numClasses,
                                              .weightsFile     = NULL,
                                              .labelsFile      = NULL,
                                              .useQuantization = true,
                                              .useSIMD         = true,
                                              .customParams    = NULL};

    HyperionImageModel *mobileNetModel = hyperionImageModelCreate(&mobileNetParams);
    ASSERT(mobileNetModel != NULL, "MobileNet model creation should succeed");
    hyperionImageModelFree(mobileNetModel);

    // Test EfficientNet
    HyperionImageModelParams efficientNetParams = {.modelType       = TINYAI_IMAGE_MODEL_EFFICIENTNET,
                                                 .inputWidth      = inputWidth,
                                                 .inputHeight     = inputHeight,
                                                 .inputChannels   = inputChannels,
                                                 .numClasses      = numClasses,
                                                 .weightsFile     = NULL,
                                                 .labelsFile      = NULL,
                                                 .useQuantization = true,
                                                 .useSIMD         = true,
                                                 .customParams    = NULL};

    HyperionImageModel *efficientNetModel = hyperionImageModelCreate(&efficientNetParams);
    ASSERT(efficientNetModel != NULL, "EfficientNet model creation should succeed");
    hyperionImageModelFree(efficientNetModel);

    printf("    PASS\n");
}

// Test model quantization options
void test_quantization_options()
{
    printf("  Testing model quantization options...\n");

    // Create models with and without quantization
    HyperionImageModelParams quantizedParams = {.modelType       = TINYAI_IMAGE_MODEL_TINY_CNN,
                                              .inputWidth      = 224,
                                              .inputHeight     = 224,
                                              .inputChannels   = 3,
                                              .numClasses      = 10,
                                              .weightsFile     = NULL,
                                              .labelsFile      = NULL,
                                              .useQuantization = true,
                                              .useSIMD         = false,
                                              .customParams    = NULL};

    HyperionImageModelParams fullPrecisionParams = {.modelType       = TINYAI_IMAGE_MODEL_TINY_CNN,
                                                  .inputWidth      = 224,
                                                  .inputHeight     = 224,
                                                  .inputChannels   = 3,
                                                  .numClasses      = 10,
                                                  .weightsFile     = NULL,
                                                  .labelsFile      = NULL,
                                                  .useQuantization = false,
                                                  .useSIMD         = false,
                                                  .customParams    = NULL};

    HyperionImageModel *quantizedModel = hyperionImageModelCreate(&quantizedParams);
    ASSERT(quantizedModel != NULL, "Quantized model creation should succeed");

    HyperionImageModel *fullPrecisionModel = hyperionImageModelCreate(&fullPrecisionParams);
    ASSERT(fullPrecisionModel != NULL, "Full precision model creation should succeed");

    // Compare memory usage
    size_t quantizedWeightMem, quantizedActivationMem;
    size_t fullWeightMem, fullActivationMem;

    hyperionImageModelGetMemoryUsage(quantizedModel, &quantizedWeightMem, &quantizedActivationMem);
    hyperionImageModelGetMemoryUsage(fullPrecisionModel, &fullWeightMem, &fullActivationMem);

    // Quantized model should use less memory for weights (about 1/8th for 4-bit vs 32-bit)
    // Allow some overhead, so check if it's at least half the size
    printf("    Quantized model weight memory: %zu bytes\n", quantizedWeightMem);
    printf("    Full precision model weight memory: %zu bytes\n", fullWeightMem);
    ASSERT(quantizedWeightMem * 2 < fullWeightMem,
           "Quantized model should use significantly less memory");

    hyperionImageModelFree(quantizedModel);
    hyperionImageModelFree(fullPrecisionModel);
    printf("    PASS\n");
}

// Create a simple test image with a gradient pattern
HyperionImage *create_test_image(int width, int height, HyperionImageFormat format)
{
    HyperionImage *image = hyperionImageCreate(width, height, format);
    if (!image)
        return NULL;

    // Determine number of channels
    int channels = 1;
    switch (format) {
    case TINYAI_IMAGE_FORMAT_GRAYSCALE:
        channels = 1;
        break;
    case TINYAI_IMAGE_FORMAT_RGB:
    case TINYAI_IMAGE_FORMAT_BGR:
        channels = 3;
        break;
    case TINYAI_IMAGE_FORMAT_RGBA:
        channels = 4;
        break;
    }

    // Fill with a gradient pattern
    uint8_t *data = (uint8_t *)image->data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Create a gradient based on position
            uint8_t r = (uint8_t)((float)x / width * 255);
            uint8_t g = (uint8_t)((float)y / height * 255);
            uint8_t b = (uint8_t)((float)(x + y) / (width + height) * 255);

            // Set pixel value based on format
            int idx = (y * width + x) * channels;

            if (format == TINYAI_IMAGE_FORMAT_GRAYSCALE) {
                data[idx] = (r + g + b) / 3; // Grayscale average
            }
            else if (format == TINYAI_IMAGE_FORMAT_RGB) {
                data[idx]     = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
            }
            else if (format == TINYAI_IMAGE_FORMAT_BGR) {
                data[idx]     = b;
                data[idx + 1] = g;
                data[idx + 2] = r;
            }
            else if (format == TINYAI_IMAGE_FORMAT_RGBA) {
                data[idx]     = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = 255; // Fully opaque
            }
        }
    }

    return image;
}

// Test image creation and manipulation
void test_image_creation()
{
    printf("  Testing image creation and properties...\n");

    const int width  = 224;
    const int height = 224;

    // Test different formats
    HyperionImage *rgbImage = create_test_image(width, height, TINYAI_IMAGE_FORMAT_RGB);
    ASSERT(rgbImage != NULL, "RGB image creation should succeed");
    ASSERT(rgbImage->width == width, "Image width should match");
    ASSERT(rgbImage->height == height, "Image height should match");
    ASSERT(rgbImage->format == TINYAI_IMAGE_FORMAT_RGB, "Image format should match");

    HyperionImage *grayscaleImage = create_test_image(width, height, TINYAI_IMAGE_FORMAT_GRAYSCALE);
    ASSERT(grayscaleImage != NULL, "Grayscale image creation should succeed");

    // Test conversion from RGB to grayscale
    HyperionImage *convertedGrayscale = hyperionImageConvert(rgbImage, TINYAI_IMAGE_FORMAT_GRAYSCALE);
    ASSERT(convertedGrayscale != NULL, "Image format conversion should succeed");
    ASSERT(convertedGrayscale->format == TINYAI_IMAGE_FORMAT_GRAYSCALE,
           "Converted format should be grayscale");
    ASSERT(convertedGrayscale->width == width, "Converted width should match");
    ASSERT(convertedGrayscale->height == height, "Converted height should match");

    // Test image resizing
    HyperionImage *resizedImage = hyperionImageResize(rgbImage, width / 2, height / 2);
    ASSERT(resizedImage != NULL, "Image resizing should succeed");
    ASSERT(resizedImage->width == width / 2, "Resized width should match");
    ASSERT(resizedImage->height == height / 2, "Resized height should match");

    // Cleanup
    hyperionImageFree(rgbImage);
    hyperionImageFree(grayscaleImage);
    hyperionImageFree(convertedGrayscale);
    hyperionImageFree(resizedImage);

    printf("    PASS\n");
}

// Test model inference with a synthetic image
void test_model_inference()
{
    printf("  Testing model inference with synthetic image...\n");

    // Create a TinyCNN model
    HyperionImageModelParams params = {.modelType       = TINYAI_IMAGE_MODEL_TINY_CNN,
                                     .inputWidth      = 224,
                                     .inputHeight     = 224,
                                     .inputChannels   = 3,
                                     .numClasses      = 10,
                                     .weightsFile     = NULL,
                                     .labelsFile      = NULL,
                                     .useQuantization = true,
                                     .useSIMD         = true,
                                     .customParams    = NULL};

    HyperionImageModel *model = hyperionImageModelCreate(&params);
    ASSERT(model != NULL, "Model creation should succeed");

    // Create a test image
    HyperionImage *image = create_test_image(224, 224, TINYAI_IMAGE_FORMAT_RGB);
    ASSERT(image != NULL, "Test image creation should succeed");

    // Run inference
    HyperionImageClassResult classResults[5]; // Top 5 results
    int                    numResults = hyperionImageModelClassify(model, image, 5, classResults);

    ASSERT(numResults > 0, "Classification should return results");
    ASSERT(numResults <= 5, "Number of results should not exceed requested count");

    // Print top classification result (just for info)
    printf("    Top prediction: ClassID=%d, Confidence=%.4f\n", classResults[0].classId,
           classResults[0].confidence);

    // Verify all confidences are between 0 and 1
    for (int i = 0; i < numResults; i++) {
        ASSERT(classResults[i].confidence >= 0.0f && classResults[i].confidence <= 1.0f,
               "Confidence values should be between 0 and 1");

        // If more than one result, check they're in decreasing order
        if (i > 0) {
            ASSERT(classResults[i].confidence <= classResults[i - 1].confidence,
                   "Confidence values should be in decreasing order");
        }
    }

    hyperionImageModelFree(model);
    hyperionImageFree(image);

    printf("    PASS\n");
}

// Test saving and loading model weights
void test_model_weight_save_load()
{
    printf("  Testing model weight saving and loading...\n");

    const char *testWeightsPath = "test_weights.bin";

    // Create a TinyCNN model
    HyperionImageModelParams params = {.modelType   = TINYAI_IMAGE_MODEL_TINY_CNN,
                                     .inputWidth  = 64, // Using smaller dimensions for faster test
                                     .inputHeight = 64,
                                     .inputChannels   = 3,
                                     .numClasses      = 10,
                                     .weightsFile     = NULL,
                                     .labelsFile      = NULL,
                                     .useQuantization = true,
                                     .useSIMD         = false,
                                     .customParams    = NULL};

    HyperionImageModel *model = hyperionImageModelCreate(&params);
    ASSERT(model != NULL, "Model creation should succeed");

    // Save weights
    bool saveResult = hyperionSaveModelWeights(model, testWeightsPath);
    ASSERT(saveResult, "Saving model weights should succeed");

    // Create a new model to load weights into
    HyperionImageModel *newModel = hyperionImageModelCreate(&params);
    ASSERT(newModel != NULL, "Second model creation should succeed");

    // Load weights
    bool loadResult = hyperionLoadModelWeights(newModel, testWeightsPath, false);
    ASSERT(loadResult, "Loading model weights should succeed");

    // Verify both models perform similarly
    HyperionImage *testImage = create_test_image(64, 64, TINYAI_IMAGE_FORMAT_RGB);

    HyperionImageClassResult results1[3];
    HyperionImageClassResult results2[3];

    hyperionImageModelClassify(model, testImage, 3, results1);
    hyperionImageModelClassify(newModel, testImage, 3, results2);

    // Top prediction should be the same
    ASSERT(results1[0].classId == results2[0].classId,
           "Top prediction should be the same after loading weights");

    // Cleanup
    hyperionImageModelFree(model);
    hyperionImageModelFree(newModel);
    hyperionImageFree(testImage);
    remove(testWeightsPath); // Delete test file

    printf("    PASS\n");
}

// Function to be called by test_main.c
void run_image_model_tests()
{
    printf("--- Running Image Model Tests ---\n");

    test_model_create_destroy();
    test_different_architectures();
    test_quantization_options();
    test_image_creation();
    test_model_inference();
    test_model_weight_save_load();

    printf("--- Image Model Tests Finished ---\n");
}
