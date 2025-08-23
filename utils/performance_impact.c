#include "performance_impact.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

// Default configuration
static const HyperionPerformanceConfig DEFAULT_CONFIG = {.track_execution_time  = true,
                                                       .track_memory_usage    = true,
                                                       .track_cpu_usage       = true,
                                                       .track_cache_usage     = true,
                                                       .analyze_optimizations = true,
                                                       .sample_interval_ms    = 100,
                                                       .analysis_window_ms    = 1000};

// Get current timestamp in milliseconds
static uint64_t get_timestamp_ms()
{
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000) / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

// Create performance analysis context
HyperionPerformanceAnalysis *hyperionCreatePerformanceAnalysis(const HyperionPerformanceConfig *config)
{
    HyperionPerformanceAnalysis *analysis = malloc(sizeof(HyperionPerformanceAnalysis));
    if (!analysis)
        return NULL;

    // Initialize with default or provided config
    if (config) {
        analysis->config = *config;
    }
    else {
        analysis->config = DEFAULT_CONFIG;
    }

    // Initialize metrics
    memset(&analysis->baseline, 0, sizeof(HyperionPerformanceMetrics));
    memset(&analysis->current, 0, sizeof(HyperionPerformanceMetrics));
    memset(&analysis->impact, 0, sizeof(HyperionOptimizationImpact));

    // Initialize timestamps
    analysis->start_time       = get_timestamp_ms();
    analysis->last_sample_time = analysis->start_time;

    return analysis;
}

// Free performance analysis context
void hyperionFreePerformanceAnalysis(HyperionPerformanceAnalysis *analysis)
{
    if (!analysis)
        return;
    free(analysis);
}

// Record performance metrics
void hyperionRecordMetrics(HyperionPerformanceAnalysis      *analysis,
                         const HyperionPerformanceMetrics *metrics)
{
    if (!analysis || !metrics)
        return;

    // Update current metrics
    if (analysis->config.track_execution_time) {
        analysis->current.execution_time = metrics->execution_time;
    }
    if (analysis->config.track_memory_usage) {
        analysis->current.memory_usage = metrics->memory_usage;
    }
    if (analysis->config.track_cpu_usage) {
        analysis->current.cpu_usage = metrics->cpu_usage;
    }
    if (analysis->config.track_cache_usage) {
        analysis->current.cache_misses    = metrics->cache_misses;
        analysis->current.cache_hits      = metrics->cache_hits;
        analysis->current.cache_hit_ratio = metrics->cache_hit_ratio;
    }
}

// Take a performance sample
void hyperionTakePerformanceSample(HyperionPerformanceAnalysis *analysis)
{
    if (!analysis)
        return;

    uint64_t current_time = get_timestamp_ms();
    uint64_t elapsed_ms   = current_time - analysis->last_sample_time;

    if (elapsed_ms >= analysis->config.sample_interval_ms) {
        // Update metrics based on current state
        // This is a placeholder - actual implementation would measure real metrics
        HyperionPerformanceMetrics metrics = {
            .execution_time  = 0.0, // Would be measured
            .memory_usage    = 0,   // Would be measured
            .cpu_usage       = 0.0, // Would be measured
            .cache_misses    = 0,   // Would be measured
            .cache_hits      = 0,   // Would be measured
            .cache_hit_ratio = 0.0  // Would be calculated
        };

        hyperionRecordMetrics(analysis, &metrics);
        analysis->last_sample_time = current_time;
    }
}

// Get current performance metrics
HyperionPerformanceMetrics hyperionGetPerformanceMetrics(const HyperionPerformanceAnalysis *analysis)
{
    if (!analysis) {
        HyperionPerformanceMetrics empty = {0};
        return empty;
    }
    return analysis->current;
}

// Analyze optimization impact
void hyperionAnalyzeOptimizationImpact(HyperionPerformanceAnalysis *analysis)
{
    if (!analysis || !analysis->config.analyze_optimizations)
        return;

    // Calculate impact metrics
    if (analysis->baseline.execution_time > 0) {
        analysis->impact.speedup_factor =
            analysis->baseline.execution_time / analysis->current.execution_time;
    }
    if (analysis->baseline.memory_usage > 0) {
        analysis->impact.memory_reduction = 100.0 * (1.0 - (double)analysis->current.memory_usage /
                                                               analysis->baseline.memory_usage);
    }
    if (analysis->baseline.cpu_usage > 0) {
        analysis->impact.cpu_efficiency =
            analysis->baseline.cpu_usage / analysis->current.cpu_usage;
    }
    if (analysis->baseline.cache_hit_ratio > 0) {
        analysis->impact.cache_improvement =
            analysis->current.cache_hit_ratio / analysis->baseline.cache_hit_ratio;
    }

    // Determine if optimization is beneficial
    analysis->impact.is_beneficial =
        (analysis->impact.speedup_factor > 1.0) || (analysis->impact.memory_reduction > 0.0) ||
        (analysis->impact.cpu_efficiency > 1.0) || (analysis->impact.cache_improvement > 1.0);

    // Generate recommendation
    if (analysis->impact.is_beneficial) {
        snprintf(analysis->impact.recommendation, sizeof(analysis->impact.recommendation),
                 "Optimization is beneficial. Speedup: %.2fx, Memory reduction: %.1f%%, CPU "
                 "efficiency: %.2fx, Cache improvement: %.2fx",
                 analysis->impact.speedup_factor, analysis->impact.memory_reduction,
                 analysis->impact.cpu_efficiency, analysis->impact.cache_improvement);
    }
    else {
        snprintf(analysis->impact.recommendation, sizeof(analysis->impact.recommendation),
                 "Optimization may not be beneficial. Consider reverting changes or trying "
                 "different optimizations.");
    }
}

// Get optimization impact
HyperionOptimizationImpact hyperionGetOptimizationImpact(const HyperionPerformanceAnalysis *analysis)
{
    if (!analysis) {
        HyperionOptimizationImpact empty = {0};
        return empty;
    }
    return analysis->impact;
}

// Generate performance report
void hyperionGeneratePerformanceReport(const HyperionPerformanceAnalysis *analysis,
                                     const char                      *filename)
{
    if (!analysis || !filename)
        return;

    FILE *file = fopen(filename, "w");
    if (!file)
        return;

    fprintf(file, "Performance Analysis Report\n");
    fprintf(file, "=========================\n\n");

    fprintf(file, "Baseline Metrics:\n");
    fprintf(file, "----------------\n");
    fprintf(file, "Execution Time: %.2f ms\n", analysis->baseline.execution_time);
    fprintf(file, "Memory Usage: %zu bytes\n", analysis->baseline.memory_usage);
    fprintf(file, "CPU Usage: %.1f%%\n", analysis->baseline.cpu_usage);
    fprintf(file, "Cache Hit Ratio: %.2f\n", analysis->baseline.cache_hit_ratio);

    fprintf(file, "\nCurrent Metrics:\n");
    fprintf(file, "---------------\n");
    fprintf(file, "Execution Time: %.2f ms\n", analysis->current.execution_time);
    fprintf(file, "Memory Usage: %zu bytes\n", analysis->current.memory_usage);
    fprintf(file, "CPU Usage: %.1f%%\n", analysis->current.cpu_usage);
    fprintf(file, "Cache Hit Ratio: %.2f\n", analysis->current.cache_hit_ratio);

    fprintf(file, "\nOptimization Impact:\n");
    fprintf(file, "-------------------\n");
    fprintf(file, "Speedup Factor: %.2fx\n", analysis->impact.speedup_factor);
    fprintf(file, "Memory Reduction: %.1f%%\n", analysis->impact.memory_reduction);
    fprintf(file, "CPU Efficiency: %.2fx\n", analysis->impact.cpu_efficiency);
    fprintf(file, "Cache Improvement: %.2fx\n", analysis->impact.cache_improvement);
    fprintf(file, "Is Beneficial: %s\n", analysis->impact.is_beneficial ? "Yes" : "No");
    fprintf(file, "Recommendation: %s\n", analysis->impact.recommendation);

    fclose(file);
}

// Get performance trend
double hyperionGetPerformanceTrend(const HyperionPerformanceAnalysis *analysis)
{
    if (!analysis)
        return 0.0;

    // Calculate trend based on execution time
    if (analysis->baseline.execution_time > 0) {
        return analysis->current.execution_time / analysis->baseline.execution_time;
    }
    return 0.0;
}

// Reset performance analysis
void hyperionResetPerformanceAnalysis(HyperionPerformanceAnalysis *analysis)
{
    if (!analysis)
        return;

    // Reset metrics
    memset(&analysis->baseline, 0, sizeof(HyperionPerformanceMetrics));
    memset(&analysis->current, 0, sizeof(HyperionPerformanceMetrics));
    memset(&analysis->impact, 0, sizeof(HyperionOptimizationImpact));

    // Reset timestamps
    analysis->start_time       = get_timestamp_ms();
    analysis->last_sample_time = analysis->start_time;
}

// Enable/disable performance analysis
void hyperionEnablePerformanceAnalysis(HyperionPerformanceAnalysis *analysis, bool enable)
{
    if (!analysis)
        return;
    analysis->config.track_execution_time  = enable;
    analysis->config.track_memory_usage    = enable;
    analysis->config.track_cpu_usage       = enable;
    analysis->config.track_cache_usage     = enable;
    analysis->config.analyze_optimizations = enable;
}

// Set performance analysis configuration
void hyperionSetPerformanceConfig(HyperionPerformanceAnalysis     *analysis,
                                const HyperionPerformanceConfig *config)
{
    if (!analysis || !config)
        return;
    analysis->config = *config;
}
