/**
 * Hyperion Enhanced Error System Implementation
 */

#include "enhanced_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

/* Error system state */
static HyperionErrorInfo g_last_error;
static int g_error_system_initialized = 0;
static int g_error_statistics[10] = {0}; /* Category counts */
static int g_severity_statistics[5] = {0}; /* Severity counts */

/* Error category names */
static const char* ERROR_CATEGORY_NAMES[] = {
    "System",
    "Memory",
    "I/O",
    "Model",
    "Configuration",
    "Network",
    "Validation",
    "Runtime",
    "SIMD",
    "User"
};

/* Error severity names */
static const char* ERROR_SEVERITY_NAMES[] = {
    "Info",
    "Warning", 
    "Error",
    "Critical",
    "Fatal"
};

/* Detailed error descriptions and solutions */
typedef struct {
    HyperionErrorCode code;
    HyperionErrorCategory category;
    HyperionErrorSeverity severity;
    const char* description;
    const char* solution;
    const char* documentation_link;
    int recoverable;
} ErrorDescriptor;

static const ErrorDescriptor ERROR_DESCRIPTORS[] = {
    /* System Errors */
    {
        HYPERION_ERROR_SYSTEM_PLATFORM_UNSUPPORTED,
        HYPERION_ERROR_CATEGORY_SYSTEM,
        HYPERION_ERROR_SEVERITY_FATAL,
        "Current platform is not supported by Hyperion",
        "Check documentation for supported platforms. Consider using a different system or building from source with platform-specific modifications.",
        "README.md#platform-support",
        0
    },
    {
        HYPERION_ERROR_SYSTEM_PERMISSIONS_INSUFFICIENT,
        HYPERION_ERROR_CATEGORY_SYSTEM,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Insufficient permissions for the requested operation",
        "Run with elevated privileges or ensure the current user has necessary permissions. Check file/directory ownership and permissions.",
        "FAQ.md#permission-issues",
        1
    },
    {
        HYPERION_ERROR_SYSTEM_RESOURCE_EXHAUSTED,
        HYPERION_ERROR_CATEGORY_SYSTEM,
        HYPERION_ERROR_SEVERITY_CRITICAL,
        "System resources exhausted (CPU, memory, or file handles)",
        "Free up system resources, close other applications, or restart the system. Monitor resource usage with system tools.",
        "FAQ.md#resource-management",
        1
    },
    
    /* Memory Errors */
    {
        HYPERION_ERROR_MEMORY_ALLOCATION_FAILED,
        HYPERION_ERROR_CATEGORY_MEMORY,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Memory allocation failed - insufficient available memory",
        "Free up memory by closing other applications, use a smaller model, or enable memory optimization options (--memory-limit, --quantization 4bit).",
        "FAQ.md#memory-issues",
        1
    },
    {
        HYPERION_ERROR_MEMORY_LIMIT_EXCEEDED,
        HYPERION_ERROR_CATEGORY_MEMORY,
        HYPERION_ERROR_SEVERITY_WARNING,
        "Memory usage exceeded configured limit",
        "Increase memory limit with --memory-limit option, use 4-bit quantization, or switch to a smaller model.",
        "ARCHITECTURE.md#memory-management",
        1
    },
    {
        HYPERION_ERROR_MEMORY_LEAK_DETECTED,
        HYPERION_ERROR_CATEGORY_MEMORY,
        HYPERION_ERROR_SEVERITY_WARNING,
        "Memory leak detected - allocated memory not properly freed",
        "Report this as a bug if using stable release. In development, check memory tracking logs and fix allocation/deallocation pairs.",
        "DEVELOPMENT.md#memory-debugging",
        1
    },
    
    /* I/O Errors */
    {
        HYPERION_ERROR_IO_FILE_NOT_FOUND,
        HYPERION_ERROR_CATEGORY_IO,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Required file could not be found",
        "Check file path is correct and file exists. Ensure proper working directory. For models, check model directory path.",
        "FAQ.md#file-not-found",
        1
    },
    {
        HYPERION_ERROR_IO_ACCESS_DENIED,
        HYPERION_ERROR_CATEGORY_IO,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Access denied when trying to read/write file",
        "Check file permissions, ensure file is not in use by another process, run with appropriate privileges.",
        "FAQ.md#access-denied",
        1
    },
    {
        HYPERION_ERROR_IO_DISK_FULL,
        HYPERION_ERROR_CATEGORY_IO,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Disk full - cannot write to storage device",
        "Free up disk space by deleting unnecessary files, or use a different storage location with more available space.",
        "FAQ.md#disk-space",
        1
    },
    
    /* Model Errors */
    {
        HYPERION_ERROR_MODEL_FORMAT_UNSUPPORTED,
        HYPERION_ERROR_CATEGORY_MODEL,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Model file format is not supported",
        "Convert model to supported format (.hbin), or check if model file is corrupted. See model conversion tools in tools/ directory.",
        "EXAMPLES.md#model-conversion",
        1
    },
    {
        HYPERION_ERROR_MODEL_VERSION_MISMATCH,
        HYPERION_ERROR_CATEGORY_MODEL,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Model version is incompatible with current Hyperion version",
        "Update Hyperion to latest version, or convert model to compatible format using conversion tools.",
        "FAQ.md#version-compatibility",
        1
    },
    {
        HYPERION_ERROR_MODEL_WEIGHTS_CORRUPTED,
        HYPERION_ERROR_CATEGORY_MODEL,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Model weights file appears to be corrupted",
        "Re-download or regenerate model weights file. Check file integrity with checksums if available.",
        "FAQ.md#corrupted-models",
        1
    },
    
    /* Configuration Errors */
    {
        HYPERION_ERROR_CONFIG_FILE_INVALID,
        HYPERION_ERROR_CATEGORY_CONFIG,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Configuration file contains invalid syntax or structure",
        "Check configuration file syntax, compare with examples in docs/. Use default configuration as a starting point.",
        "DEVELOPMENT.md#configuration",
        1
    },
    {
        HYPERION_ERROR_CONFIG_PARAMETER_MISSING,
        HYPERION_ERROR_CATEGORY_CONFIG,
        HYPERION_ERROR_SEVERITY_WARNING,
        "Required configuration parameter is missing",
        "Add missing parameter to configuration file or command line. Check documentation for required parameters.",
        "DEVELOPMENT.md#configuration",
        1
    },
    
    /* Network/MCP Errors */
    {
        HYPERION_ERROR_NETWORK_CONNECTION_FAILED,
        HYPERION_ERROR_CATEGORY_NETWORK,
        HYPERION_ERROR_SEVERITY_WARNING,
        "Failed to establish network connection",
        "Check network connectivity, firewall settings, and server availability. Try local execution mode as fallback.",
        "HYBRID_CAPABILITIES.md#troubleshooting",
        1
    },
    {
        HYPERION_ERROR_MCP_SERVER_UNAVAILABLE,
        HYPERION_ERROR_CATEGORY_NETWORK,
        HYPERION_ERROR_SEVERITY_WARNING,
        "MCP server is not available or responding",
        "Check MCP server status, restart server if needed, or switch to local execution mode.",
        "HYBRID_CAPABILITIES.md#mcp-server-setup",
        1
    },
    
    /* Validation Errors */
    {
        HYPERION_ERROR_VALIDATION_INPUT_NULL,
        HYPERION_ERROR_CATEGORY_VALIDATION,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Input parameter is NULL where non-NULL value is required",
        "Provide valid non-NULL input parameter. Check function documentation for parameter requirements.",
        "ARCHITECTURE.md#api-reference",
        1
    },
    {
        HYPERION_ERROR_VALIDATION_RANGE_EXCEEDED,
        HYPERION_ERROR_CATEGORY_VALIDATION,
        HYPERION_ERROR_SEVERITY_ERROR,
        "Input value is outside allowed range",
        "Provide value within valid range. Check documentation for parameter limits and constraints.",
        "ARCHITECTURE.md#api-reference",
        1
    },
    
    /* Runtime Errors */
    {
        HYPERION_ERROR_RUNTIME_INITIALIZATION_FAILED,
        HYPERION_ERROR_CATEGORY_RUNTIME,
        HYPERION_ERROR_SEVERITY_CRITICAL,
        "Hyperion runtime initialization failed",
        "Check system requirements, ensure all dependencies are available, restart application. Check logs for detailed error information.",
        "FAQ.md#initialization-failed",
        1
    },
    
    /* SIMD Errors */
    {
        HYPERION_ERROR_SIMD_UNSUPPORTED,
        HYPERION_ERROR_CATEGORY_SIMD,
        HYPERION_ERROR_SEVERITY_INFO,
        "SIMD instructions not supported on this hardware",
        "SIMD acceleration disabled, using scalar fallback. No action required - performance may be reduced but functionality is preserved.",
        "STATUS.md#performance-characteristics",
        1
    }
};

#define NUM_ERROR_DESCRIPTORS (sizeof(ERROR_DESCRIPTORS) / sizeof(ERROR_DESCRIPTORS[0]))

/* Helper function to get current thread ID */
static uint32_t get_current_thread_id(void) {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return (uint32_t)pthread_self();
#endif
}

/* Find error descriptor for given code */
static const ErrorDescriptor* find_error_descriptor(HyperionErrorCode code) {
    for (size_t i = 0; i < NUM_ERROR_DESCRIPTORS; i++) {
        if (ERROR_DESCRIPTORS[i].code == code) {
            return &ERROR_DESCRIPTORS[i];
        }
    }
    return NULL;
}

/* Implementation functions */

int hyperion_enhanced_errors_init(void) {
    if (g_error_system_initialized) {
        return 0; /* Already initialized */
    }
    
    memset(&g_last_error, 0, sizeof(g_last_error));
    memset(g_error_statistics, 0, sizeof(g_error_statistics));
    memset(g_severity_statistics, 0, sizeof(g_severity_statistics));
    
    g_error_system_initialized = 1;
    return 0;
}

void hyperion_enhanced_errors_cleanup(void) {
    if (!g_error_system_initialized) {
        return;
    }
    
    memset(&g_last_error, 0, sizeof(g_last_error));
    g_error_system_initialized = 0;
}

void hyperion_set_enhanced_error(HyperionErrorCode code, const char* message, 
                                const char* suggestion, const char* function, 
                                const char* file, int line) {
    if (!g_error_system_initialized) {
        hyperion_enhanced_errors_init();
    }
    
    const ErrorDescriptor* desc = find_error_descriptor(code);
    
    memset(&g_last_error, 0, sizeof(g_last_error));
    
    g_last_error.code = code;
    g_last_error.category = desc ? desc->category : HYPERION_ERROR_CATEGORY_SYSTEM;
    g_last_error.severity = desc ? desc->severity : HYPERION_ERROR_SEVERITY_ERROR;
    g_last_error.message = message ? message : (desc ? desc->description : "Unknown error");
    g_last_error.suggestion = suggestion ? suggestion : (desc ? desc->solution : "No solution available");
    g_last_error.documentation_link = desc ? desc->documentation_link : "README.md";
    g_last_error.function = function;
    g_last_error.file = file;
    g_last_error.line = line;
    g_last_error.timestamp = (uint64_t)time(NULL);
    g_last_error.thread_id = get_current_thread_id();
    
    /* Update statistics */
    if (g_last_error.category < 10) {
        g_error_statistics[g_last_error.category]++;
    }
    if (g_last_error.severity < 5) {
        g_severity_statistics[g_last_error.severity]++;
    }
}

void hyperion_set_enhanced_error_with_context(HyperionErrorCode code, const char* message,
                                             const char* suggestion, const char* function,
                                             const char* file, int line,
                                             const char* context1, const char* context2) {
    hyperion_set_enhanced_error(code, message, suggestion, function, file, line);
    
    g_last_error.context[0] = context1;
    g_last_error.context[1] = context2;
}

void hyperion_set_enhanced_error_with_data(HyperionErrorCode code, const char* message,
                                          const char* suggestion, const char* function,
                                          const char* file, int line,
                                          int64_t data1, int64_t data2) {
    hyperion_set_enhanced_error(code, message, suggestion, function, file, line);
    
    g_last_error.error_data[0] = data1;
    g_last_error.error_data[1] = data2;
}

const HyperionErrorInfo* hyperion_get_last_error(void) {
    if (!g_error_system_initialized || g_last_error.code == HYPERION_SUCCESS) {
        return NULL;
    }
    return &g_last_error;
}

const char* hyperion_get_error_category_name(HyperionErrorCategory category) {
    if (category < sizeof(ERROR_CATEGORY_NAMES) / sizeof(ERROR_CATEGORY_NAMES[0])) {
        return ERROR_CATEGORY_NAMES[category];
    }
    return "Unknown";
}

const char* hyperion_get_error_severity_name(HyperionErrorSeverity severity) {
    if (severity < sizeof(ERROR_SEVERITY_NAMES) / sizeof(ERROR_SEVERITY_NAMES[0])) {
        return ERROR_SEVERITY_NAMES[severity];
    }
    return "Unknown";
}

const char* hyperion_get_error_description(HyperionErrorCode code) {
    const ErrorDescriptor* desc = find_error_descriptor(code);
    return desc ? desc->description : "Unknown error code";
}

void hyperion_print_error_report(int include_context) {
    const HyperionErrorInfo* error = hyperion_get_last_error();
    if (!error) {
        fprintf(stderr, "No error information available.\n");
        return;
    }
    
    /* Print header */
    fprintf(stderr, "\n=== Hyperion Error Report ===\n");
    fprintf(stderr, "Error Code: %d\n", error->code);
    fprintf(stderr, "Category: %s\n", hyperion_get_error_category_name(error->category));
    fprintf(stderr, "Severity: %s\n", hyperion_get_error_severity_name(error->severity));
    fprintf(stderr, "Message: %s\n", error->message);
    
    /* Print suggestion */
    if (error->suggestion && strlen(error->suggestion) > 0) {
        fprintf(stderr, "\n💡 Solution:\n%s\n", error->suggestion);
    }
    
    /* Print documentation link */
    if (error->documentation_link && strlen(error->documentation_link) > 0) {
        fprintf(stderr, "\n📚 Documentation: %s\n", error->documentation_link);
    }
    
    /* Print context if requested */
    if (include_context) {
        if (error->function && error->file && error->line > 0) {
            fprintf(stderr, "\n🔍 Location: %s() in %s:%d\n", 
                   error->function, error->file, error->line);
        }
        
        if (error->context[0] || error->context[1]) {
            fprintf(stderr, "\n📝 Context:\n");
            if (error->context[0]) fprintf(stderr, "  - %s\n", error->context[0]);
            if (error->context[1]) fprintf(stderr, "  - %s\n", error->context[1]);
        }
        
        if (error->error_data[0] != 0 || error->error_data[1] != 0) {
            fprintf(stderr, "\n🔢 Data: [%lld, %lld]\n", 
                   (long long)error->error_data[0], (long long)error->error_data[1]);
        }
        
        fprintf(stderr, "\n⏰ Timestamp: %llu\n", (unsigned long long)error->timestamp);
        fprintf(stderr, "🧵 Thread ID: %u\n", error->thread_id);
    }
    
    fprintf(stderr, "=============================\n\n");
}

int hyperion_format_error_for_logging(char* buffer, size_t buffer_size, 
                                     int include_suggestions) {
    const HyperionErrorInfo* error = hyperion_get_last_error();
    if (!error || !buffer || buffer_size == 0) {
        return 0;
    }
    
    int written = snprintf(buffer, buffer_size,
        "[ERROR:%d] [%s:%s] %s",
        error->code,
        hyperion_get_error_category_name(error->category),
        hyperion_get_error_severity_name(error->severity),
        error->message);
    
    if (include_suggestions && error->suggestion && 
        strlen(error->suggestion) > 0 && written < (int)buffer_size - 1) {
        written += snprintf(buffer + written, buffer_size - written,
            " | Solution: %s", error->suggestion);
    }
    
    return written;
}

int hyperion_is_error_recoverable(HyperionErrorCode code) {
    const ErrorDescriptor* desc = find_error_descriptor(code);
    return desc ? desc->recoverable : 0;
}

const char** hyperion_get_error_recovery_suggestions(HyperionErrorCode code) {
    /* This would be implemented with a more complex data structure
       For now, return the basic solution */
    static const char* suggestions[2];
    const ErrorDescriptor* desc = find_error_descriptor(code);
    
    suggestions[0] = desc ? desc->solution : NULL;
    suggestions[1] = NULL;
    
    return suggestions;
}

void hyperion_clear_error(void) {
    memset(&g_last_error, 0, sizeof(g_last_error));
}

int hyperion_get_error_statistics(int* category_counts, int* severity_counts) {
    if (category_counts) {
        memcpy(category_counts, g_error_statistics, sizeof(g_error_statistics));
    }
    if (severity_counts) {
        memcpy(severity_counts, g_severity_statistics, sizeof(g_severity_statistics));
    }
    
    int total = 0;
    for (int i = 0; i < 5; i++) {
        total += g_severity_statistics[i];
    }
    return total;
}