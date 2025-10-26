#include "rules_engine.h"
#include "memory.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Enhanced memory optimization constants
#define HYPERION_4BIT_EFFICIENCY_THRESHOLD 0.25f  // 4-bit should use 25% of 32-bit
#define HYPERION_MEMORY_FRAGMENTATION_THRESHOLD 0.3f  // 30% fragmentation warning
#define HYPERION_ALLOCATION_COUNT_WARNING 1000  // Warning for too many allocations

// Enhanced memory optimization analysis structure for validation
typedef struct {
    size_t totalAllocated;
    size_t peakUsage;
    size_t currentUsage;
    size_t fragmentationBytes;
    int allocationCount;
    float quantizationEfficiency;
    bool uses4BitQuantization;
    bool usesMemoryPool;
} HyperionMemoryOptimizationAnalysis;

// Forward declarations
static HyperionRuleResult analyzeMemoryUsage(const HyperionRuleContext* context, 
                                           HyperionMemoryOptimizationAnalysis* analysis,
                                           char* errorMessage, 
                                           size_t errorMessageSize);
static HyperionRuleResult validateQuantizationEfficiency(const HyperionMemoryOptimizationAnalysis* analysis,
                                                        char* errorMessage,
                                                        size_t errorMessageSize);
static HyperionRuleResult validateMemoryFragmentation(const HyperionMemoryOptimizationAnalysis* analysis,
                                                     char* errorMessage,
                                                     size_t errorMessageSize);
static HyperionRuleResult validateAllocationPatterns(const HyperionMemoryOptimizationAnalysis* analysis,
                                                    char* errorMessage,
                                                    size_t errorMessageSize);

HyperionRuleResult hyperionValidateMemoryOptimizationEnhanced(const HyperionRuleContext* context,
                                                            char* errorMessage,
                                                            size_t errorMessageSize) {
    if (!context) {
        snprintf(errorMessage, errorMessageSize, "Invalid context");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    HyperionMemoryOptimizationAnalysis analysis = {0};
    
    // Perform comprehensive memory analysis
    HyperionRuleResult analysisResult = analyzeMemoryUsage(context, &analysis, errorMessage, errorMessageSize);
    if (analysisResult != HYPERION_RULE_RESULT_PASS) {
        return analysisResult;
    }
    
    HyperionRuleResult worstResult = HYPERION_RULE_RESULT_PASS;
    char tempMessage[256];
    
    // Validate quantization efficiency
    HyperionRuleResult quantResult = validateQuantizationEfficiency(&analysis, tempMessage, sizeof(tempMessage));
    if (quantResult > worstResult) {
        worstResult = quantResult;
        snprintf(errorMessage, errorMessageSize, "Quantization: %s", tempMessage);
    }
    
    // Validate memory fragmentation
    HyperionRuleResult fragResult = validateMemoryFragmentation(&analysis, tempMessage, sizeof(tempMessage));
    if (fragResult > worstResult) {
        worstResult = fragResult;
        snprintf(errorMessage, errorMessageSize, "Fragmentation: %s", tempMessage);
    }
    
    // Validate allocation patterns
    HyperionRuleResult allocResult = validateAllocationPatterns(&analysis, tempMessage, sizeof(tempMessage));
    if (allocResult > worstResult) {
        worstResult = allocResult;
        snprintf(errorMessage, errorMessageSize, "Allocation: %s", tempMessage);
    }
    
    // Check against memory targets
    size_t targetMemory = context->isEmbedded ? 
        (32 * 1024) :      // 32KB for embedded
        (100 * 1024 * 1024); // 100MB for main models
    
    if (analysis.peakUsage > targetMemory) {
        snprintf(errorMessage, errorMessageSize, 
                "Peak memory usage %zu exceeds %s target %zu (efficiency: %.2f%%)",
                analysis.peakUsage, 
                context->isEmbedded ? "embedded" : "main",
                targetMemory,
                analysis.quantizationEfficiency * 100.0f);
        return HYPERION_RULE_RESULT_FAIL;
    }
    
    // Check for significant memory increase
    if (context->memoryDelta > targetMemory * 0.1) {
        snprintf(errorMessage, errorMessageSize,
                "Memory delta %zu exceeds 10%% of target (%zu bytes)",
                context->memoryDelta, targetMemory / 10);
        if (worstResult < HYPERION_RULE_RESULT_WARNING) {
            worstResult = HYPERION_RULE_RESULT_WARNING;
        }
    }
    
    return worstResult;
}

static HyperionRuleResult analyzeMemoryUsage(const HyperionRuleContext* context, 
                                           HyperionMemoryOptimizationAnalysis* analysis,
                                           char* errorMessage, 
                                           size_t errorMessageSize) {
    if (!analysis) {
        snprintf(errorMessage, errorMessageSize, "Invalid analysis structure");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Initialize analysis
    memset(analysis, 0, sizeof(*analysis));
    
    // Get current memory usage from context
    analysis->currentUsage = context->memoryUsage;
    analysis->totalAllocated = context->memoryUsage + context->memoryDelta;
    analysis->peakUsage = analysis->totalAllocated; // Simplified
    
    // Check if function/file name indicates 4-bit quantization usage
    if (context->functionName) {
        analysis->uses4BitQuantization = 
            strstr(context->functionName, "quant") != NULL ||
            strstr(context->functionName, "4bit") != NULL ||
            strstr(context->functionName, "quantize") != NULL;
    }
    
    if (context->filePath) {
        analysis->uses4BitQuantization = analysis->uses4BitQuantization ||
            strstr(context->filePath, "quantize") != NULL ||
            strstr(context->filePath, "quant") != NULL;
    }
    
    // Check if using memory pool
    analysis->usesMemoryPool = 
        (context->functionName && strstr(context->functionName, "pool") != NULL) ||
        (context->filePath && strstr(context->filePath, "pool") != NULL);
    
    // Estimate quantization efficiency
    if (analysis->uses4BitQuantization) {
        // 4-bit quantization should achieve ~4x reduction (25% of original)
        analysis->quantizationEfficiency = HYPERION_4BIT_EFFICIENCY_THRESHOLD;
    } else {
        // Assume no quantization optimization
        analysis->quantizationEfficiency = 1.0f;
    }
    
    // Simulate allocation count based on memory usage
    analysis->allocationCount = (int)(analysis->totalAllocated / 1024); // Rough estimate
    
    // Simulate fragmentation (would be measured in real implementation)
    if (!analysis->usesMemoryPool) {
        analysis->fragmentationBytes = analysis->totalAllocated * 0.1f; // 10% fragmentation
    } else {
        analysis->fragmentationBytes = analysis->totalAllocated * 0.02f; // 2% with pooling
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

static HyperionRuleResult validateQuantizationEfficiency(const HyperionMemoryOptimizationAnalysis* analysis,
                                                        char* errorMessage,
                                                        size_t errorMessageSize) {
    if (!analysis) {
        snprintf(errorMessage, errorMessageSize, "Invalid analysis");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Check if model operations should use quantization
    if (analysis->totalAllocated > (10 * 1024 * 1024)) { // >10MB suggests model data
        if (!analysis->uses4BitQuantization) {
            snprintf(errorMessage, errorMessageSize,
                    "Large memory usage (%zu bytes) without 4-bit quantization",
                    analysis->totalAllocated);
            return HYPERION_RULE_RESULT_WARNING;
        }
        
        // Validate quantization efficiency
        if (analysis->quantizationEfficiency > HYPERION_4BIT_EFFICIENCY_THRESHOLD * 1.5f) {
            snprintf(errorMessage, errorMessageSize,
                    "Quantization efficiency %.2f%% below expected 25%% target",
                    analysis->quantizationEfficiency * 100.0f);
            return HYPERION_RULE_RESULT_WARNING;
        }
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

static HyperionRuleResult validateMemoryFragmentation(const HyperionMemoryOptimizationAnalysis* analysis,
                                                     char* errorMessage,
                                                     size_t errorMessageSize) {
    if (!analysis) {
        snprintf(errorMessage, errorMessageSize, "Invalid analysis");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    if (analysis->totalAllocated == 0) {
        return HYPERION_RULE_RESULT_PASS;
    }
    
    float fragmentationRatio = (float)analysis->fragmentationBytes / analysis->totalAllocated;
    
    if (fragmentationRatio > HYPERION_MEMORY_FRAGMENTATION_THRESHOLD) {
        snprintf(errorMessage, errorMessageSize,
                "Memory fragmentation %.1f%% exceeds threshold %.1f%% (%zu wasted bytes)",
                fragmentationRatio * 100.0f,
                HYPERION_MEMORY_FRAGMENTATION_THRESHOLD * 100.0f,
                analysis->fragmentationBytes);
        return HYPERION_RULE_RESULT_WARNING;
    }
    
    // Recommend memory pooling for high fragmentation
    if (fragmentationRatio > 0.1f && !analysis->usesMemoryPool) {
        snprintf(errorMessage, errorMessageSize,
                "Consider memory pooling to reduce %.1f%% fragmentation",
                fragmentationRatio * 100.0f);
        return HYPERION_RULE_RESULT_WARNING;
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

static HyperionRuleResult validateAllocationPatterns(const HyperionMemoryOptimizationAnalysis* analysis,
                                                    char* errorMessage,
                                                    size_t errorMessageSize) {
    if (!analysis) {
        snprintf(errorMessage, errorMessageSize, "Invalid analysis");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Check for excessive allocation count
    if (analysis->allocationCount > HYPERION_ALLOCATION_COUNT_WARNING) {
        snprintf(errorMessage, errorMessageSize,
                "High allocation count %d may impact performance",
                analysis->allocationCount);
        return HYPERION_RULE_RESULT_WARNING;
    }
    
    // Check allocation size patterns
    if (analysis->totalAllocated > 0 && analysis->allocationCount > 0) {
        size_t avgAllocationSize = analysis->totalAllocated / analysis->allocationCount;
        
        // Very small allocations suggest inefficient patterns
        if (avgAllocationSize < 64) {
            snprintf(errorMessage, errorMessageSize,
                    "Small average allocation size %zu bytes suggests inefficient patterns",
                    avgAllocationSize);
            return HYPERION_RULE_RESULT_WARNING;
        }
        
        // Very large allocations might indicate missing streaming/chunking
        if (avgAllocationSize > (1024 * 1024)) { // >1MB average
            snprintf(errorMessage, errorMessageSize,
                    "Large average allocation size %zu bytes, consider streaming/chunking",
                    avgAllocationSize);
            return HYPERION_RULE_RESULT_WARNING;
        }
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

// Additional utility functions for memory optimization
bool hyperionMemoryOptimizationCheckQuantization(const char* filePath, const char* functionName) {
    if (!filePath && !functionName) {
        return false;
    }
    
    const char* quantizationIndicators[] = {
        "quant", "4bit", "quantize", "q4", "int4", "quantized"
    };
    
    for (size_t i = 0; i < sizeof(quantizationIndicators) / sizeof(quantizationIndicators[0]); i++) {
        if ((filePath && strstr(filePath, quantizationIndicators[i]) != NULL) ||
            (functionName && strstr(functionName, quantizationIndicators[i]) != NULL)) {
            return true;
        }
    }
    
    return false;
}

bool hyperionMemoryOptimizationCheckPoolUsage(const char* filePath, const char* functionName) {
    if (!filePath && !functionName) {
        return false;
    }
    
    const char* poolIndicators[] = {
        "pool", "arena", "allocator", "chunk", "block"
    };
    
    for (size_t i = 0; i < sizeof(poolIndicators) / sizeof(poolIndicators[0]); i++) {
        if ((filePath && strstr(filePath, poolIndicators[i]) != NULL) ||
            (functionName && strstr(functionName, poolIndicators[i]) != NULL)) {
            return true;
        }
    }
    
    return false;
}

float hyperionMemoryOptimizationEstimateEfficiency(size_t originalSize, size_t optimizedSize) {
    if (originalSize == 0) {
        return 1.0f;
    }
    
    return (float)optimizedSize / originalSize;
}

// Memory target validation for different platforms
bool hyperionMemoryOptimizationValidateTarget(size_t memoryUsage, 
                                             bool isEmbedded, 
                                             bool isMobile,
                                             bool isCloud) {
    if (isEmbedded) {
        return memoryUsage <= (32 * 1024); // 32KB for embedded
    } else if (isMobile) {
        return memoryUsage <= (50 * 1024 * 1024); // 50MB for mobile
    } else if (isCloud) {
        return memoryUsage <= (500 * 1024 * 1024); // 500MB for cloud (more flexible)
    } else {
        return memoryUsage <= (100 * 1024 * 1024); // 100MB for desktop
    }
}