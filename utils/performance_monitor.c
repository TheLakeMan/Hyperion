/**
 * Hyperion Performance Monitor Implementation
 */

#include "performance_monitor.h"
#include "../core/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define get_timestamp_us() (GetTickCount64() * 1000)
#else
#include <sys/time.h>
static uint64_t get_timestamp_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}
#endif

// Active operation tracking
typedef struct {
    uint64_t handle;
    uint64_t start_time_us;
    HyperionPerfCounterType type;
    size_t start_memory;
    char operation_name[64];
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
    HyperionPerfStats stats[8];         // One for each operation type
    
    // Memory tracking
    size_t current_memory;              // Current allocated memory
    size_t peak_memory;                 // Peak memory usage
    uint64_t total_allocations;         // Total allocations
    
    // Configuration
    bool enable_detailed_tracking;      // Enable detailed tracking
    bool verbose;                       // Verbose output
    uint64_t start_time_us;             // Monitor start time
};

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
    for (int i = 0; i < 8; i++) {
        monitor->stats[i].min_time_ms = 1e9; // Large initial value
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
            
            // Record the sample
            hyperionPerfRecord(monitor, op->type, op->operation_name, duration_ms, memory_used, result_code);
            
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
                       const char* operation_name, double duration_ms, size_t memory_used, int result_code)
{
    if (!monitor) return;
    
    // Add sample to circular buffer
    HyperionPerfSample* sample = &monitor->samples[monitor->current_index];
    
    sample->timestamp_us = get_timestamp_us();
    sample->duration_ms = duration_ms;
    sample->memory_used = memory_used;
    sample->memory_peak = monitor->peak_memory;
    sample->result_code = result_code;
    
    if (operation_name) {
        strncpy(sample->operation_name, operation_name, sizeof(sample->operation_name) - 1);
        sample->operation_name[sizeof(sample->operation_name) - 1] = '\0';
    } else {
        sample->operation_name[0] = '\0';
    }
    
    sample->additional_info[0] = '\0';
    
    // Update circular buffer indices
    monitor->current_index = (monitor->current_index + 1) % monitor->max_samples;
    if (monitor->sample_count < monitor->max_samples) {
        monitor->sample_count++;
    }
    
    // Update statistics
    if (type < 8) {
        HyperionPerfStats* stats = &monitor->stats[type];
        
        stats->total_operations++;
        stats->total_time_ms += duration_ms;
        
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
        
        // Calculate operations per second
        uint64_t elapsed_us = get_timestamp_us() - monitor->start_time_us;
        if (elapsed_us > 0) {
            stats->operations_per_second = (stats->total_operations * 1000000.0) / elapsed_us;
            stats->memory_mb_per_second = (stats->total_memory_allocated / (1024.0 * 1024.0)) / (elapsed_us / 1000000.0);
        }
    }
}

bool hyperionPerfGetStats(HyperionPerformanceMonitor* monitor, HyperionPerfCounterType type, HyperionPerfStats* stats)
{
    if (!monitor || !stats || type >= 8) return false;
    
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
        
        for (int i = 0; i < 8; i++) {
            HyperionPerfStats* stats = &monitor->stats[i];
            if (stats->total_operations > 0) {
                fprintf(output, "%s Statistics:\n", type_names[i]);
                fprintf(output, "  Operations: %llu\n", (unsigned long long)stats->total_operations);
                fprintf(output, "  Total Time: %.2f ms\n", stats->total_time_ms);
                fprintf(output, "  Average Time: %.2f ms\n", stats->avg_time_ms);
                fprintf(output, "  Min Time: %.2f ms\n", stats->min_time_ms);
                fprintf(output, "  Max Time: %.2f ms\n", stats->max_time_ms);
                fprintf(output, "  Operations/sec: %.2f\n", stats->operations_per_second);
                fprintf(output, "  Memory Throughput: %.2f MB/s\n", stats->memory_mb_per_second);
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
        for (int i = 0; i < 8; i++) {
            HyperionPerfStats* stats = &monitor->stats[i];
            if (stats->total_operations > 0) {
                if (!first) fprintf(output, ",\n");
                fprintf(output, "    \"%s\": {\n", type_keys[i]);
                fprintf(output, "      \"operations\": %llu,\n", (unsigned long long)stats->total_operations);
                fprintf(output, "      \"total_time_ms\": %.2f,\n", stats->total_time_ms);
                fprintf(output, "      \"average_time_ms\": %.2f,\n", stats->avg_time_ms);
                fprintf(output, "      \"min_time_ms\": %.2f,\n", stats->min_time_ms);
                fprintf(output, "      \"max_time_ms\": %.2f,\n", stats->max_time_ms);
                fprintf(output, "      \"operations_per_second\": %.2f,\n", stats->operations_per_second);
                fprintf(output, "      \"memory_throughput_mb_per_s\": %.2f\n", stats->memory_mb_per_second);
                fprintf(output, "    }");
                first = false;
            }
        }
        fprintf(output, "\n  }\n}\n");
    }
    else if (strcmp(format, "csv") == 0) {
        // Generate CSV report
        fprintf(output, "Type,Operations,Total Time (ms),Average Time (ms),Min Time (ms),Max Time (ms),Operations/sec,Memory Throughput (MB/s)\n");
        
        const char* type_names[] = {
            "Text Generation", "Model Loading", "Tokenization", "Memory Allocation",
            "SIMD Operations", "Network Request", "File I/O", "Custom"
        };
        
        for (int i = 0; i < 8; i++) {
            HyperionPerfStats* stats = &monitor->stats[i];
            if (stats->total_operations > 0) {
                fprintf(output, "%s,%llu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                        type_names[i],
                        (unsigned long long)stats->total_operations,
                        stats->total_time_ms,
                        stats->avg_time_ms,
                        stats->min_time_ms,
                        stats->max_time_ms,
                        stats->operations_per_second,
                        stats->memory_mb_per_second);
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
    for (int i = 0; i < 8; i++) {
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

bool hyperionPerfMonitorMemory(HyperionPerformanceMonitor* monitor, uint32_t interval_ms, uint32_t duration_ms)
{
    if (!monitor) return false;
    
    uint64_t start_time = get_timestamp_us();
    uint64_t end_time = start_time + (duration_ms * 1000);
    
    while (get_timestamp_us() < end_time) {
        // Get current memory stats
        size_t current_memory, peak_memory;
        uint64_t total_allocs;
        hyperionMemGetUsage(&current_memory, &peak_memory);
        
        // Update monitor's memory tracking
        monitor->current_memory = current_memory;
        if (peak_memory > monitor->peak_memory) {
            monitor->peak_memory = peak_memory;
        }
        
        // Record memory sample
        hyperionPerfRecord(monitor, HYPERION_PERF_MEMORY_ALLOCATION, "memory_sample", 
                          0.0, current_memory, 0);
        
        // Sleep for interval
#ifdef _WIN32
        Sleep(interval_ms);
#else
        usleep(interval_ms * 1000);
#endif
    }
    
    return true;
}