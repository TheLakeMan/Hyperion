#include "test_framework.h"
#include "../utils/performance_monitor.h"
#include "../core/memory.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int g_slow_callback_invocations = 0;
static int g_slow_callback_errors      = 0;

static void perfSlowCallback(const HyperionPerfSample *sample, void *user_data)
{
    double *threshold = (double *)user_data;
    if (!sample || sample->duration_ms < *threshold) {
        g_slow_callback_errors++;
    } else {
        g_slow_callback_invocations++;
    }
}

HYPERION_TEST(test_performance_monitor_statistics)
{
    g_slow_callback_invocations = 0;
    g_slow_callback_errors      = 0;

    HyperionPerformanceMonitor *monitor = hyperionPerfCreate(128, true);
    HYPERION_ASSERT(monitor != NULL, "Monitor creation should succeed");

    double threshold = 8.0;
    hyperionPerfSetSlowCallback(monitor, HYPERION_PERF_TEXT_GENERATION, threshold, perfSlowCallback, &threshold);

    hyperionPerfRecord(monitor, HYPERION_PERF_TEXT_GENERATION, "run_fast", 5.0, 1024, 0, "fast", 2.0);
    hyperionPerfRecord(monitor, HYPERION_PERF_TEXT_GENERATION, "run_medium", 7.0, 2048, 0, "medium", 3.5);
    hyperionPerfRecord(monitor, HYPERION_PERF_TEXT_GENERATION, "run_slow", 12.0, 4096, 0, "slow", 6.0);

    HyperionPerfStats stats = {0};
    HYPERION_ASSERT(hyperionPerfGetStats(monitor, HYPERION_PERF_TEXT_GENERATION, &stats),
                    "Statistics should be available");

    HYPERION_ASSERT(stats.total_operations == 3, "Expected three operations recorded");
    HYPERION_ASSERT(stats.slow_operation_count == 1, "One operation should exceed threshold");
    HYPERION_ASSERT(fabs(stats.percentile_50_ms - 7.0) < 1e-3, "Median should be 7ms");
    HYPERION_ASSERT(fabs(stats.percentile_90_ms - 11.0) < 1e-3, "P90 should be 11ms");
    HYPERION_ASSERT(stats.cpu_time_total_ms > 0.0, "CPU stats should accumulate");
    HYPERION_ASSERT(stats.cpu_utilization_percent > 0.0, "CPU utilization should be positive");
    HYPERION_ASSERT(g_slow_callback_invocations == 1, "Slow callback should trigger once");
    HYPERION_ASSERT(g_slow_callback_errors == 0, "Slow callback should not report errors");

    hyperionPerfDestroy(monitor);
    return 0;
}

HYPERION_TEST(test_performance_monitor_timeline)
{
    HyperionPerformanceMonitor *monitor = hyperionPerfCreate(32, true);
    HYPERION_ASSERT(monitor != NULL, "Monitor creation should succeed");

    for (int i = 0; i < 5; ++i) {
        char info[32];
        snprintf(info, sizeof(info), "iteration=%d", i);
        hyperionPerfRecord(monitor, HYPERION_PERF_TOKENIZATION, "token_step",
                           4.0 + i, 512 + (size_t)i * 128, 0, info, 1.0 + i * 0.2);
    }

    char timeline_path[64];
    snprintf(timeline_path, sizeof(timeline_path), "timeline_test.json");
    HYPERION_ASSERT(hyperionPerfExportTimeline(monitor, timeline_path, HYPERION_PERF_CUSTOM, 5),
                    "Timeline export should succeed");

    FILE *fp = fopen(timeline_path, "r");
    HYPERION_ASSERT(fp != NULL, "Timeline file must exist");

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        hyperionPerfDestroy(monitor);
        HYPERION_ASSERT(false, "Failed to seek timeline file");
        return 1;
    }

    long length = ftell(fp);
    HYPERION_ASSERT(length >= 0, "Timeline file length should be non-negative");
    rewind(fp);

    size_t buffer_size = (size_t)length + 1;
    char *buffer = (char *)malloc(buffer_size);
    HYPERION_ASSERT(buffer != NULL, "Timeline buffer allocation should succeed");

    size_t bytes = fread(buffer, 1, buffer_size - 1, fp);
    buffer[bytes] = '\0';
    fclose(fp);
    remove(timeline_path);

    HYPERION_ASSERT(strstr(buffer, "token_step") != NULL, "Timeline should include operation name");
    HYPERION_ASSERT(strstr(buffer, "tokenization") != NULL, "Timeline should include type key");
    HYPERION_ASSERT(strstr(buffer, "iteration=4") != NULL, "Timeline should include metadata");

    free(buffer);

    HyperionPerfSample samples[3];
    size_t copied = hyperionPerfGetLatestSamples(monitor, samples, 3);
    HYPERION_ASSERT(copied == 3, "Should retrieve requested number of samples");
    HYPERION_ASSERT(samples[0].type == HYPERION_PERF_TOKENIZATION, "Sample type should be preserved");

    hyperionPerfDestroy(monitor);
    return 0;
}

const HyperionTestCase g_performance_monitor_tests[] = {
    {"performance_monitor_statistics", "profiling", test_performance_monitor_statistics},
    {"performance_monitor_timeline", "profiling", test_performance_monitor_timeline},
};

const size_t g_performance_monitor_test_count = sizeof(g_performance_monitor_tests) / sizeof(g_performance_monitor_tests[0]);
