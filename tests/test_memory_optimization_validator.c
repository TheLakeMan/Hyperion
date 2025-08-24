#include "core/rules_engine.h"
#include "core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test function declarations
static int test_basic_memory_validation(void);
static int test_4bit_quantization_detection(void);
static int test_memory_fragmentation_analysis(void);
static int test_allocation_pattern_analysis(void);
static int test_memory_target_validation(void);

int main() {
    printf("=== Hyperion Memory Optimization Validator Tests ===\n\n");
    
    // Initialize the rules engine
    if (!hyperionRulesEngineInit()) {
        printf("ERROR: Failed to initialize rules engine\n");
        return 1;
    }
    
    int passed = 0;
    int total = 0;
    
    // Run tests
    total++; if (test_basic_memory_validation() == 0) passed++;
    total++; if (test_4bit_quantization_detection() == 0) passed++;
    total++; if (test_memory_fragmentation_analysis() == 0) passed++;
    total++; if (test_allocation_pattern_analysis() == 0) passed++;
    total++; if (test_memory_target_validation() == 0) passed++;
    
    printf("\n=== Test Results: %d/%d tests passed ===\n", passed, total);
    
    // Cleanup
    hyperionRulesEngineCleanup();
    
    return (passed == total) ? 0 : 1;
}

static int test_basic_memory_validation(void) {
    printf("Testing basic memory validation...\n");
    
    HyperionRuleContext context = {
        .filePath = "test_model.c",
        .functionName = "test_function",
        .memoryUsage = 50 * 1024 * 1024, // 50MB
        .memoryDelta = 5 * 1024 * 1024,  // 5MB increase
        .isEmbedded = false,
        .usesSIMD = false,
        .description = "Test model function"
    };
    
    char errorMessage[512];
    HyperionRuleResult result = hyperionValidateMemoryOptimization(&context, errorMessage, sizeof(errorMessage));
    
    if (result == HYPERION_RULE_RESULT_PASS) {
        printf("  PASS: Basic memory validation passed\n");
        return 0;
    } else {
        printf("  FAIL: Basic memory validation failed: %s\n", errorMessage);
        return 1;
    }
}

static int test_4bit_quantization_detection(void) {
    printf("Testing 4-bit quantization detection...\n");
    
    HyperionRuleContext context = {
        .filePath = "models/quantized_model.c",
        .functionName = "quantize_weights",
        .memoryUsage = 15 * 1024 * 1024, // 15MB
        .memoryDelta = 0,
        .isEmbedded = false,
        .usesSIMD = false,
        .description = "Quantization function"
    };
    
    char errorMessage[512];
    HyperionRuleResult result = hyperionValidateMemoryOptimization(&context, errorMessage, sizeof(errorMessage));
    
    // This should pass since it's a quantization function
    if (result == HYPERION_RULE_RESULT_PASS) {
        printf("  PASS: 4-bit quantization detection working correctly\n");
        return 0;
    } else {
        printf("  FAIL: 4-bit quantization detection failed: %s\n", errorMessage);
        return 1;
    }
}

static int test_memory_fragmentation_analysis(void) {
    printf("Testing memory fragmentation analysis...\n");
    
    // Test the enhanced memory optimization validator directly
    HyperionRuleContext context = {
        .filePath = "core/memory_intensive.c",
        .functionName = "process_large_data",
        .memoryUsage = 80 * 1024 * 1024, // 80MB
        .memoryDelta = 10 * 1024 * 1024, // 10MB increase
        .isEmbedded = false,
        .usesSIMD = false,
        .description = "Memory intensive function"
    };
    
    char errorMessage[512];
    HyperionRuleResult result = hyperionValidateMemoryOptimizationEnhanced(&context, errorMessage, sizeof(errorMessage));
    
    // Should at least pass or give a warning, not fail
    if (result != HYPERION_RULE_RESULT_ERROR) {
        printf("  PASS: Memory fragmentation analysis completed\n");
        return 0;
    } else {
        printf("  FAIL: Memory fragmentation analysis failed: %s\n", errorMessage);
        return 1;
    }
}

static int test_allocation_pattern_analysis(void) {
    printf("Testing allocation pattern analysis...\n");
    
    HyperionRuleContext context = {
        .filePath = "utils/memory_utils.c",
        .functionName = "allocate_blocks",
        .memoryUsage = 5 * 1024 * 1024, // 5MB
        .memoryDelta = 100 * 1024,      // 100KB increase
        .isEmbedded = false,
        .usesSIMD = false,
        .description = "Memory allocation function"
    };
    
    char errorMessage[512];
    HyperionRuleResult result = hyperionValidateMemoryOptimizationEnhanced(&context, errorMessage, sizeof(errorMessage));
    
    // Should at least pass or give a warning, not fail
    if (result != HYPERION_RULE_RESULT_ERROR) {
        printf("  PASS: Allocation pattern analysis completed\n");
        return 0;
    } else {
        printf("  FAIL: Allocation pattern analysis failed: %s\n", errorMessage);
        return 1;
    }
}

static int test_memory_target_validation(void) {
    printf("Testing memory target validation...\n");
    
    // Test exceeding embedded memory target
    HyperionRuleContext context = {
        .filePath = "embedded/embedded_model.c",
        .functionName = "embedded_inference",
        .memoryUsage = 50 * 1024, // 50KB - exceeds 32KB embedded target
        .memoryDelta = 0,
        .isEmbedded = true,
        .usesSIMD = false,
        .description = "Embedded model function"
    };
    
    char errorMessage[512];
    HyperionRuleResult result = hyperionValidateMemoryOptimization(&context, errorMessage, sizeof(errorMessage));
    
    if (result == HYPERION_RULE_RESULT_FAIL) {
        printf("  PASS: Memory target validation correctly detected overflow\n");
        return 0;
    } else {
        printf("  FAIL: Memory target validation should have failed but didn't\n");
        return 1;
    }
}