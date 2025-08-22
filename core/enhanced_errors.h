/**
 * Hyperion Enhanced Error System
 * 
 * Provides detailed error codes, context-aware error reporting,
 * and recovery mechanisms for improved debugging and user experience.
 */

#ifndef HYPERION_ENHANCED_ERRORS_H
#define HYPERION_ENHANCED_ERRORS_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error code categories */
#define HYPERION_ERROR_CATEGORY_MASK    0xFF000000
#define HYPERION_ERROR_CATEGORY_SHIFT   24
#define HYPERION_ERROR_CODE_MASK        0x00FFFFFF

/* Error categories */
typedef enum {
    HYPERION_ERROR_CATEGORY_SUCCESS = 0x00,
    HYPERION_ERROR_CATEGORY_SYSTEM  = 0x01,    /* System-level errors */
    HYPERION_ERROR_CATEGORY_MEMORY  = 0x02,    /* Memory allocation/management */
    HYPERION_ERROR_CATEGORY_IO      = 0x03,    /* File/network I/O errors */
    HYPERION_ERROR_CATEGORY_MODEL   = 0x04,    /* Model loading/inference errors */
    HYPERION_ERROR_CATEGORY_CONFIG  = 0x05,    /* Configuration errors */
    HYPERION_ERROR_CATEGORY_NETWORK = 0x06,    /* Network/MCP errors */
    HYPERION_ERROR_CATEGORY_CLI     = 0x07,    /* CLI interface errors */
    HYPERION_ERROR_CATEGORY_RUNTIME = 0x08,    /* Runtime execution errors */
    HYPERION_ERROR_CATEGORY_USER    = 0x09     /* User input/validation errors */
} HyperionErrorCategory;

/* Detailed error codes */
typedef enum {
    /* Success */
    HYPERION_SUCCESS = 0x00000000,
    
    /* System errors (0x01xxxxxx) */
    HYPERION_ERROR_SYSTEM_INIT_FAILED       = 0x01000001,
    HYPERION_ERROR_SYSTEM_UNSUPPORTED_OS    = 0x01000002,
    HYPERION_ERROR_SYSTEM_PERMISSION_DENIED = 0x01000003,
    HYPERION_ERROR_SYSTEM_RESOURCE_BUSY     = 0x01000004,
    HYPERION_ERROR_SYSTEM_SIMD_UNAVAILABLE  = 0x01000005,
    
    /* Memory errors (0x02xxxxxx) */
    HYPERION_ERROR_MEMORY_ALLOCATION_FAILED = 0x02000001,
    HYPERION_ERROR_MEMORY_OUT_OF_BOUNDS     = 0x02000002,
    HYPERION_ERROR_MEMORY_POOL_EXHAUSTED    = 0x02000003,
    HYPERION_ERROR_MEMORY_ALIGNMENT_ERROR   = 0x02000004,
    HYPERION_ERROR_MEMORY_CORRUPTION        = 0x02000005,
    HYPERION_ERROR_MEMORY_LEAK_DETECTED     = 0x02000006,
    
    /* I/O errors (0x03xxxxxx) */
    HYPERION_ERROR_IO_FILE_NOT_FOUND        = 0x03000001,
    HYPERION_ERROR_IO_FILE_ACCESS_DENIED    = 0x03000002,
    HYPERION_ERROR_IO_FILE_CORRUPTED        = 0x03000003,
    HYPERION_ERROR_IO_WRITE_FAILED          = 0x03000004,
    HYPERION_ERROR_IO_READ_FAILED           = 0x03000005,
    HYPERION_ERROR_IO_DISK_FULL             = 0x03000006,
    HYPERION_ERROR_IO_INVALID_PATH          = 0x03000007,
    
    /* Model errors (0x04xxxxxx) */
    HYPERION_ERROR_MODEL_LOAD_FAILED        = 0x04000001,
    HYPERION_ERROR_MODEL_INVALID_FORMAT     = 0x04000002,
    HYPERION_ERROR_MODEL_UNSUPPORTED        = 0x04000003,
    HYPERION_ERROR_MODEL_QUANTIZATION_ERROR = 0x04000004,
    HYPERION_ERROR_MODEL_INFERENCE_FAILED   = 0x04000005,
    HYPERION_ERROR_MODEL_CONTEXT_OVERFLOW   = 0x04000006,
    HYPERION_ERROR_MODEL_TOKENIZATION_ERROR = 0x04000007,
    
    /* Configuration errors (0x05xxxxxx) */
    HYPERION_ERROR_CONFIG_INVALID_SYNTAX    = 0x05000001,
    HYPERION_ERROR_CONFIG_MISSING_REQUIRED  = 0x05000002,
    HYPERION_ERROR_CONFIG_INVALID_VALUE     = 0x05000003,
    HYPERION_ERROR_CONFIG_FILE_NOT_FOUND    = 0x05000004,
    HYPERION_ERROR_CONFIG_PARSE_ERROR       = 0x05000005,
    HYPERION_ERROR_CONFIG_VERSION_MISMATCH  = 0x05000006,
    
    /* Network/MCP errors (0x06xxxxxx) */
    HYPERION_ERROR_NETWORK_CONNECTION_FAILED = 0x06000001,
    HYPERION_ERROR_NETWORK_TIMEOUT          = 0x06000002,
    HYPERION_ERROR_NETWORK_PROTOCOL_ERROR   = 0x06000003,
    HYPERION_ERROR_MCP_SERVER_UNAVAILABLE   = 0x06000004,
    HYPERION_ERROR_MCP_INVALID_RESPONSE     = 0x06000005,
    HYPERION_ERROR_MCP_AUTHENTICATION_FAILED = 0x06000006,
    
    /* CLI errors (0x07xxxxxx) */
    HYPERION_ERROR_CLI_INVALID_COMMAND      = 0x07000001,
    HYPERION_ERROR_CLI_MISSING_ARGUMENT     = 0x07000002,
    HYPERION_ERROR_CLI_INVALID_ARGUMENT     = 0x07000003,
    HYPERION_ERROR_CLI_COMPLETION_FAILED    = 0x07000004,
    HYPERION_ERROR_CLI_HISTORY_ERROR        = 0x07000005,
    
    /* Runtime errors (0x08xxxxxx) */
    HYPERION_ERROR_RUNTIME_ASSERTION_FAILED = 0x08000001,
    HYPERION_ERROR_RUNTIME_STACK_OVERFLOW   = 0x08000002,
    HYPERION_ERROR_RUNTIME_DIVIDE_BY_ZERO   = 0x08000003,
    HYPERION_ERROR_RUNTIME_THREAD_ERROR     = 0x08000004,
    HYPERION_ERROR_RUNTIME_LOCK_ERROR       = 0x08000005,
    
    /* User errors (0x09xxxxxx) */
    HYPERION_ERROR_USER_INVALID_INPUT       = 0x09000001,
    HYPERION_ERROR_USER_OPERATION_CANCELLED = 0x09000002,
    HYPERION_ERROR_USER_VALIDATION_FAILED   = 0x09000003,
    HYPERION_ERROR_USER_QUOTA_EXCEEDED      = 0x09000004
} HyperionErrorCode;

/* Error context structure */
typedef struct {
    HyperionErrorCode code;                /* Error code */
    const char* file;                      /* Source file where error occurred */
    int line;                              /* Line number where error occurred */
    const char* function;                  /* Function where error occurred */
    char message[512];                     /* Error message */
    char suggestion[256];                  /* Suggested solution */
    uint64_t timestamp;                    /* Error timestamp */
    void* context_data;                    /* Additional context data */
    size_t context_size;                   /* Size of context data */
} HyperionErrorContext;

/* Error statistics */
typedef struct {
    uint32_t total_errors;                 /* Total number of errors */
    uint32_t errors_by_category[16];       /* Errors per category */
    uint32_t recent_errors[10];            /* Recent error codes */
    uint64_t last_error_time;              /* Last error timestamp */
    uint32_t recovery_attempts;            /* Number of recovery attempts */
    uint32_t successful_recoveries;        /* Successful recoveries */
} HyperionErrorStats;

/* Global error context */
extern HyperionErrorContext g_hyperion_last_error;
extern HyperionErrorStats g_hyperion_error_stats;

/* Core error functions */

/**
 * Set detailed error with context
 * 
 * @param code Error code
 * @param file Source file (__FILE__)
 * @param line Line number (__LINE__)
 * @param function Function name (__func__)
 * @param format Message format string
 * @param ... Format arguments
 */
void hyperion_set_error_detailed(HyperionErrorCode code, const char* file, int line,
                                 const char* function, const char* format, ...);

/**
 * Get the last error context
 * 
 * @return Pointer to last error context
 */
const HyperionErrorContext* hyperion_get_last_error(void);

/**
 * Clear the last error
 */
void hyperion_clear_error(void);

/**
 * Get error category from error code
 * 
 * @param code Error code
 * @return Error category
 */
HyperionErrorCategory hyperion_error_get_category(HyperionErrorCode code);

/**
 * Get human-readable error description
 * 
 * @param code Error code
 * @return Error description string
 */
const char* hyperion_error_get_description(HyperionErrorCode code);

/**
 * Get suggested solution for error
 * 
 * @param code Error code
 * @return Suggested solution string
 */
const char* hyperion_error_get_suggestion(HyperionErrorCode code);

/**
 * Check if error is recoverable
 * 
 * @param code Error code
 * @return 1 if recoverable, 0 if not
 */
int hyperion_error_is_recoverable(HyperionErrorCode code);

/**
 * Attempt automatic error recovery
 * 
 * @param code Error code
 * @return 1 if recovery succeeded, 0 if failed
 */
int hyperion_error_attempt_recovery(HyperionErrorCode code);

/**
 * Format error for display
 * 
 * @param error Error context
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @param include_context Whether to include file/line context
 * @return Number of characters written
 */
int hyperion_error_format(const HyperionErrorContext* error, char* buffer, 
                         size_t buffer_size, int include_context);

/**
 * Print error to file/stderr
 * 
 * @param error Error context
 * @param output Output file (NULL for stderr)
 * @param include_context Whether to include file/line context
 */
void hyperion_error_print(const HyperionErrorContext* error, FILE* output, 
                         int include_context);

/**
 * Get error statistics
 * 
 * @return Pointer to error statistics
 */
const HyperionErrorStats* hyperion_error_get_stats(void);

/**
 * Reset error statistics
 */
void hyperion_error_reset_stats(void);

/* Convenience macros */
#define HYPERION_SET_ERROR(code, ...) \
    hyperion_set_error_detailed(code, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define HYPERION_SET_ERROR_SIMPLE(code) \
    hyperion_set_error_detailed(code, __FILE__, __LINE__, __func__, \
                                hyperion_error_get_description(code))

#define HYPERION_CHECK_ERROR(expr) \
    do { \
        HyperionErrorCode _result = (expr); \
        if (_result != HYPERION_SUCCESS) { \
            HYPERION_SET_ERROR(_result, "Operation failed: %s", #expr); \
            return _result; \
        } \
    } while(0)

#define HYPERION_RETURN_IF_ERROR(expr) \
    do { \
        HyperionErrorCode _result = (expr); \
        if (_result != HYPERION_SUCCESS) { \
            return _result; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_ENHANCED_ERRORS_H */