#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "../models/multimodal/cross_modal_attention.h"
#include "../models/multimodal/vision_language_integration.h"

/* Test cross-modal attention creation and basic functionality */
int test_cross_modal_attention_basic() {
    printf("Testing cross-modal attention basic functionality...\n");
    
    CrossModalAttnConfig config = {
        .numHeads = 8,
        .headDim = 64,
        .maxSeqLen = 512,
        .dropoutRate = 0.1f,
        .useLayerNorm = true,
        .useResidual = true,
        .useQuantization = false,
        .useSIMD = true
    };
    
    CrossModalAttention *attn = hyperionCrossModalAttnCreate(&config);
    assert(attn != NULL);
    
    /* Test memory usage query */
    size_t weightMemory, activationMemory;
    bool success = hyperionCrossModalAttnGetMemoryUsage(attn, &weightMemory, &activationMemory);
    assert(success);
    assert(weightMemory > 0);
    assert(activationMemory > 0);
    
    printf("  - Weight memory: %zu bytes\n", weightMemory);
    printf("  - Activation memory: %zu bytes\n", activationMemory);
    
    /* Test SIMD enable/disable */
    success = hyperionCrossModalAttnEnableSIMD(attn, false);
    assert(success);
    success = hyperionCrossModalAttnEnableSIMD(attn, true);
    assert(success);
    
    /* Test quantization enable/disable */
    success = hyperionCrossModalAttnSetQuantization(attn, true);
    assert(success);
    success = hyperionCrossModalAttnSetQuantization(attn, false);
    assert(success);
    
    hyperionCrossModalAttnFree(attn);
    
    printf("✓ Cross-modal attention basic test passed\n");
    return 0;
}

/* Test attention computation with synthetic data */
int test_cross_modal_attention_computation() {
    printf("Testing cross-modal attention computation...\n");
    
    CrossModalAttnConfig config = {
        .numHeads = 4,
        .headDim = 32,
        .maxSeqLen = 64,
        .dropoutRate = 0.0f,
        .useLayerNorm = false,
        .useResidual = false,
        .useQuantization = false,
        .useSIMD = false
    };
    
    CrossModalAttention *attn = hyperionCrossModalAttnCreate(&config);
    assert(attn != NULL);
    
    /* Create synthetic input data */
    int queryLen = 8, keyLen = 16;
    int queryDim = 128, keyDim = 128, valueDim = 128;
    int outputDim = 128;
    
    float *queryFeatures = (float*)calloc(queryLen * queryDim, sizeof(float));
    float *keyFeatures = (float*)calloc(keyLen * keyDim, sizeof(float));
    float *valueFeatures = (float*)calloc(keyLen * valueDim, sizeof(float));
    float *output = (float*)calloc(queryLen * outputDim, sizeof(float));
    
    assert(queryFeatures && keyFeatures && valueFeatures && output);
    
    /* Initialize with random values */
    for (int i = 0; i < queryLen * queryDim; i++) {
        queryFeatures[i] = (float)rand() / RAND_MAX - 0.5f;
    }
    for (int i = 0; i < keyLen * keyDim; i++) {
        keyFeatures[i] = (float)rand() / RAND_MAX - 0.5f;
        valueFeatures[i] = (float)rand() / RAND_MAX - 0.5f;
    }
    
    /* Compute attention */
    bool success = hyperionCrossModalAttnCompute(attn,
                                               queryFeatures, keyFeatures, valueFeatures,
                                               queryDim, keyDim, valueDim,
                                               queryLen, keyLen,
                                               output, outputDim, NULL);
    assert(success);
    
    /* Verify output is not all zeros */
    bool hasNonZero = false;
    for (int i = 0; i < queryLen * outputDim; i++) {
        if (fabsf(output[i]) > 1e-6f) {
            hasNonZero = true;
            break;
        }
    }
    assert(hasNonZero);
    
    /* Test with attention mask */
    AttentionMask *mask = hyperionAttnMaskCreate(queryLen, keyLen);
    assert(mask != NULL);
    
    /* Set causal mask */
    success = hyperionAttnMaskSetCausal(mask);
    assert(success);
    
    /* Compute attention with mask */
    success = hyperionCrossModalAttnCompute(attn,
                                          queryFeatures, keyFeatures, valueFeatures,
                                          queryDim, keyDim, valueDim,
                                          queryLen, keyLen,
                                          output, outputDim, mask);
    assert(success);
    
    /* Clean up */
    free(queryFeatures);
    free(keyFeatures);
    free(valueFeatures);
    free(output);
    hyperionAttnMaskFree(mask);
    hyperionCrossModalAttnFree(attn);
    
    printf("✓ Cross-modal attention computation test passed\n");
    return 0;
}

/* Test bidirectional cross-modal attention */
int test_bidirectional_attention() {
    printf("Testing bidirectional cross-modal attention...\n");
    
    CrossModalAttnConfig config = {
        .numHeads = 2,
        .headDim = 16,
        .maxSeqLen = 32,
        .dropoutRate = 0.0f,
        .useLayerNorm = false,
        .useResidual = false,
        .useQuantization = false,
        .useSIMD = false
    };
    
    CrossModalAttention *attn = hyperionCrossModalAttnCreate(&config);
    assert(attn != NULL);
    
    /* Create synthetic data for two modalities */
    int len1 = 6, len2 = 8;
    int dim1 = 64, dim2 = 64;
    
    float *features1 = (float*)calloc(len1 * dim1, sizeof(float));
    float *features2 = (float*)calloc(len2 * dim2, sizeof(float));
    float *output1 = (float*)calloc(len1 * dim1, sizeof(float));
    float *output2 = (float*)calloc(len2 * dim2, sizeof(float));
    
    assert(features1 && features2 && output1 && output2);
    
    /* Initialize with different patterns */
    for (int i = 0; i < len1 * dim1; i++) {
        features1[i] = sinf((float)i * 0.1f);
    }
    for (int i = 0; i < len2 * dim2; i++) {
        features2[i] = cosf((float)i * 0.1f);
    }
    
    /* Compute bidirectional attention */
    bool success = hyperionCrossModalAttnBidirectional(attn,
                                                     features1, features2,
                                                     dim1, dim2, len1, len2,
                                                     output1, output2, NULL);
    assert(success);
    
    /* Verify outputs are different from inputs */
    bool output1_changed = false, output2_changed = false;
    
    for (int i = 0; i < len1 * dim1; i++) {
        if (fabsf(output1[i] - features1[i]) > 1e-6f) {
            output1_changed = true;
            break;
        }
    }
    
    for (int i = 0; i < len2 * dim2; i++) {
        if (fabsf(output2[i] - features2[i]) > 1e-6f) {
            output2_changed = true;
            break;
        }
    }
    
    assert(output1_changed);
    assert(output2_changed);
    
    /* Clean up */
    free(features1);
    free(features2);
    free(output1);
    free(output2);
    hyperionCrossModalAttnFree(attn);
    
    printf("✓ Bidirectional attention test passed\n");
    return 0;
}

/* Test attention mask functionality */
int test_attention_mask() {
    printf("Testing attention mask functionality...\n");
    
    int rows = 5, cols = 8;
    AttentionMask *mask = hyperionAttnMaskCreate(rows, cols);
    assert(mask != NULL);
    assert(mask->rows == rows);
    assert(mask->cols == cols);
    assert(mask->mask != NULL);
    
    /* Test initial state (all true) */
    for (int i = 0; i < rows * cols; i++) {
        assert(mask->mask[i] == true);
    }
    
    /* Test causal mask */
    bool success = hyperionAttnMaskSetCausal(mask);
    assert(success);
    
    /* Verify causal mask properties */
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            bool expected = (j <= i);
            bool actual = mask->mask[i * cols + j];
            assert(actual == expected);
        }
    }
    
    /* Test padding mask */
    int paddingPositions[] = {3, 4, 7};
    int numPadding = 3;
    
    success = hyperionAttnMaskSetPadding(mask, paddingPositions, numPadding);
    assert(success);
    
    hyperionAttnMaskFree(mask);
    
    printf("✓ Attention mask test passed\n");
    return 0;
}

/* Test temporal context functionality */
int test_temporal_context() {
    printf("Testing temporal context functionality...\n");
    
    int maxLength = 10;
    int hiddenDim = 32;
    
    TemporalContext *context = hyperionTemporalContextCreate(maxLength, hiddenDim);
    assert(context != NULL);
    assert(context->maxLength == maxLength);
    assert(context->sequenceLength == 0);
    assert(context->hiddenStates != NULL);
    assert(context->positions != NULL);
    
    /* Test updating temporal context */
    float *newHidden = (float*)malloc(hiddenDim * sizeof(float));
    assert(newHidden != NULL);
    
    for (int step = 0; step < 5; step++) {
        /* Create synthetic hidden state */
        for (int i = 0; i < hiddenDim; i++) {
            newHidden[i] = (float)step + (float)i * 0.1f;
        }
        
        bool success = hyperionTemporalContextUpdate(context, newHidden, step, hiddenDim);
        assert(success);
        assert(context->sequenceLength == step + 1);
        
        /* Verify stored values */
        for (int i = 0; i < hiddenDim; i++) {
            float expected = (float)step + (float)i * 0.1f;
            float actual = context->hiddenStates[step * hiddenDim + i];
            assert(fabsf(actual - expected) < 1e-6f);
        }
        assert(context->positions[step] == step);
    }
    
    free(newHidden);
    hyperionTemporalContextFree(context);
    
    printf("✓ Temporal context test passed\n");
    return 0;
}

/* Test vision-language integration creation */
int test_vision_language_integration() {
    printf("Testing vision-language integration...\n");
    
    VisionLanguageConfig config = {
        .visualFeatureDim = 2048,
        .textFeatureDim = 768,
        .fusedDim = 512,
        .maxRegions = 100,
        .maxTextLength = 256,
        .useRegionAttention = true,
        .useSpatialReasoning = true,
        .useHierarchical = true,
        .attnConfig = {
            .numHeads = 8,
            .headDim = 64,
            .maxSeqLen = 256,
            .dropoutRate = 0.1f,
            .useLayerNorm = true,
            .useResidual = true,
            .useQuantization = false,
            .useSIMD = true
        }
    };
    
    VisionLanguageIntegration *vl = hyperionVisionLanguageCreate(&config);
    assert(vl != NULL);
    
    /* Test memory usage query */
    size_t weightMemory, activationMemory;
    bool success = hyperionVisionLanguageGetMemoryUsage(vl, &weightMemory, &activationMemory);
    assert(success);
    
    printf("  - Vision-Language Weight memory: %zu bytes\n", weightMemory);
    printf("  - Vision-Language Activation memory: %zu bytes\n", activationMemory);
    
    hyperionVisionLanguageFree(vl);
    
    printf("✓ Vision-language integration test passed\n");
    return 0;
}

/* Test visual reasoning context */
int test_visual_reasoning_context() {
    printf("Testing visual reasoning context...\n");
    
    int maxRegions = 10;
    int featureDim = 256;
    
    VisualReasoningContext *context = hyperionVisualReasoningContextCreate(maxRegions, featureDim);
    assert(context != NULL);
    assert(context->numRegions == 0);
    assert(context->featureDim == featureDim);
    assert(context->regions != NULL);
    assert(context->regionFeatures != NULL);
    
    /* Add some test regions */
    for (int i = 0; i < 3; i++) {
        VisualRegion region = {
            .x = i * 50,
            .y = i * 30,
            .width = 100,
            .height = 80,
            .confidence = 0.8f + i * 0.05f,
            .featureIndex = i
        };
        
        float *features = (float*)malloc(featureDim * sizeof(float));
        assert(features != NULL);
        
        for (int j = 0; j < featureDim; j++) {
            features[j] = (float)i + (float)j * 0.01f;
        }
        
        bool success = hyperionVisualReasoningContextAddRegion(context, &region, features);
        assert(success);
        assert(context->numRegions == i + 1);
        
        free(features);
    }
    
    /* Compute spatial relations */
    bool success = hyperionVisualReasoningContextComputeRelations(context);
    assert(success);
    
    hyperionVisualReasoningContextFree(context);
    
    printf("✓ Visual reasoning context test passed\n");
    return 0;
}

/* Performance benchmark for cross-modal attention */
int benchmark_cross_modal_attention() {
    printf("Benchmarking cross-modal attention performance...\n");
    
    CrossModalAttnConfig config = {
        .numHeads = 16,
        .headDim = 64,
        .maxSeqLen = 1024,
        .dropoutRate = 0.1f,
        .useLayerNorm = true,
        .useResidual = true,
        .useQuantization = true,  /* Test with quantization */
        .useSIMD = true          /* Test with SIMD */
    };
    
    CrossModalAttention *attn = hyperionCrossModalAttnCreate(&config);
    assert(attn != NULL);
    
    /* Large-scale synthetic data */
    int queryLen = 256, keyLen = 512;
    int queryDim = 1024, keyDim = 1024, valueDim = 1024;
    int outputDim = 1024;
    
    float *queryFeatures = (float*)calloc(queryLen * queryDim, sizeof(float));
    float *keyFeatures = (float*)calloc(keyLen * keyDim, sizeof(float));
    float *valueFeatures = (float*)calloc(keyLen * valueDim, sizeof(float));
    float *output = (float*)calloc(queryLen * outputDim, sizeof(float));
    
    assert(queryFeatures && keyFeatures && valueFeatures && output);
    
    /* Initialize with random data */
    for (int i = 0; i < queryLen * queryDim; i++) {
        queryFeatures[i] = (float)rand() / RAND_MAX - 0.5f;
    }
    for (int i = 0; i < keyLen * keyDim; i++) {
        keyFeatures[i] = (float)rand() / RAND_MAX - 0.5f;
        valueFeatures[i] = (float)rand() / RAND_MAX - 0.5f;
    }
    
    /* Benchmark multiple runs */
    int numRuns = 10;
    clock_t start = clock();
    
    for (int run = 0; run < numRuns; run++) {
        bool success = hyperionCrossModalAttnCompute(attn,
                                                   queryFeatures, keyFeatures, valueFeatures,
                                                   queryDim, keyDim, valueDim,
                                                   queryLen, keyLen,
                                                   output, outputDim, NULL);
        assert(success);
    }
    
    clock_t end = clock();
    double totalTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    double avgTime = totalTime / numRuns;
    
    printf("  - Average attention computation time: %.3f ms\n", avgTime * 1000);
    printf("  - Throughput: %.1f computations/sec\n", 1.0 / avgTime);
    
    /* Memory efficiency test */
    size_t weightMemory, activationMemory;
    hyperionCrossModalAttnGetMemoryUsage(attn, &weightMemory, &activationMemory);
    
    printf("  - Memory efficiency: %.2f MB total\n", (weightMemory + activationMemory) / (1024.0 * 1024.0));
    
    /* Clean up */
    free(queryFeatures);
    free(keyFeatures);
    free(valueFeatures);
    free(output);
    hyperionCrossModalAttnFree(attn);
    
    printf("✓ Cross-modal attention benchmark completed\n");
    return 0;
}

int main() {
    printf("========================================\n");
    printf("Hyperion Phase 5.1: Enhanced Multimodal Capabilities Test Suite\n");
    printf("========================================\n");
    
    int result = 0;
    
    /* Basic functionality tests */
    result += test_cross_modal_attention_basic();
    result += test_cross_modal_attention_computation();
    result += test_bidirectional_attention();
    result += test_attention_mask();
    result += test_temporal_context();
    
    /* Advanced features tests */
    result += test_vision_language_integration();
    result += test_visual_reasoning_context();
    
    /* Performance benchmarks */
    result += benchmark_cross_modal_attention();
    
    printf("\n========================================\n");
    if (result == 0) {
        printf("✅ All Phase 5.1 multimodal tests passed!\n");
        printf("Enhanced cross-modal attention and vision-language integration are working correctly.\n");
    } else {
        printf("❌ %d Phase 5.1 multimodal tests failed!\n", result);
    }
    printf("========================================\n");
    
    return result;
}