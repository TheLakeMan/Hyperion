#ifndef HYPERION_MEMORY_ANALYSIS_H
#define HYPERION_MEMORY_ANALYSIS_H

#include "hyperion.h"
#include <stdbool.h>
#include <stddef.h>

// Memory analysis configuration
typedef struct {
    bool   track_allocations;   // Enable allocation tracking
    bool   track_deallocations; // Enable deallocation tracking
    bool   track_peak_usage;    // Track peak memory usage
    bool   analyze_patterns;    // Analyze memory usage patterns
    size_t sample_interval_ms;  // Sampling interval in milliseconds
    size_t analysis_window_ms;  // Analysis window in milliseconds
} HyperionMemoryAnalysisConfig;

// Memory allocation record
typedef struct {
    void       *address;   // Allocated memory address
    size_t      size;      // Allocation size
    const char *file;      // Source file
    int         line;      // Source line
    const char *function;  // Function name
    uint64_t    timestamp; // Allocation timestamp
    bool        is_freed;  // Whether the allocation was freed
} HyperionMemoryAllocation;

// Memory usage pattern
typedef struct {
    size_t total_allocations; // Total number of allocations
    size_t total_freed;       // Total number of freed allocations
    size_t peak_usage;        // Peak memory usage in bytes
    size_t current_usage;     // Current memory usage in bytes
    size_t fragmentation;     // Memory fragmentation percentage
    double allocation_rate;   // Allocations per second
    double deallocation_rate; // Deallocations per second
    double average_lifetime;  // Average allocation lifetime in milliseconds
} HyperionMemoryPattern;

// Memory analysis context
typedef struct {
    HyperionMemoryAnalysisConfig config;
    HyperionMemoryAllocation    *allocations;
    size_t                     max_allocations;
    size_t                     num_allocations;
    HyperionMemoryPattern        pattern;
    uint64_t                   start_time;
    uint64_t                   last_sample_time;
} HyperionMemoryAnalysis;

// Create memory analysis context
HyperionMemoryAnalysis *hyperionCreateMemoryAnalysis(const HyperionMemoryAnalysisConfig *config);

// Free memory analysis context
void hyperionFreeMemoryAnalysis(HyperionMemoryAnalysis *analysis);

// Record memory allocation
void hyperionRecordAllocation(HyperionMemoryAnalysis *analysis, void *address, size_t size,
                            const char *file, int line, const char *function);

// Record memory deallocation
void hyperionRecordDeallocation(HyperionMemoryAnalysis *analysis, void *address);

// Take a memory usage sample
void hyperionTakeMemorySample(HyperionMemoryAnalysis *analysis);

// Get current memory pattern
HyperionMemoryPattern hyperionGetMemoryPattern(const HyperionMemoryAnalysis *analysis);

// Analyze memory usage patterns
void hyperionAnalyzeMemoryPatterns(HyperionMemoryAnalysis *analysis);

// Generate memory usage report
void hyperionGenerateMemoryReport(const HyperionMemoryAnalysis *analysis, const char *filename);

// Get memory fragmentation
double hyperionGetMemoryFragmentation(const HyperionMemoryAnalysis *analysis);

// Get memory usage trend
double hyperionGetMemoryUsageTrend(const HyperionMemoryAnalysis *analysis);

// Get allocation hotspots
void hyperionGetAllocationHotspots(const HyperionMemoryAnalysis *analysis,
                                 HyperionMemoryAllocation **hotspots, size_t *num_hotspots);

// Get memory leak candidates
void hyperionGetMemoryLeakCandidates(const HyperionMemoryAnalysis *analysis,
                                   HyperionMemoryAllocation **leaks, size_t *num_leaks);

// Reset memory analysis
void hyperionResetMemoryAnalysis(HyperionMemoryAnalysis *analysis);

// Enable/disable memory analysis
void hyperionEnableMemoryAnalysis(HyperionMemoryAnalysis *analysis, bool enable);

// Set memory analysis configuration
void hyperionSetMemoryAnalysisConfig(HyperionMemoryAnalysis             *analysis,
                                   const HyperionMemoryAnalysisConfig *config);

#endif // HYPERION_MEMORY_ANALYSIS_H