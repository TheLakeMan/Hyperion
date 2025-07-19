/**
 * @file test_mixed_precision.c
 * @brief Mixed precision quantization tests for Hyperion
 */

#include "../utils/quantize_mixed.h"
#include "../utils/simd_ops.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ASSERT(condition, message)                                                                 \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Assertion Failed: %s (%s:%d)\n", message, __FILE__, __LINE__);        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

/**
 * Create a test matrix with specific patterns to stress quantization
 */
static float *create_test_matrix(int rows, int cols, int pattern)
{
    float *matrix = (float *)malloc(rows * cols * sizeof(float));
    ASSERT(matrix != NULL, "Failed to allocate test matrix");

    switch (pattern) {
    case 0: // Uniform random distribution
        for (int i = 0; i < rows * cols; i++) {
            matrix[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f; // -1.0 to 1.0
        }
        break;

    case 1: // Normal-like distribution (more values near zero)
        for (int i = 0; i < rows * cols; i++) {
            float r1  = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            float r2  = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
            matrix[i] = (r1 + r2) * 0.5f; // Sum of randoms approximates normal
        }
        break;

    case 2: // Outlier pattern (mostly small values with a few large ones)
        for (int i = 0; i < rows * cols; i++) {
            matrix[i] = ((float)rand() / RAND_MAX) * 0.1f; // Most values are small
            if (rand() % 100 == 0) {                       // 1% outliers
                matrix[i] = ((float)rand() / RAND_MAX) * 10.0f;
            }
        }
        break;

    case 3: // Bipolar pattern (values tend to cluster at extremes)
        for (int i = 0; i < rows * cols; i++) {
            float r = (float)rand() / RAND_MAX;
            if (r < 0.5f) {
                matrix[i] = -1.0f + r * 0.2f; // Cluster near -1
            }
            else {
                matrix[i] = 0.8f + r * 0.2f; // Cluster near 1
            }
        }
        break;

    default:
        for (int i = 0; i < rows * cols; i++) {
            matrix[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        }
    }

    return matrix;
}

/**
 * Compare original and dequantized matrices with precision-appropriate tolerance
 */
static float compare_matrices(const float *original, const float *dequantized, int size,
                              HyperionMixedPrecType precision)
{
    float max_error = 0.0f;
    float avg_error = 0.0f;

    for (int i = 0; i < size; i++) {
        float error = fabsf(original[i] - dequantized[i]);
        max_error   = fmaxf(max_error, error);
        avg_error += error;
    }
    avg_error /= size;

    // Determine acceptable error based on precision
    float acceptable_max_error;
    switch (precision) {
    case TINYAI_MIXED_PREC_FP32:
        acceptable_max_error = 0.00001f; // Minimal for FP32
        break;
    case TINYAI_MIXED_PREC_FP16:
        acceptable_max_error = 0.001f; // Reasonable for FP16
        break;
    case TINYAI_MIXED_PREC_INT8:
        acceptable_max_error = 0.05f; // Reasonable for INT8
        break;
    case TINYAI_MIXED_PREC_INT4:
        acceptable_max_error = 0.2f; // Reasonable for INT4
        break;
    case TINYAI_MIXED_PREC_INT2:
        acceptable_max_error = 0.5f; // Reasonable for INT2
        break;
    default:
        acceptable_max_error = 0.1f;
    }

    printf("    Max error: %.6f, Avg error: %.6f, Acceptable: %.6f\n", max_error, avg_error,
           acceptable_max_error);

    return max_error <= acceptable_max_error;
}

/**
 * Test quantization and dequantization at various bit precisions
 */
static void test_precision_quantization()
{
    printf("Testing mixed precision quantization across different bit widths...\n");

    const int rows = 128;
    const int cols = 128;
    const int size = rows * cols;

    HyperionMixedPrecType precisions[] = {TINYAI_MIXED_PREC_FP32, TINYAI_MIXED_PREC_FP16,
                                        TINYAI_MIXED_PREC_INT8, TINYAI_MIXED_PREC_INT4,
                                        TINYAI_MIXED_PREC_INT2};

    const char *precision_names[] = {"FP32", "FP16", "INT8", "INT4", "INT2"};

    // Test each precision with different data patterns
    for (int pattern = 0; pattern < 4; pattern++) {
        printf("  Testing pattern %d...\n", pattern);

        // Create test matrix
        float *original = create_test_matrix(rows, cols, pattern);

        for (int p = 0; p < 5; p++) {
            HyperionMixedPrecType precision = precisions[p];
            printf("    Testing %s precision...\n", precision_names[p]);

            // Create mixed precision matrix
            HyperionMixedPrecMatrix *quantized = hyperionCreateMixedPrecMatrix(
                original, rows, cols, precision, 0.0f); // Auto threshold

            ASSERT(quantized != NULL, "Failed to create mixed precision matrix");

            // Dequantize
            float *dequantized = (float *)malloc(size * sizeof(float));
            ASSERT(dequantized != NULL, "Failed to allocate dequantized matrix");

            bool success = hyperionMixedPrecToFloat(quantized, dequantized);
            ASSERT(success, "Failed to dequantize matrix");

            // Compare original and dequantized
            bool within_tolerance = compare_matrices(original, dequantized, size, precision);
            ASSERT(within_tolerance, "Quantization error exceeds acceptable tolerance");

            // Calculate memory savings
            size_t original_size     = size * sizeof(float);
            size_t quantized_size    = hyperionGetMixedPrecMatrixMemoryUsage(quantized);
            float  compression_ratio = (float)original_size / quantized_size;

            printf("    Memory usage: Original=%zu bytes, Quantized=%zu bytes, Ratio=%.2fx\n",
                   original_size, quantized_size, compression_ratio);

            // Cleanup
            free(dequantized);
            hyperionFreeMixedPrecMatrix(quantized);
        }

        free(original);
    }

    printf("  PASS: Mixed precision quantization tests\n");
}

/**
 * Test per-layer mixed precision configuration
 */
static void test_per_layer_mixed_precision()
{
    printf("Testing per-layer mixed precision configuration...\n");

    // Create a default config for 3 layers
    HyperionMixedPrecConfig *config = hyperionCreateDefaultMixedPrecConfig(3);
    ASSERT(config != NULL, "Failed to create default mixed precision config");

    // Modify config to use different precisions per layer
    config->layerConfigs[0].weightPrecision = TINYAI_MIXED_PREC_INT8; // Layer 1: INT8 weights
    config->layerConfigs[1].weightPrecision = TINYAI_MIXED_PREC_INT4; // Layer 2: INT4 weights
    config->layerConfigs[2].weightPrecision = TINYAI_MIXED_PREC_FP16; // Layer 3: FP16 weights

    config->layerConfigs[0].activPrecision = TINYAI_MIXED_PREC_FP16; // Layer 1: FP16 activations
    config->layerConfigs[1].activPrecision = TINYAI_MIXED_PREC_INT8; // Layer 2: INT8 activations
    config->layerConfigs[2].activPrecision = TINYAI_MIXED_PREC_FP32; // Layer 3: FP32 activations

    // Verify configuration
    ASSERT(config->numLayers == 3, "Config should have 3 layers");
    ASSERT(config->layerConfigs[0].weightPrecision == TINYAI_MIXED_PREC_INT8,
           "Layer 1 weight precision should be INT8");
    ASSERT(config->layerConfigs[1].weightPrecision == TINYAI_MIXED_PREC_INT4,
           "Layer 2 weight precision should be INT4");
    ASSERT(config->layerConfigs[2].weightPrecision == TINYAI_MIXED_PREC_FP16,
           "Layer 3 weight precision should be FP16");

    // Estimate memory usage - create matrices with corresponding precisions
    const int rows  = 1024;
    const int cols  = 1024;
    float    *dummy = create_test_matrix(rows, cols, 0);

    // Create matrices with layer-specific precisions
    HyperionMixedPrecMatrix *layer1 = hyperionCreateMixedPrecMatrix(
        dummy, rows, cols, config->layerConfigs[0].weightPrecision, 0.0f);
    HyperionMixedPrecMatrix *layer2 = hyperionCreateMixedPrecMatrix(
        dummy, rows, cols, config->layerConfigs[1].weightPrecision, 0.0f);
    HyperionMixedPrecMatrix *layer3 = hyperionCreateMixedPrecMatrix(
        dummy, rows, cols, config->layerConfigs[2].weightPrecision, 0.0f);

    ASSERT(layer1 != NULL && layer2 != NULL && layer3 != NULL, "Failed to create layer matrices");

    // Compare memory usage
    size_t layer1_size = hyperionGetMixedPrecMatrixMemoryUsage(layer1);
    size_t layer2_size = hyperionGetMixedPrecMatrixMemoryUsage(layer2);
    size_t layer3_size = hyperionGetMixedPrecMatrixMemoryUsage(layer3);

    printf("    Layer 1 (INT8) size: %zu bytes\n", layer1_size);
    printf("    Layer 2 (INT4) size: %zu bytes\n", layer2_size);
    printf("    Layer 3 (FP16) size: %zu bytes\n", layer3_size);

    // Verify expected size relationships
    ASSERT(layer2_size < layer1_size, "INT4 should use less memory than INT8");
    ASSERT(layer3_size > layer2_size, "FP16 should use more memory than INT4");

    // Cleanup
    hyperionFreeMixedPrecMatrix(layer1);
    hyperionFreeMixedPrecMatrix(layer2);
    hyperionFreeMixedPrecMatrix(layer3);
    free(dummy);
    hyperionFreeMixedPrecConfig(config);

    printf("  PASS: Per-layer mixed precision configuration tests\n");
}

/**
 * Test matrix operations with mixed precision
 */
static void test_mixed_precision_operations()
{
    printf("Testing mixed precision matrix operations...\n");

    const int m = 64;  // Output rows
    const int k = 128; // Inner dimension
    const int n = 64;  // Output columns

    // Create test matrices
    float *a_data = create_test_matrix(m, k, 0);
    float *b_data = create_test_matrix(k, n, 1);

    // Create mixed precision matrices with different precisions
    HyperionMixedPrecMatrix *a_fp32 =
        hyperionCreateMixedPrecMatrix(a_data, m, k, TINYAI_MIXED_PREC_FP32, 0.0f);
    HyperionMixedPrecMatrix *a_int8 =
        hyperionCreateMixedPrecMatrix(a_data, m, k, TINYAI_MIXED_PREC_INT8, 0.0f);
    HyperionMixedPrecMatrix *a_int4 =
        hyperionCreateMixedPrecMatrix(a_data, m, k, TINYAI_MIXED_PREC_INT4, 0.0f);

    HyperionMixedPrecMatrix *b_fp32 =
        hyperionCreateMixedPrecMatrix(b_data, k, n, TINYAI_MIXED_PREC_FP32, 0.0f);
    HyperionMixedPrecMatrix *b_int8 =
        hyperionCreateMixedPrecMatrix(b_data, k, n, TINYAI_MIXED_PREC_INT8, 0.0f);
    HyperionMixedPrecMatrix *b_int4 =
        hyperionCreateMixedPrecMatrix(b_data, k, n, TINYAI_MIXED_PREC_INT4, 0.0f);

    ASSERT(a_fp32 != NULL && a_int8 != NULL && a_int4 != NULL, "Failed to create A matrices");
    ASSERT(b_fp32 != NULL && b_int8 != NULL && b_int4 != NULL, "Failed to create B matrices");

    // Allocate result matrices
    HyperionMixedPrecMatrix *c_fp32 = (HyperionMixedPrecMatrix *)malloc(sizeof(HyperionMixedPrecMatrix));
    c_fp32->rows                  = m;
    c_fp32->cols                  = n;
    c_fp32->precision             = TINYAI_MIXED_PREC_FP32;
    c_fp32->data                  = malloc(m * n * sizeof(float));
    c_fp32->dataSize              = m * n * sizeof(float);

    HyperionMixedPrecMatrix *c_int8 = (HyperionMixedPrecMatrix *)malloc(sizeof(HyperionMixedPrecMatrix));
    c_int8->rows                  = m;
    c_int8->cols                  = n;
    c_int8->precision             = TINYAI_MIXED_PREC_INT8;
    c_int8->data                  = malloc(m * n * sizeof(int8_t));
    c_int8->dataSize              = m * n * sizeof(int8_t);

    // Perform matrix multiplications
    printf("  Testing FP32 x FP32 matrix multiplication...\n");
    bool fp32_success = hyperionMixedPrecMatMul(a_fp32, b_fp32, c_fp32);
    ASSERT(fp32_success, "FP32 matrix multiplication failed");

    printf("  Testing INT8 x INT8 matrix multiplication...\n");
    bool int8_success = hyperionMixedPrecMatMul(a_int8, b_int8, c_int8);
    ASSERT(int8_success, "INT8 matrix multiplication failed");

    printf("  Testing INT4 x INT4 matrix multiplication...\n");
    // Convert result to FP32 for comparison
    float *c_fp32_data = (float *)c_fp32->data;
    float *c_int8_data = (float *)malloc(m * n * sizeof(float));

    // Convert INT8 result to float for comparison
    for (int i = 0; i < m * n; i++) {
        c_int8_data[i] = ((int8_t *)c_int8->data)[i] * c_int8->scale;
    }

    // Compute reference result
    float *c_ref = (float *)malloc(m * n * sizeof(float));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            c_ref[i * n + j] = 0.0f;
            for (int l = 0; l < k; l++) {
                c_ref[i * n + j] += a_data[i * k + l] * b_data[l * n + j];
            }
        }
    }

    // Compare results
    float fp32_max_error = 0.0f;
    float int8_max_error = 0.0f;

    for (int i = 0; i < m * n; i++) {
        fp32_max_error = fmaxf(fp32_max_error, fabsf(c_fp32_data[i] - c_ref[i]));
        int8_max_error = fmaxf(int8_max_error, fabsf(c_int8_data[i] - c_ref[i]));
    }

    printf("    FP32 max error: %.6f\n", fp32_max_error);
    printf("    INT8 max error: %.6f\n", int8_max_error);

    ASSERT(fp32_max_error < 0.001f, "FP32 matrix multiplication error too high");
    ASSERT(int8_max_error < 0.2f, "INT8 matrix multiplication error too high");

    // Cleanup
    free(c_ref);
    free(c_int8_data);
    free(a_data);
    free(b_data);

    hyperionFreeMixedPrecMatrix(a_fp32);
    hyperionFreeMixedPrecMatrix(a_int8);
    hyperionFreeMixedPrecMatrix(a_int4);
    hyperionFreeMixedPrecMatrix(b_fp32);
    hyperionFreeMixedPrecMatrix(b_int8);
    hyperionFreeMixedPrecMatrix(b_int4);

    free(c_fp32->data);
    free(c_int8->data);
    free(c_fp32);
    free(c_int8);

    printf("  PASS: Mixed precision matrix operations tests\n");
}

/**
 * Run all mixed precision tests
 */
int main()
{
    printf("Running Mixed Precision Quantization Tests...\n");

    // Seed random number generator
    srand((unsigned)time(NULL));

    // Run tests
    test_precision_quantization();
    test_per_layer_mixed_precision();
    test_mixed_precision_operations();

    printf("All Mixed Precision Quantization Tests PASSED\n");
    return 0;
}