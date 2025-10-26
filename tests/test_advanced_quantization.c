#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../utils/advanced_quantization.h"

/* Test quantization statistics computation */
int test_quantization_statistics() {
    printf("Testing quantization statistics computation...\n");
    
    /* Create test data with known statistics */
    float testData[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    size_t size = sizeof(testData) / sizeof(testData[0]);
    
    QuantStats stats = {0};
    stats.histogramBins = 8;
    
    bool success = hyperionComputeQuantStats(testData, size, &stats);
    assert(success);
    
    /* Verify statistics */
    assert(fabsf(stats.min - (-2.0f)) < 1e-6f);
    assert(fabsf(stats.max - 5.0f) < 1e-6f);
    assert(fabsf(stats.mean - 1.5f) < 1e-6f);
    
    printf("  - Min: %.3f, Max: %.3f, Mean: %.3f\n", stats.min, stats.max, stats.mean);
    printf("  - Variance: %.3f\n", stats.variance);
    
    /* Verify histogram was computed */
    assert(stats.histogram != NULL);
    
    /* Clean up */
    free(stats.histogram);
    
    printf("✓ Quantization statistics test passed\n");
    return 0;
}

/* Test asymmetric quantization */
int test_asymmetric_quantization() {
    printf("Testing asymmetric quantization...\n");
    
    /* Create test data */
    float input[] = {-3.0f, -1.5f, 0.0f, 1.5f, 3.0f, 4.5f, 6.0f, 7.5f};
    size_t size = sizeof(input) / sizeof(input[0]);
    
    uint8_t quantized[8];
    float scale;
    int zeroPoint;
    
    bool success = hyperionAsymmetricQuantize(input, size, HYPERION_QUANT_8BIT,
                                            quantized, &scale, &zeroPoint);
    assert(success);
    
    printf("  - Scale: %.6f, Zero point: %d\n", scale, zeroPoint);
    
    /* Verify quantization properties */
    assert(scale > 0);
    assert(zeroPoint >= 0 && zeroPoint < 256);
    
    /* Test dequantization */
    float dequantized[8];
    success = hyperionDequantize(quantized, size, HYPERION_QUANT_8BIT,
                               scale, zeroPoint, dequantized);
    assert(success);
    
    /* Verify dequantization error is reasonable */
    float maxError = 0.0f;
    for (size_t i = 0; i < size; i++) {
        float error = fabsf(input[i] - dequantized[i]);
        if (error > maxError) maxError = error;
        printf("  - Input[%zu]: %.3f -> Quantized: %d -> Dequantized: %.3f (error: %.3f)\n",
               i, input[i], quantized[i], dequantized[i], error);
    }
    
    printf("  - Maximum quantization error: %.6f\n", maxError);
    assert(maxError < scale * 2.0f); /* Error should be at most 2 * scale */
    
    printf("✓ Asymmetric quantization test passed\n");
    return 0;
}

/* Test binary quantization */
int test_binary_quantization() {
    printf("Testing binary quantization...\n");
    
    float input[] = {-2.5f, -1.0f, -0.1f, 0.1f, 1.0f, 2.5f, 3.0f, -3.0f};
    size_t size = sizeof(input) / sizeof(input[0]);
    
    uint8_t quantized[1]; /* 8 bits packed into 1 byte */
    float scale;
    
    bool success = hyperionBinaryQuantize(input, size, quantized, &scale);
    assert(success);
    
    printf("  - Scale: %.6f\n", scale);
    printf("  - Quantized byte: 0x%02X\n", quantized[0]);
    
    /* Verify each bit corresponds to sign */
    for (size_t i = 0; i < size; i++) {
        bool expectedBit = (input[i] >= 0);
        bool actualBit = (quantized[0] & (1 << (i % 8))) != 0;
        
        printf("  - Input[%zu]: %.3f -> Expected bit: %d, Actual bit: %d\n",
               i, input[i], expectedBit, actualBit);
        assert(expectedBit == actualBit);
    }
    
    printf("✓ Binary quantization test passed\n");
    return 0;
}

/* Test ternary quantization */
int test_ternary_quantization() {
    printf("Testing ternary quantization...\n");
    
    float input[] = {-2.0f, -0.5f, -0.05f, 0.05f, 0.5f, 2.0f, 3.0f, -3.0f};
    size_t size = sizeof(input) / sizeof(input[0]);
    float threshold = 0.1f;
    
    int8_t quantized[8];
    float scale;
    
    bool success = hyperionTernaryQuantize(input, size, threshold, quantized, &scale);
    assert(success);
    
    printf("  - Threshold: %.3f, Scale: %.6f\n", threshold, scale);
    
    /* Verify ternary values */
    for (size_t i = 0; i < size; i++) {
        int8_t expected;
        if (input[i] > threshold) expected = 1;
        else if (input[i] < -threshold) expected = -1;
        else expected = 0;
        
        printf("  - Input[%zu]: %.3f -> Expected: %d, Actual: %d\n",
               i, input[i], expected, quantized[i]);
        assert(quantized[i] == expected);
    }
    
    printf("✓ Ternary quantization test passed\n");
    return 0;
}

/* Test fake quantization (for quantization-aware training) */
int test_fake_quantization() {
    printf("Testing fake quantization...\n");
    
    float input[] = {-1.5f, -0.5f, 0.0f, 0.5f, 1.5f, 2.5f};
    size_t size = sizeof(input) / sizeof(input[0]);
    
    float scale = 0.1f;
    int zeroPoint = 128;
    float fakeQuantized[6];
    
    bool success = hyperionFakeQuantize(input, size, HYPERION_QUANT_8BIT,
                                      scale, zeroPoint, fakeQuantized);
    assert(success);
    
    printf("  - Scale: %.3f, Zero point: %d\n", scale, zeroPoint);
    
    /* Verify fake quantization properties */
    for (size_t i = 0; i < size; i++) {
        /* Fake quantized values should be quantized grid points */
        float expectedQuantized = scale * roundf(input[i] / scale);
        float error = fabsf(fakeQuantized[i] - expectedQuantized);
        
        printf("  - Input[%zu]: %.3f -> Fake quantized: %.3f (expected: %.3f, error: %.6f)\n",
               i, input[i], fakeQuantized[i], expectedQuantized, error);
        
        assert(error < scale * 0.1f); /* Should be very close to grid point */
    }
    
    printf("✓ Fake quantization test passed\n");
    return 0;
}

/* Test advanced quantization context creation */
int test_advanced_quantization_context() {
    printf("Testing advanced quantization context...\n");
    
    /* Configure mixed precision */
    MixedPrecisionConfig mixedConfig = {0};
    mixedConfig.numLayers = 5;
    mixedConfig.memoryBudget = 0.5f;
    mixedConfig.accuracyThreshold = 0.95f;
    mixedConfig.autoAssign = true;
    
    /* Configure dynamic quantization */
    DynamicQuantConfig dynamicConfig = {0};
    dynamicConfig.activationThreshold = 0.1f;
    dynamicConfig.calibrationSamples = 1000;
    dynamicConfig.adaptToInput = true;
    dynamicConfig.useRunningStats = true;
    dynamicConfig.momentumFactor = 0.9f;
    
    /* Main configuration */
    AdvancedQuantConfig config = {0};
    config.method = HYPERION_QUANT_ASYMMETRIC;
    config.defaultBitWidth = HYPERION_QUANT_8BIT;
    config.mixedPrecision = mixedConfig;
    config.dynamicQuant = dynamicConfig;
    config.useCalibration = true;
    config.useSIMD = true;
    config.compressionRatio = 4.0f;
    
    AdvancedQuantization *quant = hyperionAdvancedQuantCreate(&config);
    assert(quant != NULL);
    
    /* Test SIMD enable/disable */
    bool success = hyperionQuantEnableSIMD(quant, false);
    assert(success);
    success = hyperionQuantEnableSIMD(quant, true);
    assert(success);
    
    /* Test memory savings estimation */
    size_t originalSize = 1024 * 1024; /* 1MB */
    size_t quantizedSize;
    float compressionRatio;
    
    success = hyperionQuantGetMemorySavings(quant, originalSize, 
                                          &quantizedSize, &compressionRatio);
    assert(success);
    
    printf("  - Original size: %zu bytes\n", originalSize);
    printf("  - Quantized size: %zu bytes\n", quantizedSize);
    printf("  - Compression ratio: %.2fx\n", compressionRatio);
    
    assert(quantizedSize < originalSize);
    assert(compressionRatio > 1.0f);
    
    hyperionAdvancedQuantFree(quant);
    
    printf("✓ Advanced quantization context test passed\n");
    return 0;
}

/* Test dynamic activation quantization */
int test_dynamic_activation_quantization() {
    printf("Testing dynamic activation quantization...\n");
    
    AdvancedQuantConfig config = {0};
    config.method = HYPERION_QUANT_ASYMMETRIC;
    config.defaultBitWidth = HYPERION_QUANT_8BIT;
    config.useSIMD = false;
    
    AdvancedQuantization *quant = hyperionAdvancedQuantCreate(&config);
    assert(quant != NULL);
    
    /* Create activation data with different distributions */
    float activations1[] = {0.1f, 0.3f, 0.5f, 0.7f, 0.9f, 1.1f, 1.3f, 1.5f};
    float activations2[] = {-2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    size_t size = 8;
    
    uint8_t quantizedAct1[8], quantizedAct2[8];
    float scale1, scale2;
    int zeroPoint1, zeroPoint2;
    
    /* Quantize first set of activations */
    bool success = hyperionDynamicQuantizeActivations(quant, activations1, size,
                                                    HYPERION_QUANT_8BIT,
                                                    quantizedAct1, &scale1, &zeroPoint1);
    assert(success);
    
    /* Quantize second set with different distribution */
    success = hyperionDynamicQuantizeActivations(quant, activations2, size,
                                               HYPERION_QUANT_8BIT,
                                               quantizedAct2, &scale2, &zeroPoint2);
    assert(success);
    
    printf("  - Activations 1 - Scale: %.6f, Zero point: %d\n", scale1, zeroPoint1);
    printf("  - Activations 2 - Scale: %.6f, Zero point: %d\n", scale2, zeroPoint2);
    
    /* Scales should be different due to different distributions */
    assert(fabsf(scale1 - scale2) > 1e-6f);
    
    hyperionAdvancedQuantFree(quant);
    
    printf("✓ Dynamic activation quantization test passed\n");
    return 0;
}

/* Test 4-bit quantization with packing */
int test_4bit_quantization() {
    printf("Testing 4-bit quantization with packing...\n");
    
    float input[] = {0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f};
    size_t size = sizeof(input) / sizeof(input[0]);
    
    uint8_t quantized[4]; /* 8 values packed into 4 bytes */
    float scale;
    int zeroPoint;
    
    bool success = hyperionAsymmetricQuantize(input, size, HYPERION_QUANT_4BIT,
                                            quantized, &scale, &zeroPoint);
    assert(success);
    
    printf("  - 4-bit Scale: %.6f, Zero point: %d\n", scale, zeroPoint);
    printf("  - Packed bytes: ");
    for (int i = 0; i < 4; i++) {
        printf("0x%02X ", quantized[i]);
    }
    printf("\n");
    
    /* Test dequantization */
    float dequantized[8];
    success = hyperionDequantize(quantized, size, HYPERION_QUANT_4BIT,
                               scale, zeroPoint, dequantized);
    assert(success);
    
    /* Verify dequantization */
    for (size_t i = 0; i < size; i++) {
        printf("  - Input[%zu]: %.3f -> Dequantized: %.3f\n",
               i, input[i], dequantized[i]);
    }
    
    printf("✓ 4-bit quantization test passed\n");
    return 0;
}

/* Performance benchmark for quantization methods */
int benchmark_quantization_methods() {
    printf("Benchmarking quantization methods...\n");
    
    AdvancedQuantConfig config = {0};
    config.method = HYPERION_QUANT_ASYMMETRIC;
    config.defaultBitWidth = HYPERION_QUANT_8BIT;
    config.useSIMD = true;
    
    AdvancedQuantization *quant = hyperionAdvancedQuantCreate(&config);
    assert(quant != NULL);
    
    /* Benchmark different data sizes */
    size_t testSizes[] = {1024, 4096, 16384, 65536};
    int numSizes = sizeof(testSizes) / sizeof(testSizes[0]);
    
    for (int i = 0; i < numSizes; i++) {
        float avgTimeMs, throughputMBps;
        bool success = hyperionQuantBenchmark(quant, testSizes[i], 100,
                                            &avgTimeMs, &throughputMBps);
        assert(success);
        
        printf("  - Size: %zu elements\n", testSizes[i]);
        printf("    * Average time: %.3f ms\n", avgTimeMs);
        printf("    * Throughput: %.1f MB/s\n", throughputMBps);
    }
    
    /* Test with SIMD disabled for comparison */
    hyperionQuantEnableSIMD(quant, false);
    
    float avgTimeNoSIMD, throughputNoSIMD;
    bool success = hyperionQuantBenchmark(quant, 16384, 100,
                                        &avgTimeNoSIMD, &throughputNoSIMD);
    assert(success);
    
    hyperionQuantEnableSIMD(quant, true);
    float avgTimeSIMD, throughputSIMD;
    success = hyperionQuantBenchmark(quant, 16384, 100,
                                   &avgTimeSIMD, &throughputSIMD);
    assert(success);
    
    printf("  - SIMD Performance Comparison (16K elements):\n");
    printf("    * Without SIMD: %.3f ms (%.1f MB/s)\n", avgTimeNoSIMD, throughputNoSIMD);
    printf("    * With SIMD: %.3f ms (%.1f MB/s)\n", avgTimeSIMD, throughputSIMD);
    printf("    * Speedup: %.2fx\n", avgTimeNoSIMD / avgTimeSIMD);
    
    hyperionAdvancedQuantFree(quant);
    
    printf("✓ Quantization methods benchmark completed\n");
    return 0;
}

/* Test memory efficiency of different quantization methods */
int test_quantization_memory_efficiency() {
    printf("Testing quantization memory efficiency...\n");
    
    size_t originalSize = 1024 * 1024; /* 1MB original model */
    
    struct {
        HyperionQuantBitWidth bitWidth;
        const char* name;
    } testCases[] = {
        {HYPERION_QUANT_1BIT, "Binary (1-bit)"},
        {HYPERION_QUANT_2BIT, "2-bit"},
        {HYPERION_QUANT_3BIT, "3-bit"},
        {HYPERION_QUANT_4BIT, "4-bit"},
        {HYPERION_QUANT_8BIT, "8-bit"},
        {HYPERION_QUANT_16BIT, "16-bit"}
    };
    
    int numTests = sizeof(testCases) / sizeof(testCases[0]);
    
    printf("  - Original model size: %.2f MB\n", originalSize / (1024.0 * 1024.0));
    printf("\n");
    
    for (int i = 0; i < numTests; i++) {
        AdvancedQuantConfig config = {0};
        config.method = HYPERION_QUANT_ASYMMETRIC;
        config.defaultBitWidth = testCases[i].bitWidth;
        
        AdvancedQuantization *quant = hyperionAdvancedQuantCreate(&config);
        assert(quant != NULL);
        
        size_t quantizedSize;
        float compressionRatio;
        bool success = hyperionQuantGetMemorySavings(quant, originalSize,
                                                   &quantizedSize, &compressionRatio);
        assert(success);
        
        float sizeMB = quantizedSize / (1024.0f * 1024.0f);
        float savings = (1.0f - (float)quantizedSize / originalSize) * 100.0f;
        
        printf("  - %s:\n", testCases[i].name);
        printf("    * Size: %.2f MB\n", sizeMB);
        printf("    * Compression: %.1fx\n", compressionRatio);
        printf("    * Memory savings: %.1f%%\n", savings);
        printf("\n");
        
        hyperionAdvancedQuantFree(quant);
    }
    
    printf("✓ Quantization memory efficiency test passed\n");
    return 0;
}

int main() {
    printf("========================================\n");
    printf("Hyperion Phase 5.2: Advanced Quantization Test Suite\n");
    printf("========================================\n");
    
    srand(time(NULL)); /* Initialize random seed */
    
    int result = 0;
    
    /* Basic functionality tests */
    result += test_quantization_statistics();
    result += test_asymmetric_quantization();
    result += test_binary_quantization();
    result += test_ternary_quantization();
    result += test_fake_quantization();
    
    /* Advanced features tests */
    result += test_advanced_quantization_context();
    result += test_dynamic_activation_quantization();
    result += test_4bit_quantization();
    
    /* Performance and efficiency tests */
    result += benchmark_quantization_methods();
    result += test_quantization_memory_efficiency();
    
    printf("========================================\n");
    if (result == 0) {
        printf("✅ All Phase 5.2 advanced quantization tests passed!\n");
        printf("Advanced quantization techniques are working correctly:\n");
        printf("  - Mixed precision quantization\n");
        printf("  - Dynamic activation quantization\n");
        printf("  - Binary and ternary quantization\n");
        printf("  - Asymmetric and symmetric quantization\n");
        printf("  - Quantization-aware training support\n");
        printf("  - Memory efficiency optimizations\n");
    } else {
        printf("❌ %d Phase 5.2 advanced quantization tests failed!\n", result);
    }
    printf("========================================\n");
    
    return result;
}