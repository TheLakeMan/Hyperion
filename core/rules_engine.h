#ifndef HYPERION_RULES_ENGINE_H
#define HYPERION_RULES_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Rule types and categories
typedef enum {
    HYPERION_RULE_MEMORY_OPTIMIZATION,
    HYPERION_RULE_PLATFORM_COMPATIBILITY,
    HYPERION_RULE_QUEST_VERIFICATION,
    HYPERION_RULE_CMAKE_INTEGRATION,
    HYPERION_RULE_DOCS_SYNC,
    HYPERION_RULE_MCP_INTEGRATION,
    HYPERION_RULE_SIMD_OPTIMIZATION,
    HYPERION_RULE_STREAMING_COMPATIBILITY,
    HYPERION_RULE_CROSS_PLATFORM,
    HYPERION_RULE_MAX
} HyperionRuleType;

typedef enum {
    HYPERION_TRIGGER_CODE_MODIFICATION,
    HYPERION_TRIGGER_NEW_FEATURE,
    HYPERION_TRIGGER_MODEL_OPERATION,
    HYPERION_TRIGGER_CORE_MODIFICATION,
    HYPERION_TRIGGER_SIMD_CODE,
    HYPERION_TRIGGER_BUILD_CHANGE,
    HYPERION_TRIGGER_PHASE_COMPLETE,
    HYPERION_TRIGGER_QUEST_COMPLETE,
    HYPERION_TRIGGER_MAX
} HyperionRuleTrigger;

typedef enum {
    HYPERION_RULE_RESULT_PASS,
    HYPERION_RULE_RESULT_FAIL,
    HYPERION_RULE_RESULT_WARNING,
    HYPERION_RULE_RESULT_ERROR
} HyperionRuleResult;

// Rule validation context
typedef struct {
    const char* filePath;
    const char* functionName;
    size_t memoryUsage;
    size_t memoryDelta;
    int platform;
    bool isEmbedded;
    bool usesSIMD;
    const char* description;
} HyperionRuleContext;

// Rule validation function pointer
typedef HyperionRuleResult (*HyperionRuleValidator)(const HyperionRuleContext* context, 
                                                   char* errorMessage, 
                                                   size_t errorMessageSize);

// Rule definition structure
typedef struct {
    HyperionRuleType type;
    const char* name;
    const char* description;
    HyperionRuleTrigger triggers[8];
    int numTriggers;
    HyperionRuleValidator validator;
    bool enabled;
    int priority;
} HyperionRule;

// Rules engine functions
bool hyperionRulesEngineInit(void);
void hyperionRulesEngineCleanup(void);

// Rule management
bool hyperionRulesEngineAddRule(const HyperionRule* rule);
bool hyperionRulesEngineRemoveRule(HyperionRuleType type);
bool hyperionRulesEngineEnableRule(HyperionRuleType type, bool enabled);

// Rule validation
HyperionRuleResult hyperionRulesEngineValidate(HyperionRuleTrigger trigger,
                                               const HyperionRuleContext* context);
HyperionRuleResult hyperionRulesEngineValidateAll(const HyperionRuleContext* context);

// Memory optimization rule validators
HyperionRuleResult hyperionValidateMemoryOptimization(const HyperionRuleContext* context,
                                                      char* errorMessage,
                                                      size_t errorMessageSize);

// Platform compatibility rule validators  
HyperionRuleResult hyperionValidatePlatformCompatibility(const HyperionRuleContext* context,
                                                        char* errorMessage,
                                                        size_t errorMessageSize);

// Quest verification rule validators
HyperionRuleResult hyperionValidateQuestCompletion(const HyperionRuleContext* context,
                                                   char* errorMessage,
                                                   size_t errorMessageSize);
HyperionRuleResult hyperionValidateQuestCompletionEnhanced(const HyperionRuleContext* context,
                                                         char* errorMessage,
                                                         size_t errorMessageSize);

// SIMD optimization rule validators
HyperionRuleResult hyperionValidateSIMDOptimization(const HyperionRuleContext* context,
                                                   char* errorMessage,
                                                   size_t errorMessageSize);
HyperionRuleResult hyperionValidateSIMDOptimizationEnhanced(const HyperionRuleContext* context,
                                                          char* errorMessage,
                                                          size_t errorMessageSize);

// Cross-platform validation rule validators
HyperionRuleResult hyperionValidateCrossPlatformCompatibilityRule(const HyperionRuleContext* context,
                                                                char* errorMessage,
                                                                size_t errorMessageSize);

// Utility functions for rule checking
bool hyperionRulesEngineCheckMemoryTarget(size_t memoryUsage, bool isEmbedded);
bool hyperionRulesEngineCheckPlatformSupport(const char* filePath);
bool hyperionRulesEngineCheckFileExists(const char* filePath);
bool hyperionRulesEngineCheckCompilation(const char* filePath);
bool hyperionRulesEngineCheckSIMDCompatibility(const char* code);

// Statistics and reporting
typedef struct {
    int totalRules;
    int enabledRules;
    int passedValidations;
    int failedValidations;
    int warnings;
    int errors;
} HyperionRulesEngineStats;

void hyperionRulesEngineGetStats(HyperionRulesEngineStats* stats);
void hyperionRulesEngineResetStats(void);
void hyperionRulesEnginePrintReport(void);

#ifdef __cplusplus
}
#endif

#endif // HYPERION_RULES_ENGINE_H