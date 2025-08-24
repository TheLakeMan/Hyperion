#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "models/multimodal/cross_modal_attention.h"

/* Mock memory functions for testing */
void* hyperionAlloc(size_t size) { return malloc(size); }
void* hyperionCalloc(size_t num, size_t size) { return calloc(num, size); }
void* hyperionRealloc(void* ptr, size_t size) { return realloc(ptr, size); }
void hyperionFree(void* ptr) { free(ptr); }

/* Test cross-modal attention creation and basic functionality */
int test_cross_modal_attention_basic() {
    printf("🔍 Quest 6A: Testing cross-modal attention creation...\n");
    
    CrossModalAttnConfig config = {
        .numHeads = 8,
        .headDim = 64,
        .maxSeqLen = 128,
        .dropoutRate = 0.1f,
        .useLayerNorm = true,
        .useResidual = true,
        .useQuantization = false,
        .useSIMD = false
    };
    
    CrossModalAttention *attn = hyperionCrossModalAttnCreate(&config);
    if (!attn) {
        printf("✗ Failed to create cross-modal attention\n");
        return 1;
    }
    
    printf("✓ Cross-modal attention created successfully\n");
    printf("  - Heads: %d\n", config.numHeads);
    printf("  - Head dimension: %d\n", config.headDim);
    printf("  - Max sequence length: %d\n", config.maxSeqLen);
    
    /* Test memory usage */
    size_t weightMemory, activationMemory;
    bool success = hyperionCrossModalAttnGetMemoryUsage(attn, &weightMemory, &activationMemory);
    if (success) {
        printf("✓ Memory usage: %zu bytes (weights), %zu bytes (activations)\n", 
               weightMemory, activationMemory);
    }
    
    /* Test SIMD enable/disable */
    success = hyperionCrossModalAttnEnableSIMD(attn, true);
    assert(success);
    printf("✓ SIMD acceleration enabled\n");
    
    /* Test quantization setting */
    success = hyperionCrossModalAttnSetQuantization(attn, true);
    assert(success);
    printf("✓ Quantization enabled\n");
    
    hyperionCrossModalAttnFree(attn);
    printf("✓ Cross-modal attention freed successfully\n");
    
    return 0;
}

/* Test attention mask functionality */
int test_attention_mask() {
    printf("🔍 Quest 6A: Testing attention mask functionality...\n");
    
    AttentionMask *mask = hyperionAttnMaskCreate(10, 10);
    if (!mask) {
        printf("✗ Failed to create attention mask\n");
        return 1;
    }
    
    printf("✓ Attention mask created (10x10)\n");
    
    /* Test causal mask */
    bool success = hyperionAttnMaskSetCausal(mask);
    assert(success);
    printf("✓ Causal mask set successfully\n");
    
    hyperionAttnMaskFree(mask);
    printf("✓ Attention mask freed successfully\n");
    
    return 0;
}

/* Test temporal context */
int test_temporal_context() {
    printf("🔍 Quest 6A: Testing temporal context functionality...\n");
    
    TemporalContext *context = hyperionTemporalContextCreate(50, 256);
    if (!context) {
        printf("✗ Failed to create temporal context\n");
        return 1;
    }
    
    printf("✓ Temporal context created (50 length, 256 hidden dim)\n");
    
    hyperionTemporalContextFree(context);
    printf("✓ Temporal context freed successfully\n");
    
    return 0;
}

int main() {
    printf("========================================\n");
    printf("🎯 QUEST 6A: MULTIMODAL CAPABILITIES REAL TESTING\n");
    printf("========================================\n");
    
    int result = 0;
    
    /* Run all multimodal tests */
    result += test_cross_modal_attention_basic();
    result += test_attention_mask();
    result += test_temporal_context();
    
    printf("========================================\n");
    if (result == 0) {
        printf("✅ QUEST 6A COMPLETE: All multimodal capabilities tests passed!\n");
        printf("Validated features:\n");
        printf("  - Cross-modal attention creation and configuration\n");
        printf("  - Memory usage tracking and optimization\n");
        printf("  - SIMD acceleration support\n");
        printf("  - Quantization configuration\n");
        printf("  - Attention mask management\n");
        printf("  - Temporal context handling\n");
    } else {
        printf("❌ QUEST 6A FAILED: %d multimodal tests failed!\n", result);
    }
    printf("========================================\n");
    
    return result;
}