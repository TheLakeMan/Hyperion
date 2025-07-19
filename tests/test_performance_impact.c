#include "performance_impact.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test basic performance tracking
static void test_basic_tracking()
{
    HyperionPerformanceConfig config = {.track_execution_time  = true,
                                      .track_memory_usage    = true,
                                      .track_cpu_usage       = true,
                                      .track_cache_usage     = true,
                                      .analyze_optimizations = true,
                                      .sample_interval_ms    = 100,
                                      .analysis_window_ms    = 1000};

    HyperionPerformanceAnalysis *analysis = hyperionCreatePerformanceAnalysis(&config);
    assert(analysis != NULL);

    // Record baseline metrics
    HyperionPerformanceMetrics baseline = {.execution_time  = 100.0,
                                         .memory_usage    = 1024 * 1024,
                                         .cpu_usage       = 50.0,
                                         .cache_misses    = 1000,
                                         .cache_hits      = 9000,
                                         .cache_hit_ratio = 0.9};
    hyperionRecordMetrics(analysis, &baseline);
    analysis->baseline = baseline;

    // Record improved metrics
    HyperionPerformanceMetrics improved = {.execution_time  = 50.0,
                                         .memory_usage    = 512 * 1024,
                                         .cpu_usage       = 25.0,
                                         .cache_misses    = 500,
                                         .cache_hits      = 9500,
                                         .cache_hit_ratio = 0.95};
    hyperionRecordMetrics(analysis, &improved);

    // Analyze impact
    hyperionAnalyzeOptimizationImpact(analysis);

    // Check impact
    HyperionOptimizationImpact impact = hyperionGetOptimizationImpact(analysis);
    assert(impact.speedup_factor == 2.0);    // 100ms -> 50ms
    assert(impact.memory_reduction == 50.0); // 1MB -> 512KB
    assert(impact.cpu_efficiency == 2.0);    // 50% -> 25%
    assert(impact.cache_improvement > 1.0);  // 0.9 -> 0.95
    assert(impact.is_beneficial);

    // Cleanup
    hyperionFreePerformanceAnalysis(analysis);
}

// Test performance sampling
static void test_performance_sampling()
{
    HyperionPerformanceAnalysis *analysis = hyperionCreatePerformanceAnalysis(NULL);
    assert(analysis != NULL);

    // Take multiple samples
    for (int i = 0; i < 10; i++) {
        hyperionTakePerformanceSample(analysis);
    }

    // Get current metrics
    HyperionPerformanceMetrics metrics = hyperionGetPerformanceMetrics(analysis);
    assert(metrics.execution_time >= 0.0);
    assert(metrics.memory_usage >= 0);
    assert(metrics.cpu_usage >= 0.0);
    assert(metrics.cache_hits >= 0);
    assert(metrics.cache_misses >= 0);

    // Cleanup
    hyperionFreePerformanceAnalysis(analysis);
}

// Test performance reporting
static void test_performance_reporting()
{
    HyperionPerformanceAnalysis *analysis = hyperionCreatePerformanceAnalysis(NULL);
    assert(analysis != NULL);

    // Set baseline metrics
    analysis->baseline.execution_time  = 100.0;
    analysis->baseline.memory_usage    = 1024 * 1024;
    analysis->baseline.cpu_usage       = 50.0;
    analysis->baseline.cache_hit_ratio = 0.9;

    // Set current metrics
    analysis->current.execution_time  = 50.0;
    analysis->current.memory_usage    = 512 * 1024;
    analysis->current.cpu_usage       = 25.0;
    analysis->current.cache_hit_ratio = 0.95;

    // Generate report
    hyperionGeneratePerformanceReport(analysis, "performance_report.txt");

    // Cleanup
    hyperionFreePerformanceAnalysis(analysis);
}

// Test configuration changes
static void test_configuration()
{
    HyperionPerformanceAnalysis *analysis = hyperionCreatePerformanceAnalysis(NULL);
    assert(analysis != NULL);

    // Test enabling/disabling
    hyperionEnablePerformanceAnalysis(analysis, false);
    HyperionPerformanceMetrics metrics = {.execution_time  = 100.0,
                                        .memory_usage    = 1024 * 1024,
                                        .cpu_usage       = 50.0,
                                        .cache_misses    = 1000,
                                        .cache_hits      = 9000,
                                        .cache_hit_ratio = 0.9};
    hyperionRecordMetrics(analysis, &metrics);
    assert(hyperionGetPerformanceMetrics(analysis).execution_time == 0.0);

    // Test configuration changes
    HyperionPerformanceConfig new_config = {.track_execution_time  = true,
                                          .track_memory_usage    = true,
                                          .track_cpu_usage       = true,
                                          .track_cache_usage     = true,
                                          .analyze_optimizations = true,
                                          .sample_interval_ms    = 50,
                                          .analysis_window_ms    = 500};
    hyperionSetPerformanceConfig(analysis, &new_config);
    hyperionEnablePerformanceAnalysis(analysis, true);
    hyperionRecordMetrics(analysis, &metrics);
    assert(hyperionGetPerformanceMetrics(analysis).execution_time == 100.0);

    // Cleanup
    hyperionFreePerformanceAnalysis(analysis);
}

// Test performance trend
static void test_performance_trend()
{
    HyperionPerformanceAnalysis *analysis = hyperionCreatePerformanceAnalysis(NULL);
    assert(analysis != NULL);

    // Set baseline and current metrics
    analysis->baseline.execution_time = 100.0;
    analysis->current.execution_time  = 50.0;

    // Check trend
    double trend = hyperionGetPerformanceTrend(analysis);
    assert(trend == 0.5); // 50ms / 100ms

    // Cleanup
    hyperionFreePerformanceAnalysis(analysis);
}

int main()
{
    printf("Testing performance impact assessment...\n");

    test_basic_tracking();
    printf("Basic tracking test passed\n");

    test_performance_sampling();
    printf("Performance sampling test passed\n");

    test_performance_reporting();
    printf("Performance reporting test passed\n");

    test_configuration();
    printf("Configuration test passed\n");

    test_performance_trend();
    printf("Performance trend test passed\n");

    printf("All performance impact assessment tests passed successfully!\n");
    return 0;
}