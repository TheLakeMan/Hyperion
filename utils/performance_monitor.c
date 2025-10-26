/**
 * Hyperion Performance Monitor Implementation
 */

#include "performance_monitor.h"
#include "../core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#define get_timestamp_us() (GetTickCount64() * 1000)
#include <processthreadsapi.h>
#include <psapi.h>
#else
#include <sys/time.h>
static uint64_t get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
#include <unistd.h>
#include <sys/resource.h>
#endif

static double get_cpu_time_ms(void)
{
#ifdef _WIN32
    FILETIME creation, exit, kernel, user;
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        ULARGE_INTEGER user64;
        user64.LowPart = user.dwLowDateTime;
        user64.HighPart = user.dwHighDateTime;
        return user64.QuadPart / 10000.0; // Convert 100-ns units to ms
    }
    return 0.0;
#else
#ifdef CLOCK_PROCESS_CPUTIME_ID
    struct timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == 0) {
        return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
    }
#endif
    return ((double)clock() * 1000.0) / CLOCKS_PER_SEC;
#endif
}

static double compute_percentile(double *values, size_t count, double percentile)
{
    if (count == 0) {
        return 0.0;
    }
    if (count == 1) {
        return values[0];
    }
    double rank = percentile * (count - 1);
    size_t lower = (size_t)rank;
    size_t upper = (lower + 1 < count) ? lower + 1 : lower;
    double fraction = rank - lower;
    return values[lower] + (values[upper] - values[lower]) * fraction;
}

// Internal constants
#define HYPERION_PERF_MAX_TYPES 8
#define HYPERION_PERF_HISTORY   256

// Active operation tracking
typedef struct {
    uint64_t handle;
    uint64_t start_time_us;
    HyperionPerfCounterType type;
    size_t start_memory;
    char operation_name[64];
    double start_cpu_ms;
    bool active;
} ActiveOperation;

// Performance monitor implementation
struct HyperionPerformanceMonitor {
    HyperionPerfSample* samples;        // Circular buffer of samples
    size_t max_samples;                 // Maximum number of samples
    size_t sample_count;                // Current number of samples
    size_t current_index;               // Current write index
    
    ActiveOperation* active_ops;        // Active operations
    size_t max_active_ops;              // Maximum active operations
    uint64_t next_handle;               // Next operation handle
    
    // Statistics per operation type
    HyperionPerfStats stats[HYPERION_PERF_MAX_TYPES];   // One for each operation type
    double sum_squared_time[HYPERION_PERF_MAX_TYPES];   // Sum of squares for stddev
    double duration_window[HYPERION_PERF_MAX_TYPES][HYPERION_PERF_HISTORY];
    size_t duration_counts[HYPERION_PERF_MAX_TYPES];
    size_t duration_index[HYPERION_PERF_MAX_TYPES];
    
    // Memory tracking
    size_t current_memory;              // Current allocated memory
    size_t peak_memory;                 // Peak memory usage
    uint64_t total_allocations;         // Total allocations
    
    // Configuration
    bool enable_detailed_tracking;      // Enable detailed tracking
    bool verbose;                       // Verbose output
    uint64_t start_time_us;             // Monitor start time

    // Slow operation tracking
    double slow_threshold_ms[HYPERION_PERF_MAX_TYPES];
    HyperionPerfSlowOpCallback slow_callback[HYPERION_PERF_MAX_TYPES];
    void* slow_callback_user_data[HYPERION_PERF_MAX_TYPES];
    size_t slow_counts[HYPERION_PERF_MAX_TYPES];
    double slow_total_time_ms[HYPERION_PERF_MAX_TYPES];

    // CPU time tracking
    double cpu_time_total_ms[HYPERION_PERF_MAX_TYPES];
};

static void perfGetMemoryUsage(size_t *current_memory, size_t *peak_memory)
{
    size_t total = 0;
    size_t used = 0;
    size_t peak = 0;
    size_t allocs = 0;
    hyperionMemPoolStats(&total, &used, &peak, &allocs);
    if (current_memory) *current_memory = used;
    if (peak_memory) *peak_memory = peak;
}

static const char* hyperionPerfTypeKey(HyperionPerfCounterType type)
{
    switch (type) {
        case HYPERION_PERF_TEXT_GENERATION: return "text_generation";
        case HYPERION_PERF_MODEL_LOADING: return "model_loading";
        case HYPERION_PERF_TOKENIZATION: return "tokenization";
        case HYPERION_PERF_MEMORY_ALLOCATION: return "memory_allocation";
        case HYPERION_PERF_SIMD_OPERATIONS: return "simd_operations";
        case HYPERION_PERF_NETWORK_REQUEST: return "network_request";
        case HYPERION_PERF_FILE_IO: return "file_io";
        case HYPERION_PERF_CUSTOM:
        default: return "custom";
    }
}

static size_t collect_recent_durations(const HyperionPerformanceMonitor *monitor, HyperionPerfCounterType type,
                                       double *out_buffer, size_t buffer_capacity)
{
    if (type >= HYPERION_PERF_MAX_TYPES || buffer_capacity < HYPERION_PERF_HISTORY) {
        return 0;
    }

    size_t count = monitor->duration_counts[type];
    if (count == 0) {
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        size_t index = (monitor->duration_index[type] + HYPERION_PERF_HISTORY - count + i) % HYPERION_PERF_HISTORY;
        out_buffer[i] = monitor->duration_window[type][index];
    }
    return count;
}

static void update_advanced_statistics(HyperionPerformanceMonitor *monitor, HyperionPerfCounterType type)
{
    if (type >= HYPERION_PERF_MAX_TYPES) {
        return;
    }

    HyperionPerfStats *stats = &monitor->stats[type];
    if (stats->total_operations == 0) {
        return;
    }

    double mean = stats->total_time_ms / stats->total_operations;
    double variance = 0.0;
    double sum_sq = monitor->sum_squared_time[type];
    if (stats->total_operations > 1) {
        variance = (sum_sq / stats->total_operations) - (mean * mean);
        if (variance < 0.0) variance = 0.0;
    }
    stats->stddev_time_ms = (variance > 0.0 && stats->total_operations > 1) ? sqrt(variance) : 0.0;

    double durations[HYPERION_PERF_HISTORY];
    size_t count = collect_recent_durations(monitor, type, durations, HYPERION_PERF_HISTORY);
    if (count > 0) {
        // Sort durations in ascending order for percentile calculation
        for (size_t i = 1; i < count; i++) {
            double key = durations[i];
            size_t j = i;
            while (j > 0 && durations[j - 1] > key) {
                durations[j] = durations[j - 1];
                j--;
            }
            durations[j] = key;
        }

        stats->percentile_50_ms = compute_percentile(durations, count, 0.50);
        stats->percentile_90_ms = compute_percentile(durations, count, 0.90);
        stats->percentile_95_ms = compute_percentile(durations, count, 0.95);
        stats->percentile_99_ms = compute_percentile(durations, count, 0.99);
    } else {
        stats->percentile_50_ms = 0.0;
        stats->percentile_90_ms = 0.0;
        stats->percentile_95_ms = 0.0;
        stats->percentile_99_ms = 0.0;
    }

    double total_time = stats->total_time_ms > 0.0 ? stats->total_time_ms : 1.0;
    stats->cpu_time_total_ms = monitor->cpu_time_total_ms[type];
    stats->avg_cpu_time_ms = stats->cpu_time_total_ms / stats->total_operations;
    stats->cpu_utilization_percent = (stats->cpu_time_total_ms / total_time) * 100.0;
    if (stats->cpu_utilization_percent < 0.0) stats->cpu_utilization_percent = 0.0;
}
HyperionPerformanceMonitor* hyperionPerfCreate(size_t max_samples, bool enable_detailed_tracking)
{
    HyperionPerformanceMonitor* monitor = (HyperionPerformanceMonitor*)HYPERION_MALLOC(sizeof(HyperionPerformanceMonitor));
    if (!monitor) return NULL;
    
    memset(monitor, 0, sizeof(HyperionPerformanceMonitor));
    
    // Allocate sample buffer
    monitor->samples = (HyperionPerfSample*)HYPERION_MALLOC(sizeof(HyperionPerfSample) * max_samples);
    if (!monitor->samples) {
        HYPERION_FREE(monitor);
        return NULL;
    }
    
    // Allocate active operations buffer
    monitor->max_active_ops = 32; // Support up to 32 concurrent operations
    monitor->active_ops = (ActiveOperation*)HYPERION_MALLOC(sizeof(ActiveOperation) * monitor->max_active_ops);
    if (!monitor->active_ops) {
        HYPERION_FREE(monitor->samples);
        HYPERION_FREE(monitor);
        return NULL;
    }
    
    monitor->max_samples = max_samples;
    monitor->enable_detailed_tracking = enable_detailed_tracking;
    monitor->start_time_us = get_timestamp_us();
    monitor->next_handle = 1;
    
    // Initialize statistics
    for (int i = 0; i < HYPERION_PERF_MAX_TYPES; i++) {
        monitor->stats[i].min_time_ms = 1e9; // Large initial value
        monitor->slow_threshold_ms[i] = -1.0;
    }
    
    return monitor;
}

void hyperionPerfDestroy(HyperionPerformanceMonitor* monitor)
{
    if (!monitor) return;
    
    HYPERION_FREE(monitor->samples);
    HYPERION_FREE(monitor->active_ops);
    HYPERION_FREE(monitor);
}

uint64_t hyperionPerfBegin(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, const char* operation_name)
{
    if (!monitor) return 0;
    
    // Find free slot in active operations
    for (size_t i = 0; i < monitor->max_active_ops; i++) {
        if (!monitor->active_ops[i].active) {
            ActiveOperation* op = &monitor->active_ops[i];
            
            op->handle = monitor->next_handle++;
            op->start_time_us = get_timestamp_us();
            op->type = type;
            op->start_memory = monitor->current_memory;
            op->start_cpu_ms = get_cpu_time_ms();
            op->active = true;
            
            if (operation_name) {
                strncpy(op->operation_name, operation_name, sizeof(op->operation_name) - 1);
                op->operation_name[sizeof(op->operation_name) - 1] = '\0';
            } else {
                op->operation_name[0] = '\0';
            }
            
            if (monitor->verbose) {
                printf("[PERF] Started operation %s (handle: %llu)\n", 
                       operation_name ? operation_name : "unknown", 
                       (unsigned long long)op->handle);
            }
            
            return op->handle;
        }
    }
    
    return 0; // No free slots
}

void hyperionPerfEnd(HyperionPerformanceMonitor* monitor, uint64_t handle, int result_code, const char* additional_info)
{
    if (!monitor || handle == 0) return;
    
    uint64_t end_time_us = get_timestamp_us();
    
    // Find the active operation
    for (size_t i = 0; i < monitor->max_active_ops; i++) {
        ActiveOperation* op = &monitor->active_ops[i];
        if (op->active && op->handle == handle) {
            
            double duration_ms = (end_time_us - op->start_time_us) / 1000.0;
            size_t memory_used = monitor->current_memory - op->start_memory;
            double cpu_time_ms = get_cpu_time_ms() - op->start_cpu_ms;
            if (cpu_time_ms < 0.0) cpu_time_ms = 0.0;
            
            // Record the sample
            hyperionPerfRecord(monitor, op->type, op->operation_name, duration_ms, memory_used,
                               result_code, additional_info, cpu_time_ms);
            
            if (monitor->verbose) {
                printf("[PERF] Completed operation %s (%.2f ms, %zu bytes)\n", 
                       op->operation_name, duration_ms, memory_used);
            }
            
            // Mark operation as inactive
            op->active = false;
            break;
        }
    }
}

void hyperionPerfRecord(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, 
                       const char* operation_name, double duration_ms, size_t memory_used, int result_code,
                       const char* additional_info, double cpu_time_ms)
{
    if (!monitor) return;
    
    // Add sample to circular buffer
    HyperionPerfSample* sample = &monitor->samples[monitor->current_index];
    
    sample->type = type;
    sample->timestamp_us = get_timestamp_us();
    sample->duration_ms = duration_ms;
    sample->cpu_time_ms = cpu_time_ms;
    if (duration_ms > 0.0) {
        sample->cpu_utilization_percent = (cpu_time_ms / duration_ms) * 100.0;
        if (sample->cpu_utilization_percent < 0.0) sample->cpu_utilization_percent = 0.0;
    } else {
        sample->cpu_utilization_percent = 0.0;
    }
    sample->memory_used = memory_used;
    sample->memory_peak = monitor->peak_memory;
    sample->result_code = result_code;
    
    if (operation_name) {
        strncpy(sample->operation_name, operation_name, sizeof(sample->operation_name) - 1);
        sample->operation_name[sizeof(sample->operation_name) - 1] = '\0';
    } else {
        sample->operation_name[0] = '\0';
    }

    if (additional_info) {
        strncpy(sample->additional_info, additional_info, sizeof(sample->additional_info) - 1);
        sample->additional_info[sizeof(sample->additional_info) - 1] = '\0';
    } else {
        sample->additional_info[0] = '\0';
    }
    
    // Update circular buffer indices
    monitor->current_index = (monitor->current_index + 1) % monitor->max_samples;
    if (monitor->sample_count < monitor->max_samples) {
        monitor->sample_count++;
    }
    
    // Update statistics
    if (type < HYPERION_PERF_MAX_TYPES) {
        HyperionPerfStats* stats = &monitor->stats[type];
        
        stats->total_operations++;
        stats->total_time_ms += duration_ms;
        monitor->sum_squared_time[type] += duration_ms * duration_ms;
        if (monitor->duration_counts[type] < HYPERION_PERF_HISTORY) {
            size_t idx = monitor->duration_counts[type];
            monitor->duration_window[type][idx] = duration_ms;
            monitor->duration_counts[type]++;
            monitor->duration_index[type] = monitor->duration_counts[type] % HYPERION_PERF_HISTORY;
        } else {
            monitor->duration_window[type][monitor->duration_index[type]] = duration_ms;
            monitor->duration_index[type] = (monitor->duration_index[type] + 1) % HYPERION_PERF_HISTORY;
        }
        
        if (duration_ms < stats->min_time_ms) {
            stats->min_time_ms = duration_ms;
        }
        if (duration_ms > stats->max_time_ms) {
            stats->max_time_ms = duration_ms;
        }
        
        stats->avg_time_ms = stats->total_time_ms / stats->total_operations;
        stats->total_memory_allocated += memory_used;
        
        if (monitor->current_memory > stats->peak_memory_usage) {
            stats->peak_memory_usage = monitor->current_memory;
        }

        monitor->cpu_time_total_ms[type] += cpu_time_ms;
        stats->cpu_time_total_ms = monitor->cpu_time_total_ms[type];
        
        // Calculate operations per second
        uint64_t elapsed_us = get_timestamp_us() - monitor->start_time_us;
        if (elapsed_us > 0) {
            stats->operations_per_second = (stats->total_operations * 1000000.0) / elapsed_us;
            stats->memory_mb_per_second = (stats->total_memory_allocated / (1024.0 * 1024.0)) / (elapsed_us / 1000000.0);
        }

        // Slow operation tracking
        if (monitor->slow_threshold_ms[type] > 0.0 && duration_ms >= monitor->slow_threshold_ms[type]) {
            stats->slow_operation_count++;
            monitor->slow_counts[type]++;
            monitor->slow_total_time_ms[type] += duration_ms;
            stats->slow_average_over_threshold_ms = monitor->slow_total_time_ms[type] / monitor->slow_counts[type];
            if (duration_ms > stats->slowest_operation_ms) {
                stats->slowest_operation_ms = duration_ms;
            }
            if (monitor->slow_callback[type]) {
                monitor->slow_callback[type](sample, monitor->slow_callback_user_data[type]);
            }
        }

        update_advanced_statistics(monitor, type);
    }
}

bool hyperionPerfGetStats(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, HyperionPerfStats* stats)
{
    if (!monitor || !stats || type >= HYPERION_PERF_MAX_TYPES) return false;
    
    *stats = monitor->stats[type];
    return stats->total_operations > 0;
}

void hyperionPerfGetMemoryStats(HyperionPerformanceMonitor* monitor, size_t* current_allocated, 
                               size_t* peak_allocated, uint64_t* total_allocations)
{
    if (!monitor) return;
    
    if (current_allocated) *current_allocated = monitor->current_memory;
    if (peak_allocated) *peak_allocated = monitor->peak_memory;
    if (total_allocations) *total_allocations = monitor->total_allocations;
}

bool hyperionPerfGenerateReport(HyperionPerformanceMonitor* monitor, const char* output_path, const char* format)
{
    if (!monitor) return false;
    
    FILE* output = stdout;
    bool close_file = false;
    
    if (output_path) {
        output = fopen(output_path, "w");
        if (!output) return false;
        close_file = true;
    }
    
    if (!format || strcmp(format, "text") == 0) {
        // Generate text report
        fprintf(output, "Hyperion Performance Report\n");
        fprintf(output, "===========================\n\n");
        
        uint64_t elapsed_us = get_timestamp_us() - monitor->start_time_us;
        fprintf(output, "Monitoring Duration: %.2f seconds\n", elapsed_us / 1000000.0);
        fprintf(output, "Total Samples: %zu\n", monitor->sample_count);
        fprintf(output, "Current Memory: %.2f MB\n", monitor->current_memory / (1024.0 * 1024.0));
        fprintf(output, "Peak Memory: %.2f MB\n", monitor->peak_memory / (1024.0 * 1024.0));
        fprintf(output, "\n");
        
        const char* type_names[] = {
            "Text Generation", "Model Loading", "Tokenization", "Memory Allocation",
            "SIMD Operations", "Network Request", "File I/O", "Custom"
        };
        
        for (int i = 0; i < HYPERION_PERF_MAX_TYPES; i++) {
            HyperionPerfStats* stats = &monitor->stats[i];
            if (stats->total_operations > 0) {
                fprintf(output, "%s Statistics:\n", type_names[i]);
                fprintf(output, "  Operations: %llu\n", (unsigned long long)stats->total_operations);
                fprintf(output, "  Total Time: %.2f ms\n", stats->total_time_ms);
                fprintf(output, "  Average Time: %.2f ms\n", stats->avg_time_ms);
                fprintf(output, "  Min Time: %.2f ms\n", stats->min_time_ms);
                fprintf(output, "  Max Time: %.2f ms\n", stats->max_time_ms);
                fprintf(output, "  Std Dev: %.2f ms\n", stats->stddev_time_ms);
                fprintf(output, "  P50/P90/P95/P99: %.2f / %.2f / %.2f / %.2f ms\n",
                        stats->percentile_50_ms,
                        stats->percentile_90_ms,
                        stats->percentile_95_ms,
                        stats->percentile_99_ms);
                fprintf(output, "  Operations/sec: %.2f\n", stats->operations_per_second);
                fprintf(output, "  Memory Throughput: %.2f MB/s\n", stats->memory_mb_per_second);
                fprintf(output, "  CPU Avg: %.2f ms (%.2f%%)\n",
                        stats->avg_cpu_time_ms,
                        stats->cpu_utilization_percent);
                fprintf(output, "  Slow Ops: %zu (slowest %.2f ms, avg over threshold %.2f ms)\n",
                        stats->slow_operation_count,
                        stats->slowest_operation_ms,
                        stats->slow_average_over_threshold_ms);
                fprintf(output, "\n");
            }
        }
    }
    else if (strcmp(format, "json") == 0) {
        // Generate JSON report
        fprintf(output, "{\n");
        fprintf(output, "  \"monitoring_duration_seconds\": %.2f,\n", (get_timestamp_us() - monitor->start_time_us) / 1000000.0);
        fprintf(output, "  \"total_samples\": %zu,\n", monitor->sample_count);
        fprintf(output, "  \"current_memory_mb\": %.2f,\n", monitor->current_memory / (1024.0 * 1024.0));
        fprintf(output, "  \"peak_memory_mb\": %.2f,\n", monitor->peak_memory / (1024.0 * 1024.0));
        fprintf(output, "  \"statistics\": {\n");
        
        const char* type_keys[] = {
            "text_generation", "model_loading", "tokenization", "memory_allocation",
            "simd_operations", "network_request", "file_io", "custom"
        };
        
        bool first = true;
        for (int i = 0; i < HYPERION_PERF_MAX_TYPES; i++) {
            HyperionPerfStats* stats = &monitor->stats[i];
            if (stats->total_operations > 0) {
                if (!first) fprintf(output, ",\n");
                fprintf(output, "    \"%s\": {\n", type_keys[i]);
                fprintf(output, "      \"operations\": %llu,\n", (unsigned long long)stats->total_operations);
                fprintf(output, "      \"total_time_ms\": %.2f,\n", stats->total_time_ms);
                fprintf(output, "      \"average_time_ms\": %.2f,\n", stats->avg_time_ms);
                fprintf(output, "      \"min_time_ms\": %.2f,\n", stats->min_time_ms);
                fprintf(output, "      \"max_time_ms\": %.2f,\n", stats->max_time_ms);
                fprintf(output, "      \"stddev_time_ms\": %.2f,\n", stats->stddev_time_ms);
                fprintf(output, "      \"percentiles_ms\": {\"p50\": %.2f, \"p90\": %.2f, \"p95\": %.2f, \"p99\": %.2f},\n",
                        stats->percentile_50_ms,
                        stats->percentile_90_ms,
                        stats->percentile_95_ms,
                        stats->percentile_99_ms);
                fprintf(output, "      \"operations_per_second\": %.2f,\n", stats->operations_per_second);
                fprintf(output, "      \"memory_throughput_mb_per_s\": %.2f,\n", stats->memory_mb_per_second);
                fprintf(output, "      \"cpu_time_ms\": {\"total\": %.2f, \"avg\": %.2f, \"utilization_percent\": %.2f},\n",
                        stats->cpu_time_total_ms,
                        stats->avg_cpu_time_ms,
                        stats->cpu_utilization_percent);
                fprintf(output, "      \"slow_operations\": {\"count\": %zu, \"slowest_ms\": %.2f, \"avg_over_threshold_ms\": %.2f}\n",
                        stats->slow_operation_count,
                        stats->slowest_operation_ms,
                        stats->slow_average_over_threshold_ms);
                fprintf(output, "    }");
                first = false;
            }
        }
        fprintf(output, "\n  }\n}\n");
    }
    else if (strcmp(format, "csv") == 0) {
        // Generate CSV report
        fprintf(output, "Type,Operations,Total Time (ms),Average Time (ms),Min Time (ms),Max Time (ms),Std Dev (ms),P50 (ms),P90 (ms),P95 (ms),P99 (ms),Operations/sec,Memory Throughput (MB/s),CPU Total (ms),CPU Avg (ms),CPU Util (%%),Slow Ops,Slowest (ms),Avg Slow (ms)\n");
        
        const char* type_names[] = {
            "Text Generation", "Model Loading", "Tokenization", "Memory Allocation",
            "SIMD Operations", "Network Request", "File I/O", "Custom"
        };
        
        for (int i = 0; i < HYPERION_PERF_MAX_TYPES; i++) {
            HyperionPerfStats* stats = &monitor->stats[i];
            if (stats->total_operations > 0) {
                fprintf(output, "%s,%llu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,",
                        type_names[i],
                        (unsigned long long)stats->total_operations,
                        stats->total_time_ms,
                        stats->avg_time_ms,
                        stats->min_time_ms,
                        stats->max_time_ms,
                        stats->stddev_time_ms,
                        stats->percentile_50_ms,
                        stats->percentile_90_ms,
                        stats->percentile_95_ms,
                        stats->percentile_99_ms,
                        stats->operations_per_second,
                        stats->memory_mb_per_second,
                        stats->cpu_time_total_ms,
                        stats->avg_cpu_time_ms,
                        stats->cpu_utilization_percent);

                fprintf(output, "%.0f,%.2f,%.2f\n",
                        (double)stats->slow_operation_count,
                        stats->slowest_operation_ms,
                        stats->slow_average_over_threshold_ms);
            }
        }
    }
    
    if (close_file) {
        fclose(output);
    }
    
    return true;
}

void hyperionPerfReset(HyperionPerformanceMonitor* monitor)
{
    if (!monitor) return;
    
    monitor->sample_count = 0;
    monitor->current_index = 0;
    monitor->start_time_us = get_timestamp_us();
    
    // Reset all statistics
    memset(monitor->stats, 0, sizeof(monitor->stats));
    memset(monitor->sum_squared_time, 0, sizeof(monitor->sum_squared_time));
    memset(monitor->duration_window, 0, sizeof(monitor->duration_window));
    memset(monitor->duration_counts, 0, sizeof(monitor->duration_counts));
    memset(monitor->duration_index, 0, sizeof(monitor->duration_index));
    memset(monitor->slow_counts, 0, sizeof(monitor->slow_counts));
    memset(monitor->slow_total_time_ms, 0, sizeof(monitor->slow_total_time_ms));
    memset(monitor->cpu_time_total_ms, 0, sizeof(monitor->cpu_time_total_ms));
    for (int i = 0; i < HYPERION_PERF_MAX_TYPES; i++) {
        monitor->stats[i].min_time_ms = 1e9;
    }
    
    // Mark all operations as inactive
    for (size_t i = 0; i < monitor->max_active_ops; i++) {
        monitor->active_ops[i].active = false;
    }
}

void hyperionPerfSetVerbose(HyperionPerformanceMonitor* monitor, bool verbose)
{
    if (monitor) {
        monitor->verbose = verbose;
    }
}

size_t hyperionPerfGetLatestSamples(HyperionPerformanceMonitor* monitor, HyperionPerfSample* samples, size_t max_samples)
{
    if (!monitor || !samples) return 0;
    
    size_t samples_to_copy = (monitor->sample_count < max_samples) ? monitor->sample_count : max_samples;
    
    for (size_t i = 0; i < samples_to_copy; i++) {
        size_t source_index = (monitor->current_index - samples_to_copy + i + monitor->max_samples) % monitor->max_samples;
        samples[i] = monitor->samples[source_index];
    }
    
    return samples_to_copy;
}

void hyperionPerfSetSlowCallback(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type,
                                 double threshold_ms, HyperionPerfSlowOpCallback callback, void* user_data)
{
    if (!monitor || type >= HYPERION_PERF_MAX_TYPES) {
        return;
    }

    if (threshold_ms <= 0.0 || !callback) {
        monitor->slow_threshold_ms[type] = -1.0;
        monitor->slow_callback[type] = NULL;
        monitor->slow_callback_user_data[type] = NULL;
        return;
    }

    monitor->slow_threshold_ms[type] = threshold_ms;
    monitor->slow_callback[type] = callback;
    monitor->slow_callback_user_data[type] = user_data;
}

bool hyperionPerfMonitorMemory(HyperionPerformanceMonitor* monitor, uint32_t interval_ms, uint32_t duration_ms)
{
    if (!monitor) return false;
    
    uint64_t start_time = get_timestamp_us();
    uint64_t end_time = start_time + (duration_ms * 1000);
    
    while (get_timestamp_us() < end_time) {
        // Get current memory stats
        size_t current_memory, peak_memory;
        perfGetMemoryUsage(&current_memory, &peak_memory);
        
        // Update monitor's memory tracking
        monitor->current_memory = current_memory;
        if (peak_memory > monitor->peak_memory) {
            monitor->peak_memory = peak_memory;
        }
        
        // Record memory sample
        char info[64];
        snprintf(info, sizeof(info), "current=%zu", current_memory);
        hyperionPerfRecord(monitor, HYPERION_PERF_MEMORY_ALLOCATION, "memory_sample", 
                          0.0, current_memory, 0, info, 0.0);
        
        // Sleep for interval
#ifdef _WIN32
        Sleep(interval_ms);
#else
        usleep(interval_ms * 1000);
#endif
    }
    
    return true;
}

bool hyperionPerfExportTimeline(HyperionPerformanceMonitor* monitor, const char* output_path,
                                HyperionPerfCounterType type, size_t max_events)
{
    if (!monitor || !output_path || max_events == 0) {
        return false;
    }

    FILE *output = fopen(output_path, "w");
    if (!output) {
        return false;
    }

    size_t request = (max_events < monitor->sample_count) ? max_events : monitor->sample_count;
    if (request == 0) {
        fprintf(output, "[]\n");
        fclose(output);
        return true;
    }

    HyperionPerfSample *samples = (HyperionPerfSample*)HYPERION_CALLOC(request, sizeof(HyperionPerfSample));
    if (!samples) {
        fclose(output);
        return false;
    }

    size_t retrieved = hyperionPerfGetLatestSamples(monitor, samples, request);
    fprintf(output, "[\n");
    size_t written = 0;
    for (size_t i = 0; i < retrieved && written < max_events; i++) {
        const HyperionPerfSample *sample = &samples[i];
        if (type != HYPERION_PERF_CUSTOM && sample->type != type) {
            continue;
        }
        if (written > 0) {
            fprintf(output, ",\n");
        }
        fprintf(output,
                "  {\"type\":\"%s\",\"timestamp_us\":%llu,\"duration_ms\":%.6f,\"cpu_time_ms\":%.6f,\"cpu_percent\":%.3f,"
                "\"memory_used\":%zu,\"memory_peak\":%zu,\"result_code\":%d,\"operation\":\"%s\",\"info\":\"",
                hyperionPerfTypeKey(sample->type),
                (unsigned long long)sample->timestamp_us,
                sample->duration_ms,
                sample->cpu_time_ms,
                sample->cpu_utilization_percent,
                sample->memory_used,
                sample->memory_peak,
                sample->result_code,
                sample->operation_name);

        // Escape quotes in additional info
        const char *info = sample->additional_info;
        for (size_t j = 0; info && info[j] != '\0'; j++) {
            char ch = info[j];
            if (ch == '"' || ch == '\\') {
                fputc('\\', output);
            }
            fputc(ch, output);
        }
        fprintf(output, "\"}");
        written++;
    }
    fprintf(output, "\n]\n");

    HYPERION_FREE(samples);
    fclose(output);
    return true;
}