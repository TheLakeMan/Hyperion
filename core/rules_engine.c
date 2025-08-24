#include "rules_engine.h"
#include "memory.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Maximum number of rules that can be registered
#define MAX_RULES 32

// Memory targets for different platforms
#define EMBEDDED_MEMORY_TARGET (32 * 1024)      // 32KB for embedded
#define MAIN_MEMORY_TARGET (100 * 1024 * 1024)  // 100MB for main models

// Global rules registry
static HyperionRule g_rules[MAX_RULES];
static int g_numRules = 0;
static bool g_rulesEngineInitialized = false;
static HyperionRulesEngineStats g_stats = {0};

// Forward declarations
static void initializeDefaultRules(void);
static const char* ruleTypeToString(HyperionRuleType type);
static const char* ruleResultToString(HyperionRuleResult result);

bool hyperionRulesEngineInit(void) {
    if (g_rulesEngineInitialized) {
        return true;
    }
    
    // Reset stats
    memset(&g_stats, 0, sizeof(g_stats));
    
    // Initialize default rules
    initializeDefaultRules();
    
    g_rulesEngineInitialized = true;
    
    hyperion_log(HYPERION_LOG_INFO, "Rules engine initialized with %d default rules", g_numRules);
    return true;
}

void hyperionRulesEngineCleanup(void) {
    if (!g_rulesEngineInitialized) {
        return;
    }
    
    g_numRules = 0;
    g_rulesEngineInitialized = false;
    
    hyperion_log(HYPERION_LOG_INFO, "Rules engine cleaned up");
}

bool hyperionRulesEngineAddRule(const HyperionRule* rule) {
    if (!g_rulesEngineInitialized || !rule || g_numRules >= MAX_RULES) {
        return false;
    }
    
    // Check if rule already exists
    for (int i = 0; i < g_numRules; i++) {
        if (g_rules[i].type == rule->type) {
            hyperion_log(HYPERION_LOG_WARN, "Rule type %s already exists, replacing", 
                             ruleTypeToString(rule->type));
            g_rules[i] = *rule;
            return true;
        }
    }
    
    // Add new rule
    g_rules[g_numRules] = *rule;
    g_numRules++;
    g_stats.totalRules++;
    
    if (rule->enabled) {
        g_stats.enabledRules++;
    }
    
    hyperion_log(HYPERION_LOG_INFO, "Added rule: %s", rule->name);
    return true;
}

bool hyperionRulesEngineRemoveRule(HyperionRuleType type) {
    if (!g_rulesEngineInitialized) {
        return false;
    }
    
    for (int i = 0; i < g_numRules; i++) {
        if (g_rules[i].type == type) {
            // Shift remaining rules down
            for (int j = i; j < g_numRules - 1; j++) {
                g_rules[j] = g_rules[j + 1];
            }
            g_numRules--;
            g_stats.totalRules--;
            
            hyperion_log(HYPERION_LOG_INFO, "Removed rule type: %s", ruleTypeToString(type));
            return true;
        }
    }
    
    return false;
}

bool hyperionRulesEngineEnableRule(HyperionRuleType type, bool enabled) {
    if (!g_rulesEngineInitialized) {
        return false;
    }
    
    for (int i = 0; i < g_numRules; i++) {
        if (g_rules[i].type == type) {
            bool wasEnabled = g_rules[i].enabled;
            g_rules[i].enabled = enabled;
            
            if (enabled && !wasEnabled) {
                g_stats.enabledRules++;
            } else if (!enabled && wasEnabled) {
                g_stats.enabledRules--;
            }
            
            hyperion_log(HYPERION_LOG_INFO, "Rule %s %s", ruleTypeToString(type), 
                          enabled ? "enabled" : "disabled");
            return true;
        }
    }
    
    return false;
}

HyperionRuleResult hyperionRulesEngineValidate(HyperionRuleTrigger trigger,
                                               const HyperionRuleContext* context) {
    if (!g_rulesEngineInitialized || !context) {
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    HyperionRuleResult worstResult = HYPERION_RULE_RESULT_PASS;
    char errorMessage[512];
    
    for (int i = 0; i < g_numRules; i++) {
        HyperionRule* rule = &g_rules[i];
        
        if (!rule->enabled || !rule->validator) {
            continue;
        }
        
        // Check if rule is triggered by this trigger
        bool triggered = false;
        for (int j = 0; j < rule->numTriggers; j++) {
            if (rule->triggers[j] == trigger) {
                triggered = true;
                break;
            }
        }
        
        if (!triggered) {
            continue;
        }
        
        // Validate rule
        HyperionRuleResult result = rule->validator(context, errorMessage, sizeof(errorMessage));
        
        // Update statistics
        switch (result) {
            case HYPERION_RULE_RESULT_PASS:
                g_stats.passedValidations++;
                break;
            case HYPERION_RULE_RESULT_WARNING:
                g_stats.warnings++;
                hyperion_log(HYPERION_LOG_WARN, "Rule %s: %s", rule->name, errorMessage);
                break;
            case HYPERION_RULE_RESULT_FAIL:
                g_stats.failedValidations++;
                hyperion_log(HYPERION_LOG_ERROR, "Rule %s failed: %s", rule->name, errorMessage);
                break;
            case HYPERION_RULE_RESULT_ERROR:
                g_stats.errors++;
                hyperion_log(HYPERION_LOG_ERROR, "Rule %s error: %s", rule->name, errorMessage);
                break;
        }
        
        // Track worst result
        if (result > worstResult) {
            worstResult = result;
        }
    }
    
    return worstResult;
}

HyperionRuleResult hyperionRulesEngineValidateAll(const HyperionRuleContext* context) {
    if (!g_rulesEngineInitialized || !context) {
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    HyperionRuleResult worstResult = HYPERION_RULE_RESULT_PASS;
    char errorMessage[512];
    
    for (int i = 0; i < g_numRules; i++) {
        HyperionRule* rule = &g_rules[i];
        
        if (!rule->enabled || !rule->validator) {
            continue;
        }
        
        HyperionRuleResult result = rule->validator(context, errorMessage, sizeof(errorMessage));
        
        // Update statistics (same as above)
        switch (result) {
            case HYPERION_RULE_RESULT_PASS:
                g_stats.passedValidations++;
                break;
            case HYPERION_RULE_RESULT_WARNING:
                g_stats.warnings++;
                hyperion_log(HYPERION_LOG_WARN, "Rule %s: %s", rule->name, errorMessage);
                break;
            case HYPERION_RULE_RESULT_FAIL:
                g_stats.failedValidations++;
                hyperion_log(HYPERION_LOG_ERROR, "Rule %s failed: %s", rule->name, errorMessage);
                break;
            case HYPERION_RULE_RESULT_ERROR:
                g_stats.errors++;
                hyperion_log(HYPERION_LOG_ERROR, "Rule %s error: %s", rule->name, errorMessage);
                break;
        }
        
        if (result > worstResult) {
            worstResult = result;
        }
    }
    
    return worstResult;
}

// Memory optimization validator
HyperionRuleResult hyperionValidateMemoryOptimization(const HyperionRuleContext* context,
                                                      char* errorMessage,
                                                      size_t errorMessageSize) {
    if (!context) {
        snprintf(errorMessage, errorMessageSize, "Invalid context");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Check memory targets
    size_t targetMemory = context->isEmbedded ? EMBEDDED_MEMORY_TARGET : MAIN_MEMORY_TARGET;
    
    if (context->memoryUsage > targetMemory) {
        snprintf(errorMessage, errorMessageSize, 
                "Memory usage %zu exceeds target %zu for %s platform",
                context->memoryUsage, targetMemory,
                context->isEmbedded ? "embedded" : "main");
        return HYPERION_RULE_RESULT_FAIL;
    }
    
    // Check for significant memory increase
    if (context->memoryDelta > targetMemory * 0.1) {  // More than 10% increase
        snprintf(errorMessage, errorMessageSize,
                "Memory delta %zu is significant (>10%% of target)",
                context->memoryDelta);
        return HYPERION_RULE_RESULT_WARNING;
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

// Platform compatibility validator
HyperionRuleResult hyperionValidatePlatformCompatibility(const HyperionRuleContext* context,
                                                        char* errorMessage,
                                                        size_t errorMessageSize) {
    if (!context || !context->filePath) {
        snprintf(errorMessage, errorMessageSize, "Invalid context or file path");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Check if file uses platform-specific features
    const char* platformSpecificPatterns[] = {
        "#ifdef _WIN32",
        "#ifdef __linux__", 
        "#ifdef __APPLE__",
        "windows.h",
        "unistd.h"
    };
    
    // This is a simplified check - in a real implementation,
    // we would parse the file contents
    for (size_t i = 0; i < sizeof(platformSpecificPatterns) / sizeof(platformSpecificPatterns[0]); i++) {
        // Warning for platform-specific code (should have proper abstractions)
        if (strstr(context->filePath, "platform") == NULL && 
            strstr(context->description ? context->description : "", platformSpecificPatterns[i]) != NULL) {
            snprintf(errorMessage, errorMessageSize,
                    "Platform-specific code detected: %s. Consider using abstractions.",
                    platformSpecificPatterns[i]);
            return HYPERION_RULE_RESULT_WARNING;
        }
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

// Quest completion validator
HyperionRuleResult hyperionValidateQuestCompletion(const HyperionRuleContext* context,
                                                   char* errorMessage,
                                                   size_t errorMessageSize) {
    if (!context || !context->filePath) {
        snprintf(errorMessage, errorMessageSize, "Invalid context or file path");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Check if file exists and has content
    if (!hyperionRulesEngineCheckFileExists(context->filePath)) {
        snprintf(errorMessage, errorMessageSize, "File does not exist: %s", context->filePath);
        return HYPERION_RULE_RESULT_FAIL;
    }
    
    // Check file size (should not be 0.0KB)
    struct stat st;
    if (stat(context->filePath, &st) == 0) {
        if (st.st_size == 0) {
            snprintf(errorMessage, errorMessageSize, "File is empty (0.0KB): %s", context->filePath);
            return HYPERION_RULE_RESULT_FAIL;
        }
        
        if (st.st_size < 100) {  // Very small files might be incomplete
            snprintf(errorMessage, errorMessageSize, 
                    "File is very small (%ld bytes), might be incomplete: %s", 
                    st.st_size, context->filePath);
            return HYPERION_RULE_RESULT_WARNING;
        }
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

// SIMD optimization validator
HyperionRuleResult hyperionValidateSIMDOptimization(const HyperionRuleContext* context,
                                                   char* errorMessage,
                                                   size_t errorMessageSize) {
    if (!context) {
        snprintf(errorMessage, errorMessageSize, "Invalid context");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Check if function name suggests matrix operations that could benefit from SIMD
    if (context->functionName) {
        const char* matrixOperations[] = {
            "multiply", "matmul", "conv", "dot", "gemm", "add", "scale"
        };
        
        for (size_t i = 0; i < sizeof(matrixOperations) / sizeof(matrixOperations[0]); i++) {
            if (strstr(context->functionName, matrixOperations[i]) != NULL) {
                if (!context->usesSIMD) {
                    snprintf(errorMessage, errorMessageSize,
                            "Function %s performs matrix operations but doesn't use SIMD",
                            context->functionName);
                    return HYPERION_RULE_RESULT_WARNING;
                }
                break;
            }
        }
    }
    
    return HYPERION_RULE_RESULT_PASS;
}

// Utility functions
bool hyperionRulesEngineCheckMemoryTarget(size_t memoryUsage, bool isEmbedded) {
    size_t target = isEmbedded ? EMBEDDED_MEMORY_TARGET : MAIN_MEMORY_TARGET;
    return memoryUsage <= target;
}

bool hyperionRulesEngineCheckPlatformSupport(const char* filePath) {
    // Simple check - files in platform-specific directories are allowed
    return strstr(filePath, "platform") != NULL || 
           strstr(filePath, "windows") != NULL ||
           strstr(filePath, "linux") != NULL ||
           strstr(filePath, "macos") != NULL;
}

bool hyperionRulesEngineCheckFileExists(const char* filePath) {
    if (!filePath) return false;
    
    struct stat st;
    return stat(filePath, &st) == 0;
}

bool hyperionRulesEngineCheckCompilation(const char* filePath) {
    // This would need to be implemented to actually run compilation
    // For now, just check if it's a C file
    return strstr(filePath, ".c") != NULL || strstr(filePath, ".h") != NULL;
}

bool hyperionRulesEngineCheckSIMDCompatibility(const char* code) {
    if (!code) return false;
    
    // Check for SIMD intrinsics
    const char* simdPatterns[] = {
        "_mm_", "_mm256_", "__m128", "__m256", "avx", "sse"
    };
    
    for (size_t i = 0; i < sizeof(simdPatterns) / sizeof(simdPatterns[0]); i++) {
        if (strstr(code, simdPatterns[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

// Statistics and reporting
void hyperionRulesEngineGetStats(HyperionRulesEngineStats* stats) {
    if (stats) {
        *stats = g_stats;
        stats->totalRules = g_numRules;
        
        // Count enabled rules
        stats->enabledRules = 0;
        for (int i = 0; i < g_numRules; i++) {
            if (g_rules[i].enabled) {
                stats->enabledRules++;
            }
        }
    }
}

void hyperionRulesEngineResetStats(void) {
    g_stats.passedValidations = 0;
    g_stats.failedValidations = 0;
    g_stats.warnings = 0;
    g_stats.errors = 0;
}

void hyperionRulesEnginePrintReport(void) {
    HyperionRulesEngineStats stats;
    hyperionRulesEngineGetStats(&stats);
    
    printf("\n=== HYPERION RULES ENGINE REPORT ===\n");
    printf("Total Rules: %d\n", stats.totalRules);
    printf("Enabled Rules: %d\n", stats.enabledRules);
    printf("Passed Validations: %d\n", stats.passedValidations);
    printf("Failed Validations: %d\n", stats.failedValidations);
    printf("Warnings: %d\n", stats.warnings);
    printf("Errors: %d\n", stats.errors);
    printf("=====================================\n\n");
}

// Initialize default rules
static void initializeDefaultRules(void) {
    // Memory optimization rule
    HyperionRule memoryRule = {
        .type = HYPERION_RULE_MEMORY_OPTIMIZATION,
        .name = "Memory Optimization",
        .description = "Ensures code modifications maintain memory efficiency targets",
        .triggers = {HYPERION_TRIGGER_CODE_MODIFICATION, HYPERION_TRIGGER_NEW_FEATURE, HYPERION_TRIGGER_MODEL_OPERATION},
        .numTriggers = 3,
        .validator = hyperionValidateMemoryOptimization,
        .enabled = true,
        .priority = 1
    };
    
    // Platform compatibility rule
    HyperionRule platformRule = {
        .type = HYPERION_RULE_PLATFORM_COMPATIBILITY,
        .name = "Platform Compatibility",
        .description = "Validates cross-platform compatibility",
        .triggers = {HYPERION_TRIGGER_CORE_MODIFICATION, HYPERION_TRIGGER_SIMD_CODE, HYPERION_TRIGGER_BUILD_CHANGE},
        .numTriggers = 3,
        .validator = hyperionValidatePlatformCompatibility,
        .enabled = true,
        .priority = 2
    };
    
    // Quest completion rule
    HyperionRule questRule = {
        .type = HYPERION_RULE_QUEST_VERIFICATION,
        .name = "Quest Completion Verification",
        .description = "Verifies quest completion with file existence and content validation",
        .triggers = {HYPERION_TRIGGER_PHASE_COMPLETE, HYPERION_TRIGGER_QUEST_COMPLETE},
        .numTriggers = 2,
        .validator = hyperionValidateQuestCompletion,
        .enabled = true,
        .priority = 1
    };
    
    // SIMD optimization rule
    HyperionRule simdRule = {
        .type = HYPERION_RULE_SIMD_OPTIMIZATION,
        .name = "SIMD Optimization",
        .description = "Ensures matrix operations use SIMD when possible",
        .triggers = {HYPERION_TRIGGER_SIMD_CODE, HYPERION_TRIGGER_CODE_MODIFICATION},
        .numTriggers = 2,
        .validator = hyperionValidateSIMDOptimization,
        .enabled = true,
        .priority = 3
    };
    
    // Add default rules
    hyperionRulesEngineAddRule(&memoryRule);
    hyperionRulesEngineAddRule(&platformRule);
    hyperionRulesEngineAddRule(&questRule);
    hyperionRulesEngineAddRule(&simdRule);
}

// Helper functions
static const char* ruleTypeToString(HyperionRuleType type) {
    switch (type) {
        case HYPERION_RULE_MEMORY_OPTIMIZATION: return "Memory Optimization";
        case HYPERION_RULE_PLATFORM_COMPATIBILITY: return "Platform Compatibility";
        case HYPERION_RULE_QUEST_VERIFICATION: return "Quest Verification";
        case HYPERION_RULE_CMAKE_INTEGRATION: return "CMake Integration";
        case HYPERION_RULE_DOCS_SYNC: return "Documentation Sync";
        case HYPERION_RULE_MCP_INTEGRATION: return "MCP Integration";
        case HYPERION_RULE_SIMD_OPTIMIZATION: return "SIMD Optimization";
        case HYPERION_RULE_STREAMING_COMPATIBILITY: return "Streaming Compatibility";
        default: return "Unknown";
    }
}

static const char* ruleResultToString(HyperionRuleResult result) {
    switch (result) {
        case HYPERION_RULE_RESULT_PASS: return "PASS";
        case HYPERION_RULE_RESULT_FAIL: return "FAIL";
        case HYPERION_RULE_RESULT_WARNING: return "WARNING";
        case HYPERION_RULE_RESULT_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}