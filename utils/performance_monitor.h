/**
 * Hyperion Performance Monitor
 * 
 * Comprehensive performance monitoring and profiling system for Hyperion,
 * tracking memory usage, execution times, and model performance.
 */

#ifndef HYPERION_PERFORMANCE_MONITOR_H
#define HYPERION_PERFORMANCE_MONITOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Performance counter types
typedef enum {
    HYPERION_PERF_TEXT_GENERATION,
    HYPERION_PERF_MODEL_LOADING,
    HYPERION_PERF_TOKENIZATION,
    HYPERION_PERF_MEMORY_ALLOCATION,
    HYPERION_PERF_SIMD_OPERATIONS,
    HYPERION_PERF_NETWORK_REQUEST,
    HYPERION_PERF_FILE_IO,
    HYPERION_PERF_CUSTOM
} HyperionPerfCounterType;

// Performance sample
typedef struct {
    uint64_t timestamp_us;          // Timestamp in microseconds
    double duration_ms;             // Duration in milliseconds
    size_t memory_used;             // Memory usage in bytes
    size_t memory_peak;             // Peak memory during operation
    int result_code;                // Operation result code
    char operation_name[64];        // Operation name
    char additional_info[128];      // Additional information
} HyperionPerfSample;

// Performance statistics
typedef struct {
    uint64_t total_operations;      // Total number of operations
    double total_time_ms;           // Total time spent
    double min_time_ms;             // Minimum operation time
    double max_time_ms;             // Maximum operation time
    double avg_time_ms;             // Average operation time
    size_t total_memory_allocated;  // Total memory allocated
    size_t peak_memory_usage;       // Peak memory usage
    double operations_per_second;   // Operations per second
    double memory_mb_per_second;    // Memory throughput
} HyperionPerfStats;

// Performance monitor context
typedef struct HyperionPerformanceMonitor HyperionPerformanceMonitor;

/**
 * Create a performance monitor
 * 
 * @param max_samples Maximum number of samples to store
 * @param enable_detailed_tracking Enable detailed tracking (more overhead)
 * @return Performance monitor instance or NULL on failure
 */
HyperionPerformanceMonitor* hyperionPerfCreate(size_t max_samples, bool enable_detailed_tracking);

/**
 * Destroy a performance monitor
 * 
 * @param monitor Performance monitor instance
 */
void hyperionPerfDestroy(HyperionPerformanceMonitor* monitor);

/**
 * Start tracking an operation
 * 
 * @param monitor Performance monitor instance
 * @param type Operation type
 * @param operation_name Operation name
 * @return Operation handle (use with hyperionPerfEnd)
 */
uint64_t hyperionPerfBegin(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, const char* operation_name);

/**
 * End tracking an operation
 * 
 * @param monitor Performance monitor instance
 * @param handle Operation handle from hyperionPerfBegin
 * @param result_code Operation result code (0 = success)
 * @param additional_info Additional information (optional)
 */
void hyperionPerfEnd(HyperionPerformanceMonitor* monitor, uint64_t handle, int result_code, const char* additional_info);

/**
 * Record a single performance sample
 * 
 * @param monitor Performance monitor instance
 * @param type Operation type
 * @param operation_name Operation name
 * @param duration_ms Duration in milliseconds
 * @param memory_used Memory used in bytes
 * @param result_code Operation result code
 */
void hyperionPerfRecord(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, 
                       const char* operation_name, double duration_ms, size_t memory_used, int result_code);

/**
 * Get performance statistics for a specific operation type
 * 
 * @param monitor Performance monitor instance
 * @param type Operation type (or HYPERION_PERF_CUSTOM for all)
 * @param stats Output statistics structure
 * @return true if statistics are available, false otherwise
 */
bool hyperionPerfGetStats(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, HyperionPerfStats* stats);

/**
 * Get current memory usage statistics
 * 
 * @param monitor Performance monitor instance
 * @param current_allocated Output: currently allocated memory
 * @param peak_allocated Output: peak allocated memory
 * @param total_allocations Output: total number of allocations
 */
void hyperionPerfGetMemoryStats(HyperionPerformanceMonitor* monitor, size_t* current_allocated, 
                               size_t* peak_allocated, uint64_t* total_allocations);

/**
 * Generate a performance report
 * 
 * @param monitor Performance monitor instance
 * @param output_path Output file path (NULL for stdout)
 * @param format Report format ("text", "json", "csv")
 * @return true on success, false on failure
 */
bool hyperionPerfGenerateReport(HyperionPerformanceMonitor* monitor, const char* output_path, const char* format);

/**
 * Reset performance counters
 * 
 * @param monitor Performance monitor instance
 */
void hyperionPerfReset(HyperionPerformanceMonitor* monitor);

/**
 * Set performance monitoring verbosity
 * 
 * @param monitor Performance monitor instance
 * @param verbose Enable verbose output
 */
void hyperionPerfSetVerbose(HyperionPerformanceMonitor* monitor, bool verbose);

/**
 * Get the latest performance samples
 * 
 * @param monitor Performance monitor instance
 * @param samples Output buffer for samples
 * @param max_samples Maximum number of samples to retrieve
 * @return Number of samples retrieved
 */
size_t hyperionPerfGetLatestSamples(HyperionPerformanceMonitor* monitor, HyperionPerfSample* samples, size_t max_samples);

/**
 * Monitor memory usage continuously
 * 
 * @param monitor Performance monitor instance
 * @param interval_ms Monitoring interval in milliseconds
 * @param duration_ms Total monitoring duration in milliseconds
 * @return true on success, false on failure
 */
bool hyperionPerfMonitorMemory(HyperionPerformanceMonitor* monitor, uint32_t interval_ms, uint32_t duration_ms);

// Convenience macros for performance tracking
#define HYPERION_PERF_BEGIN(monitor, type, name) \
    uint64_t _perf_handle = hyperionPerfBegin(monitor, type, name)

#define HYPERION_PERF_END(monitor, result) \
    hyperionPerfEnd(monitor, _perf_handle, result, NULL)

#define HYPERION_PERF_END_WITH_INFO(monitor, result, info) \
    hyperionPerfEnd(monitor, _perf_handle, result, info)

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_PERFORMANCE_MONITOR_H */