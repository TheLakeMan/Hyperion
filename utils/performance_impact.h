#ifndef HYPERION_PERFORMANCE_IMPACT_H
#define HYPERION_PERFORMANCE_IMPACT_H

#include "hyperion.h"
#include <stdbool.h>
#include <stddef.h>

// Performance impact configuration
typedef struct {
    bool   track_execution_time;  // Track execution time
    bool   track_memory_usage;    // Track memory usage
    bool   track_cpu_usage;       // Track CPU usage
    bool   track_cache_usage;     // Track cache usage
    bool   analyze_optimizations; // Analyze optimization impact
    size_t sample_interval_ms;    // Sampling interval in milliseconds
    size_t analysis_window_ms;    // Analysis window in milliseconds
} HyperionPerformanceConfig;

// Performance metrics
typedef struct {
    double execution_time;  // Execution time in milliseconds
    size_t memory_usage;    // Memory usage in bytes
    double cpu_usage;       // CPU usage percentage
    size_t cache_misses;    // Number of cache misses
    size_t cache_hits;      // Number of cache hits
    double cache_hit_ratio; // Cache hit ratio
} HyperionPerformanceMetrics;

// Optimization impact
typedef struct {
    double speedup_factor;      // Speedup compared to baseline
    double memory_reduction;    // Memory reduction percentage
    double cpu_efficiency;      // CPU efficiency improvement
    double cache_improvement;   // Cache performance improvement
    bool   is_beneficial;       // Whether optimization is beneficial
    char   recommendation[256]; // Optimization recommendation
} HyperionOptimizationImpact;

// Performance analysis context
typedef struct {
    HyperionPerformanceConfig  config;
    HyperionPerformanceMetrics baseline;
    HyperionPerformanceMetrics current;
    HyperionOptimizationImpact impact;
    uint64_t                 start_time;
    uint64_t                 last_sample_time;
} HyperionPerformanceAnalysis;

// Create performance analysis context
HyperionPerformanceAnalysis *hyperionCreatePerformanceAnalysis(const HyperionPerformanceConfig *config);

// Free performance analysis context
void hyperionFreePerformanceAnalysis(HyperionPerformanceAnalysis *analysis);

// Record performance metrics
void hyperionRecordMetrics(HyperionPerformanceAnalysis      *analysis,
                         const HyperionPerformanceMetrics *metrics);

// Take a performance sample
void hyperionTakePerformanceSample(HyperionPerformanceAnalysis *analysis);

// Get current performance metrics
HyperionPerformanceMetrics hyperionGetPerformanceMetrics(const HyperionPerformanceAnalysis *analysis);

// Analyze optimization impact
void hyperionAnalyzeOptimizationImpact(HyperionPerformanceAnalysis *analysis);

// Get optimization impact
HyperionOptimizationImpact hyperionGetOptimizationImpact(const HyperionPerformanceAnalysis *analysis);

// Generate performance report
void hyperionGeneratePerformanceReport(const HyperionPerformanceAnalysis *analysis,
                                     const char                      *filename);

// Get performance trend
double hyperionGetPerformanceTrend(const HyperionPerformanceAnalysis *analysis);

// Reset performance analysis
void hyperionResetPerformanceAnalysis(HyperionPerformanceAnalysis *analysis);

// Enable/disable performance analysis
void hyperionEnablePerformanceAnalysis(HyperionPerformanceAnalysis *analysis, bool enable);

// Set performance analysis configuration
void hyperionSetPerformanceConfig(HyperionPerformanceAnalysis     *analysis,
                                const HyperionPerformanceConfig *config);

#endif // HYPERION_PERFORMANCE_IMPACT_H