#include "rules_engine.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Platform compatibility constants
#define MAX_PLATFORM_ISSUES 16
#define MAX_ISSUE_MESSAGE_LEN 256

// Platform types
typedef enum {
    HYPERION_PLATFORM_WINDOWS,
    HYPERION_PLATFORM_LINUX,
    HYPERION_PLATFORM_MACOS,
    HYPERION_PLATFORM_WASM,
    HYPERION_PLATFORM_ANDROID,
    HYPERION_PLATFORM_IOS,
    HYPERION_PLATFORM_EMBEDDED,
    HYPERION_PLATFORM_MAX
} HyperionPlatformType;

// Compatibility issue types
typedef enum {
    HYPERION_ISSUE_PLATFORM_SPECIFIC_HEADER,
    HYPERION_ISSUE_PLATFORM_SPECIFIC_API,
    HYPERION_ISSUE_INCOMPATIBLE_SIMD,
    HYPERION_ISSUE_MISSING_ABSTRACTION,
    HYPERION_ISSUE_ENDIANNESS,
    HYPERION_ISSUE_COMPILER_SPECIFIC,
    HYPERION_ISSUE_THREADING,
    HYPERION_ISSUE_FILESYSTEM,
    HYPERION_ISSUE_MAX
} HyperionCompatibilityIssueType;

// Compatibility issue structure
typedef struct {
    HyperionCompatibilityIssueType type;
    char message[MAX_ISSUE_MESSAGE_LEN];
    HyperionRuleResult severity;
    const char* suggestion;
} HyperionCompatibilityIssue;

// Platform compatibility analysis
typedef struct {
    HyperionPlatformType targetPlatforms[HYPERION_PLATFORM_MAX];
    int numTargetPlatforms;
    HyperionCompatibilityIssue issues[MAX_PLATFORM_ISSUES];
    int numIssues;
    bool hasAbstractionLayer;
    bool usesSIMD;
    bool isPortable;
} HyperionPlatformAnalysis;

// Forward declarations
static HyperionRuleResult analyzePlatformCompatibility(const HyperionRuleContext* context,
                                                     HyperionPlatformAnalysis* analysis,
                                                     char* errorMessage,
                                                     size_t errorMessageSize);
static void checkPlatformSpecificHeaders(const char* filePath, HyperionPlatformAnalysis* analysis);
static void checkPlatformSpecificAPIs(const char* functionName, HyperionPlatformAnalysis* analysis);
static void checkSIMDCompatibility(const HyperionRuleContext* context, HyperionPlatformAnalysis* analysis);
static void checkCompilerSpecificCode(const HyperionRuleContext* context, HyperionPlatformAnalysis* analysis);
static void checkThreadingCompatibility(const HyperionRuleContext* context, HyperionPlatformAnalysis* analysis);
static void addCompatibilityIssue(HyperionPlatformAnalysis* analysis,
                                 HyperionCompatibilityIssueType type,
                                 const char* message,
                                 HyperionRuleResult severity,
                                 const char* suggestion);
static const char* platformTypeToString(HyperionPlatformType platform);

HyperionRuleResult hyperionValidatePlatformCompatibilityEnhanced(const HyperionRuleContext* context,
                                                               char* errorMessage,
                                                               size_t errorMessageSize) {
    if (!context) {
        snprintf(errorMessage, errorMessageSize, "Invalid context");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    HyperionPlatformAnalysis analysis = {0};
    
    // Perform comprehensive platform compatibility analysis
    HyperionRuleResult analysisResult = analyzePlatformCompatibility(context, &analysis, 
                                                                   errorMessage, errorMessageSize);
    if (analysisResult != HYPERION_RULE_RESULT_PASS) {
        return analysisResult;
    }
    
    // Determine worst compatibility issue
    HyperionRuleResult worstResult = HYPERION_RULE_RESULT_PASS;
    char detailedMessage[512] = {0};
    
    for (int i = 0; i < analysis.numIssues; i++) {
        if (analysis.issues[i].severity > worstResult) {
            worstResult = analysis.issues[i].severity;
            snprintf(detailedMessage, sizeof(detailedMessage), "%s. %s", 
                    analysis.issues[i].message,
                    analysis.issues[i].suggestion ? analysis.issues[i].suggestion : "");
        }
    }
    
    if (worstResult > HYPERION_RULE_RESULT_PASS) {
        snprintf(errorMessage, errorMessageSize, "%s", detailedMessage);
    }
    
    // Additional checks for Hyperion-specific requirements
    if (!analysis.isPortable && analysis.numTargetPlatforms > 1) {
        snprintf(errorMessage, errorMessageSize, 
                "Code targets multiple platforms but lacks portability features");
        if (worstResult < HYPERION_RULE_RESULT_WARNING) {
            worstResult = HYPERION_RULE_RESULT_WARNING;
        }
    }
    
    return worstResult;
}

static HyperionRuleResult analyzePlatformCompatibility(const HyperionRuleContext* context,
                                                     HyperionPlatformAnalysis* analysis,
                                                     char* errorMessage,
                                                     size_t errorMessageSize) {
    if (!analysis) {
        snprintf(errorMessage, errorMessageSize, "Invalid analysis structure");
        return HYPERION_RULE_RESULT_ERROR;
    }
    
    // Initialize analysis
    memset(analysis, 0, sizeof(*analysis));
    
    // Determine target platforms based on file path and context
    if (context->filePath) {
        if (strstr(context->filePath, "wasm") || strstr(context->filePath, "web")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_WASM;
        }
        if (strstr(context->filePath, "android")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_ANDROID;
        }
        if (strstr(context->filePath, "ios")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_IOS;
        }
        if (strstr(context->filePath, "embedded")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_EMBEDDED;
        }
        if (strstr(context->filePath, "windows")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_WINDOWS;
        }
        if (strstr(context->filePath, "linux")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_LINUX;
        }
        if (strstr(context->filePath, "macos") || strstr(context->filePath, "darwin")) {
            analysis->targetPlatforms[analysis->numTargetPlatforms++] = HYPERION_PLATFORM_MACOS;
        }
    }
    
    // If no specific platform detected, assume cross-platform
    if (analysis->numTargetPlatforms == 0) {
        analysis->targetPlatforms[0] = HYPERION_PLATFORM_WINDOWS;
        analysis->targetPlatforms[1] = HYPERION_PLATFORM_LINUX;
        analysis->targetPlatforms[2] = HYPERION_PLATFORM_MACOS;
        analysis->numTargetPlatforms = 3;
    }
    
    // Check for abstraction layer usage
    analysis->hasAbstractionLayer = 
        (context->filePath && (strstr(context->filePath, "core/") || 
                              strstr(context->filePath, "platform/"))) ||
        (context->functionName && strstr(context->functionName, "hyperion"));
    
    // Check if uses SIMD
    analysis->usesSIMD = context->usesSIMD;
    
    // Perform specific compatibility checks
    if (context->filePath) {
        checkPlatformSpecificHeaders(context->filePath, analysis);
    }
    
    if (context->functionName) {
        checkPlatformSpecificAPIs(context->functionName, analysis);
    }
    
    checkSIMDCompatibility(context, analysis);
    checkCompilerSpecificCode(context, analysis);
    checkThreadingCompatibility(context, analysis);
    
    // Determine overall portability
    analysis->isPortable = (analysis->numIssues == 0) || 
                          (analysis->hasAbstractionLayer && analysis->numIssues <= 2);
    
    return HYPERION_RULE_RESULT_PASS;
}

static void checkPlatformSpecificHeaders(const char* filePath, HyperionPlatformAnalysis* analysis) {
    if (!filePath || !analysis) {
        return;
    }
    
    // Platform-specific headers that indicate potential compatibility issues
    const struct {
        const char* header;
        const char* platform;
        HyperionRuleResult severity;
        const char* suggestion;
    } platformHeaders[] = {
        {"windows.h", "Windows", HYPERION_RULE_RESULT_WARNING, "Use core/io.h abstraction"},
        {"unistd.h", "Unix/Linux", HYPERION_RULE_RESULT_WARNING, "Use core/io.h abstraction"},
        {"sys/mman.h", "Unix/Linux", HYPERION_RULE_RESULT_WARNING, "Use memory mapping abstraction"},
        {"direct.h", "Windows", HYPERION_RULE_RESULT_WARNING, "Use filesystem abstraction"},
        {"pthread.h", "POSIX", HYPERION_RULE_RESULT_WARNING, "Use threading abstraction"},
        {"emmintrin.h", "SSE2", HYPERION_RULE_RESULT_WARNING, "Ensure SIMD fallbacks exist"},
        {"immintrin.h", "AVX/AVX2", HYPERION_RULE_RESULT_WARNING, "Ensure SIMD fallbacks exist"},
        {"arm_neon.h", "ARM NEON", HYPERION_RULE_RESULT_WARNING, "Ensure SIMD fallbacks exist"}
    };
    
    // Check if file content would contain these headers (simplified check)
    for (size_t i = 0; i < sizeof(platformHeaders) / sizeof(platformHeaders[0]); i++) {
        if (strstr(filePath, platformHeaders[i].header) || 
            (analysis->numIssues == 0 && strstr(filePath, "simd") && 
             (strcmp(platformHeaders[i].header, "emmintrin.h") == 0 ||
              strcmp(platformHeaders[i].header, "immintrin.h") == 0))) {
            
            char message[MAX_ISSUE_MESSAGE_LEN];
            snprintf(message, sizeof(message), 
                    "Uses %s-specific header: %s", 
                    platformHeaders[i].platform, 
                    platformHeaders[i].header);
            
            addCompatibilityIssue(analysis, HYPERION_ISSUE_PLATFORM_SPECIFIC_HEADER,
                                message, platformHeaders[i].severity,
                                platformHeaders[i].suggestion);
        }
    }
}

static void checkPlatformSpecificAPIs(const char* functionName, HyperionPlatformAnalysis* analysis) {
    if (!functionName || !analysis) {
        return;
    }
    
    // Platform-specific API patterns
    const struct {
        const char* apiPattern;
        const char* platform;
        HyperionRuleResult severity;
        const char* suggestion;
    } platformAPIs[] = {
        {"CreateFile", "Windows", HYPERION_RULE_RESULT_FAIL, "Use hyperionIOOpen()"},
        {"ReadFile", "Windows", HYPERION_RULE_RESULT_FAIL, "Use hyperionIORead()"},
        {"WriteFile", "Windows", HYPERION_RULE_RESULT_FAIL, "Use hyperionIOWrite()"},
        {"VirtualAlloc", "Windows", HYPERION_RULE_RESULT_FAIL, "Use hyperionMemAlloc()"},
        {"malloc", "C stdlib", HYPERION_RULE_RESULT_WARNING, "Use hyperionMemAlloc() with pools"},
        {"free", "C stdlib", HYPERION_RULE_RESULT_WARNING, "Use hyperionMemFree() with pools"},
        {"open", "POSIX", HYPERION_RULE_RESULT_FAIL, "Use hyperionIOOpen()"},
        {"read", "POSIX", HYPERION_RULE_RESULT_FAIL, "Use hyperionIORead()"},
        {"write", "POSIX", HYPERION_RULE_RESULT_FAIL, "Use hyperionIOWrite()"},
        {"mmap", "POSIX", HYPERION_RULE_RESULT_WARNING, "Use hyperionMemMapFile()"},
        {"pthread_create", "POSIX", HYPERION_RULE_RESULT_WARNING, "Use threading abstraction"}
    };
    
    for (size_t i = 0; i < sizeof(platformAPIs) / sizeof(platformAPIs[0]); i++) {
        if (strstr(functionName, platformAPIs[i].apiPattern)) {
            char message[MAX_ISSUE_MESSAGE_LEN];
            snprintf(message, sizeof(message), 
                    "Uses %s-specific API: %s", 
                    platformAPIs[i].platform, 
                    platformAPIs[i].apiPattern);
            
            addCompatibilityIssue(analysis, HYPERION_ISSUE_PLATFORM_SPECIFIC_API,
                                message, platformAPIs[i].severity,
                                platformAPIs[i].suggestion);
        }
    }
}

static void checkSIMDCompatibility(const HyperionRuleContext* context, HyperionPlatformAnalysis* analysis) {
    if (!context || !analysis) {
        return;
    }
    
    if (!context->usesSIMD) {
        return;
    }
    
    // Check for SIMD instruction compatibility across target platforms
    const struct {
        const char* instruction;
        bool availableOnWASM;
        bool availableOnMobile;
        bool availableOnEmbedded;
        const char* fallback;
    } simdInstructions[] = {
        {"_mm256_", false, false, false, "Use _mm_ (SSE) or scalar fallback"},
        {"_mm512_", false, false, false, "Use _mm256_ (AVX2) or scalar fallback"},
        {"__builtin_", true, true, false, "Provide embedded-specific implementation"},
        {"vld1q_", false, true, false, "ARM NEON - provide x86 equivalent"}
    };
    
    for (int i = 0; i < analysis->numTargetPlatforms; i++) {
        HyperionPlatformType platform = analysis->targetPlatforms[i];
        
        for (size_t j = 0; j < sizeof(simdInstructions) / sizeof(simdInstructions[0]); j++) {
            bool incompatible = false;
            
            switch (platform) {
                case HYPERION_PLATFORM_WASM:
                    incompatible = !simdInstructions[j].availableOnWASM;
                    break;
                case HYPERION_PLATFORM_ANDROID:
                case HYPERION_PLATFORM_IOS:
                    incompatible = !simdInstructions[j].availableOnMobile;
                    break;
                case HYPERION_PLATFORM_EMBEDDED:
                    incompatible = !simdInstructions[j].availableOnEmbedded;
                    break;
                default:
                    incompatible = false; // Desktop platforms generally support SIMD
                    break;
            }
            
            if (incompatible && (context->functionName && 
                strstr(context->functionName, simdInstructions[j].instruction))) {
                char message[MAX_ISSUE_MESSAGE_LEN];
                snprintf(message, sizeof(message), 
                        "SIMD instruction %s incompatible with %s", 
                        simdInstructions[j].instruction,
                        platformTypeToString(platform));
                
                addCompatibilityIssue(analysis, HYPERION_ISSUE_INCOMPATIBLE_SIMD,
                                    message, HYPERION_RULE_RESULT_FAIL,
                                    simdInstructions[j].fallback);
            }
        }
    }
}

static void checkCompilerSpecificCode(const HyperionRuleContext* context, HyperionPlatformAnalysis* analysis) {
    if (!context || !analysis) {
        return;
    }
    
    // Check for compiler-specific extensions
    const struct {
        const char* pattern;
        const char* compiler;
        const char* suggestion;
    } compilerSpecific[] = {
        {"__declspec", "MSVC", "Use cross-platform attributes or macros"},
        {"__attribute__", "GCC/Clang", "Use cross-platform attributes or macros"},
        {"__forceinline", "MSVC", "Use HYPERION_INLINE macro"},
        {"__always_inline__", "GCC", "Use HYPERION_INLINE macro"},
        {"#pragma pack", "Compiler-specific", "Use portable structure packing"},
        {"__builtin_expect", "GCC", "Use HYPERION_LIKELY/UNLIKELY macros"}
    };
    
    for (size_t i = 0; i < sizeof(compilerSpecific) / sizeof(compilerSpecific[0]); i++) {
        if ((context->functionName && strstr(context->functionName, compilerSpecific[i].pattern)) ||
            (context->description && strstr(context->description, compilerSpecific[i].pattern))) {
            
            char message[MAX_ISSUE_MESSAGE_LEN];
            snprintf(message, sizeof(message), 
                    "Uses %s-specific extension: %s", 
                    compilerSpecific[i].compiler,
                    compilerSpecific[i].pattern);
            
            addCompatibilityIssue(analysis, HYPERION_ISSUE_COMPILER_SPECIFIC,
                                message, HYPERION_RULE_RESULT_WARNING,
                                compilerSpecific[i].suggestion);
        }
    }
}

static void checkThreadingCompatibility(const HyperionRuleContext* context, HyperionPlatformAnalysis* analysis) {
    if (!context || !analysis) {
        return;
    }
    
    // Check for threading-related compatibility issues
    const char* threadingPatterns[] = {
        "thread", "mutex", "lock", "atomic", "barrier", "condition"
    };
    
    for (size_t i = 0; i < sizeof(threadingPatterns) / sizeof(threadingPatterns[0]); i++) {
        if ((context->functionName && strstr(context->functionName, threadingPatterns[i])) ||
            (context->description && strstr(context->description, threadingPatterns[i]))) {
            
            // Check if targeting platforms with limited threading support
            for (int j = 0; j < analysis->numTargetPlatforms; j++) {
                if (analysis->targetPlatforms[j] == HYPERION_PLATFORM_WASM ||
                    analysis->targetPlatforms[j] == HYPERION_PLATFORM_EMBEDDED) {
                    
                    char message[MAX_ISSUE_MESSAGE_LEN];
                    snprintf(message, sizeof(message), 
                            "Threading code may not be supported on %s", 
                            platformTypeToString(analysis->targetPlatforms[j]));
                    
                    addCompatibilityIssue(analysis, HYPERION_ISSUE_THREADING,
                                        message, HYPERION_RULE_RESULT_WARNING,
                                        "Provide single-threaded fallback");
                    break;
                }
            }
            break;
        }
    }
}

static void addCompatibilityIssue(HyperionPlatformAnalysis* analysis,
                                 HyperionCompatibilityIssueType type,
                                 const char* message,
                                 HyperionRuleResult severity,
                                 const char* suggestion) {
    if (!analysis || analysis->numIssues >= MAX_PLATFORM_ISSUES) {
        return;
    }
    
    HyperionCompatibilityIssue* issue = &analysis->issues[analysis->numIssues];
    issue->type = type;
    issue->severity = severity;
    issue->suggestion = suggestion;
    
    strncpy(issue->message, message, MAX_ISSUE_MESSAGE_LEN - 1);
    issue->message[MAX_ISSUE_MESSAGE_LEN - 1] = '\0';
    
    analysis->numIssues++;
}

static const char* platformTypeToString(HyperionPlatformType platform) {
    switch (platform) {
        case HYPERION_PLATFORM_WINDOWS: return "Windows";
        case HYPERION_PLATFORM_LINUX: return "Linux";
        case HYPERION_PLATFORM_MACOS: return "macOS";
        case HYPERION_PLATFORM_WASM: return "WebAssembly";
        case HYPERION_PLATFORM_ANDROID: return "Android";
        case HYPERION_PLATFORM_IOS: return "iOS";
        case HYPERION_PLATFORM_EMBEDDED: return "Embedded";
        default: return "Unknown";
    }
}

// Additional utility functions for platform compatibility
bool hyperionPlatformCompatibilityCheckSIMDSupport(HyperionPlatformType platform, const char* instruction) {
    if (!instruction) {
        return false;
    }
    
    // Simplified SIMD support matrix
    if (strstr(instruction, "_mm256_") || strstr(instruction, "avx")) {
        return platform == HYPERION_PLATFORM_WINDOWS || 
               platform == HYPERION_PLATFORM_LINUX || 
               platform == HYPERION_PLATFORM_MACOS;
    }
    
    if (strstr(instruction, "_mm_") || strstr(instruction, "sse")) {
        return platform != HYPERION_PLATFORM_EMBEDDED;
    }
    
    if (strstr(instruction, "vld1q_") || strstr(instruction, "neon")) {
        return platform == HYPERION_PLATFORM_ANDROID || platform == HYPERION_PLATFORM_IOS;
    }
    
    return true; // Generic instructions assumed supported
}

bool hyperionPlatformCompatibilityRequiresAbstraction(const char* functionName) {
    if (!functionName) {
        return false;
    }
    
    const char* requiresAbstraction[] = {
        "malloc", "free", "open", "read", "write", "close",
        "CreateFile", "ReadFile", "WriteFile", "VirtualAlloc",
        "pthread_", "thread", "mutex"
    };
    
    for (size_t i = 0; i < sizeof(requiresAbstraction) / sizeof(requiresAbstraction[0]); i++) {
        if (strstr(functionName, requiresAbstraction[i])) {
            return true;
        }
    }
    
    return false;
}