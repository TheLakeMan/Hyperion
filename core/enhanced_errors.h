/**
 * Hyperion Enhanced Error System
 * 
 * Provides detailed error codes, context-aware error reporting,
 * and recovery mechanisms for improved debugging and user experience.
 */

#ifndef HYPERION_ENHANCED_ERRORS_H
#define HYPERION_ENHANCED_ERRORS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error code categories */
#define HYPERION_ERROR_CATEGORY_SHIFT   24
#define HYPERION_ERROR_CATEGORY_MASK    (0xFFu << HYPERION_ERROR_CATEGORY_SHIFT)
#define HYPERION_ERROR_CODE_MASK        0x00FFFFFFu
#define HYPERION_ERROR_MAKE(category, value) \
    (((uint32_t)(category) << HYPERION_ERROR_CATEGORY_SHIFT) | \
     ((uint32_t)(value) & HYPERION_ERROR_CODE_MASK))

/* Error categories */
typedef enum {
    HYPERION_ERROR_CATEGORY_SYSTEM = 0,        /* System-level errors */
    HYPERION_ERROR_CATEGORY_MEMORY,            /* Memory allocation/management */
    HYPERION_ERROR_CATEGORY_IO,                /* File/network I/O errors */
    HYPERION_ERROR_CATEGORY_MODEL,             /* Model loading/inference errors */
    HYPERION_ERROR_CATEGORY_CONFIG,            /* Configuration errors */
    HYPERION_ERROR_CATEGORY_NETWORK,           /* Network/MCP errors */
    HYPERION_ERROR_CATEGORY_VALIDATION,        /* Input validation errors */
    HYPERION_ERROR_CATEGORY_RUNTIME,           /* Runtime execution errors */
    HYPERION_ERROR_CATEGORY_SIMD,              /* SIMD acceleration issues */
    HYPERION_ERROR_CATEGORY_USER,              /* User input/interaction errors */
    HYPERION_ERROR_CATEGORY_COUNT
} HyperionErrorCategory;

/**
 * Error severity indicates urgency and recommended response.
 */
typedef enum {
    HYPERION_ERROR_SEVERITY_INFO = 0,
    HYPERION_ERROR_SEVERITY_WARNING,
    HYPERION_ERROR_SEVERITY_ERROR,
    HYPERION_ERROR_SEVERITY_CRITICAL,
    HYPERION_ERROR_SEVERITY_FATAL
} HyperionErrorSeverity;

/* Detailed error codes */
typedef enum {
    HYPERION_SUCCESS = 0,

    /* System errors */
    HYPERION_ERROR_SYSTEM_INIT_FAILED            = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0001),
    HYPERION_ERROR_SYSTEM_UNSUPPORTED_OS         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0002),
    HYPERION_ERROR_SYSTEM_PERMISSION_DENIED      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0003),
    HYPERION_ERROR_SYSTEM_RESOURCE_BUSY          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0004),
    HYPERION_ERROR_SYSTEM_SIMD_UNAVAILABLE       = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0005),
    HYPERION_ERROR_SYSTEM_PLATFORM_UNSUPPORTED   = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0006),
    HYPERION_ERROR_SYSTEM_PERMISSIONS_INSUFFICIENT = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,    0x0007),
    HYPERION_ERROR_SYSTEM_RESOURCE_EXHAUSTED     = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SYSTEM,      0x0008),

    /* Memory errors */
    HYPERION_ERROR_MEMORY_ALLOCATION_FAILED      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0001),
    HYPERION_ERROR_MEMORY_OUT_OF_BOUNDS          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0002),
    HYPERION_ERROR_MEMORY_POOL_EXHAUSTED         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0003),
    HYPERION_ERROR_MEMORY_ALIGNMENT_ERROR        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0004),
    HYPERION_ERROR_MEMORY_CORRUPTION             = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0005),
    HYPERION_ERROR_MEMORY_LEAK_DETECTED          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0006),
    HYPERION_ERROR_MEMORY_LIMIT_EXCEEDED         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MEMORY,      0x0007),

    /* I/O errors */
    HYPERION_ERROR_IO_FILE_NOT_FOUND             = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0001),
    HYPERION_ERROR_IO_FILE_ACCESS_DENIED         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0002),
    HYPERION_ERROR_IO_FILE_CORRUPTED             = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0003),
    HYPERION_ERROR_IO_WRITE_FAILED               = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0004),
    HYPERION_ERROR_IO_READ_FAILED                = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0005),
    HYPERION_ERROR_IO_DISK_FULL                  = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0006),
    HYPERION_ERROR_IO_INVALID_PATH               = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_IO,          0x0007),
    HYPERION_ERROR_IO_ACCESS_DENIED              = HYPERION_ERROR_IO_FILE_ACCESS_DENIED,

    /* Model errors */
    HYPERION_ERROR_MODEL_LOAD_FAILED             = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0001),
    HYPERION_ERROR_MODEL_INVALID_FORMAT          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0002),
    HYPERION_ERROR_MODEL_UNSUPPORTED             = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0003),
    HYPERION_ERROR_MODEL_QUANTIZATION_ERROR      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0004),
    HYPERION_ERROR_MODEL_INFERENCE_FAILED        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0005),
    HYPERION_ERROR_MODEL_CONTEXT_OVERFLOW        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0006),
    HYPERION_ERROR_MODEL_TOKENIZATION_ERROR      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0007),
    HYPERION_ERROR_MODEL_FORMAT_UNSUPPORTED      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0008),
    HYPERION_ERROR_MODEL_VERSION_MISMATCH        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x0009),
    HYPERION_ERROR_MODEL_WEIGHTS_CORRUPTED       = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x000A),
    HYPERION_ERROR_MODEL_TOKENIZER_MISSING       = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_MODEL,       0x000B),

    /* Configuration errors */
    HYPERION_ERROR_CONFIG_INVALID_SYNTAX         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0001),
    HYPERION_ERROR_CONFIG_MISSING_REQUIRED       = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0002),
    HYPERION_ERROR_CONFIG_INVALID_VALUE          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0003),
    HYPERION_ERROR_CONFIG_FILE_NOT_FOUND         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0004),
    HYPERION_ERROR_CONFIG_PARSE_ERROR            = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0005),
    HYPERION_ERROR_CONFIG_VERSION_MISMATCH       = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0006),
    HYPERION_ERROR_CONFIG_FILE_INVALID           = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0007),
    HYPERION_ERROR_CONFIG_PARAMETER_MISSING      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_CONFIG,      0x0008),

    /* Network/MCP errors */
    HYPERION_ERROR_NETWORK_CONNECTION_FAILED     = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_NETWORK,     0x0001),
    HYPERION_ERROR_NETWORK_TIMEOUT               = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_NETWORK,     0x0002),
    HYPERION_ERROR_NETWORK_PROTOCOL_ERROR        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_NETWORK,     0x0003),
    HYPERION_ERROR_MCP_SERVER_UNAVAILABLE        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_NETWORK,     0x0004),
    HYPERION_ERROR_MCP_INVALID_RESPONSE          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_NETWORK,     0x0005),
    HYPERION_ERROR_MCP_AUTHENTICATION_FAILED     = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_NETWORK,     0x0006),

    /* Validation errors */
    HYPERION_ERROR_VALIDATION_INPUT_NULL         = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_VALIDATION,  0x0001),
    HYPERION_ERROR_VALIDATION_RANGE_EXCEEDED     = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_VALIDATION,  0x0002),

    /* Runtime errors */
    HYPERION_ERROR_RUNTIME_ASSERTION_FAILED      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_RUNTIME,     0x0001),
    HYPERION_ERROR_RUNTIME_STACK_OVERFLOW        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_RUNTIME,     0x0002),
    HYPERION_ERROR_RUNTIME_DIVIDE_BY_ZERO        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_RUNTIME,     0x0003),
    HYPERION_ERROR_RUNTIME_THREAD_ERROR          = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_RUNTIME,     0x0004),
    HYPERION_ERROR_RUNTIME_LOCK_ERROR            = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_RUNTIME,     0x0005),
    HYPERION_ERROR_RUNTIME_INITIALIZATION_FAILED = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_RUNTIME,     0x0006),

    /* SIMD errors */
    HYPERION_ERROR_SIMD_UNSUPPORTED              = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_SIMD,        0x0001),

    /* User errors */
    HYPERION_ERROR_USER_INVALID_INPUT            = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_USER,        0x0001),
    HYPERION_ERROR_USER_OPERATION_CANCELLED      = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_USER,        0x0002),
    HYPERION_ERROR_USER_VALIDATION_FAILED        = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_USER,        0x0003),
    HYPERION_ERROR_USER_QUOTA_EXCEEDED           = HYPERION_ERROR_MAKE(HYPERION_ERROR_CATEGORY_USER,        0x0004)
} HyperionErrorCode;

typedef struct {
    HyperionErrorCode      code;
    HyperionErrorCategory  category;
    HyperionErrorSeverity  severity;
    const char*            message;
    const char*            suggestion;
    const char*            documentation_link;
    const char*            function;
    const char*            file;
    int                    line;
    const char*            context[2];
    int64_t                error_data[2];
    uint64_t               timestamp;
    uint32_t               thread_id;
} HyperionErrorInfo;

/* Core error API */
int hyperion_enhanced_errors_init(void);
void hyperion_enhanced_errors_cleanup(void);

void hyperion_set_enhanced_error(HyperionErrorCode code,
                                 const char* message,
                                 const char* suggestion,
                                 const char* function,
                                 const char* file,
                                 int line);

void hyperion_set_enhanced_error_with_context(HyperionErrorCode code,
                                              const char* message,
                                              const char* suggestion,
                                              const char* function,
                                              const char* file,
                                              int line,
                                              const char* context1,
                                              const char* context2);

void hyperion_set_enhanced_error_with_data(HyperionErrorCode code,
                                           const char* message,
                                           const char* suggestion,
                                           const char* function,
                                           const char* file,
                                           int line,
                                           int64_t data1,
                                           int64_t data2);

const HyperionErrorInfo* hyperion_get_last_error(void);
void hyperion_clear_error(void);

const char* hyperion_get_error_category_name(HyperionErrorCategory category);
const char* hyperion_get_error_severity_name(HyperionErrorSeverity severity);
const char* hyperion_get_error_description(HyperionErrorCode code);
const char** hyperion_get_error_recovery_suggestions(HyperionErrorCode code);
int hyperion_is_error_recoverable(HyperionErrorCode code);

int hyperion_get_error_statistics(int* category_counts, int* severity_counts);
void hyperion_print_error_report(int include_context);
int hyperion_format_error_for_logging(char* buffer, size_t buffer_size,
                                      int include_suggestions);

/* Convenience macros */
#define HYPERION_SET_ERROR(code, message, suggestion) \
    hyperion_set_enhanced_error((code), (message), (suggestion), __func__, __FILE__, __LINE__)

#define HYPERION_SET_ERROR_WITH_CONTEXT(code, message, suggestion, ctx1, ctx2) \
    hyperion_set_enhanced_error_with_context((code), (message), (suggestion), \
                                             __func__, __FILE__, __LINE__, (ctx1), (ctx2))

#define HYPERION_SET_ERROR_WITH_DATA(code, message, suggestion, data1, data2) \
    hyperion_set_enhanced_error_with_data((code), (message), (suggestion), \
                                          __func__, __FILE__, __LINE__, (data1), (data2))

#define HYPERION_RETURN_IF_ERROR(expr) \
    do { \
        HyperionErrorCode _result = (expr); \
        if (_result != HYPERION_SUCCESS) { \
            return _result; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_ENHANCED_ERRORS_H */