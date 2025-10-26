/**
 * @file image_test.c
 * @brief Test program for Hyperion image model functionality
 */

#include "image_model.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * Create a simple test image for classification
 */
HyperionImage *createTestImage(int width, int height)
{
    HyperionImage *image = hyperionImageCreate(width, height, HYPERION_IMAGE_FORMAT_RGB);
    if (!image) {
        fprintf(stderr, "Failed to create test image\n");
        return NULL;
    }

    /* Create a simple pattern (gradient) */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t *pixel = image->data + (y * width + x) * 3;

            /* Create red gradient from left to right */
            pixel[0] = (uint8_t)(255.0f * x / width);

            /* Create green gradient from top to bottom */
            pixel[1] = (uint8_t)(255.0f * y / height);

            /* Create blue gradient diagonally */
            pixel[2] = (uint8_t)(255.0f * (x + y) / (width + height));
        }
    }

    return image;
}

int main(int argc, char **argv)
{
    printf("Hyperion Image Classification Test\n");
    printf("================================\n\n");

    /* Create a HyperionCNN model */
    HyperionImageModelParams params = {.modelType       = HYPERION_IMAGE_MODEL_TINY_CNN,
                                     .inputWidth      = 224,
                                     .inputHeight     = 224,
                                     .inputChannels   = 3,
                                     .numClasses      = 10,
                                     .weightsFile     = NULL, /* No weights for this test */
                                     .labelsFile      = NULL, /* No labels for this test */
                                     .useQuantization = true,
                                     .useSIMD         = true,
                                     .customParams    = NULL};

    HyperionImageModel *model = hyperionImageModelCreate(&params);
    if (!model) {
        fprintf(stderr, "Failed to create image model\n");
        return 1;
    }

    /* Create a test image */
    HyperionImage *image = createTestImage(640, 480);
    if (!image) {
        hyperionImageModelFree(model);
        return 1;
    }

    printf("Created test image (640x480) with RGB gradient pattern\n");

    /* Print model information */
    hyperionImageModelPrintSummary(model);

    /* Classify the image */
    printf("\nClassifying image...\n");

    HyperionImageClassResult results[5]; /* Get top 5 results */
    int                    numResults = hyperionImageModelClassify(model, image, 5, results);

    if (numResults < 0) {
        fprintf(stderr, "Classification failed\n");
    }
    else {
        printf("\nClassification Results:\n");
        for (int i = 0; i < numResults; i++) {
            printf("  Class %d: Confidence %.4f\n", results[i].classId, results[i].confidence);
        }
    }

    /* Clean up */
    hyperionImageFree(image);
    hyperionImageModelFree(model);

    printf("\nTest completed\n");
    return 0;
}