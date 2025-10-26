/**
 * Hyperion Test Runner
 */

#include "test_framework.h"
#include <stdio.h>
#include <string.h>

extern const HyperionTestCase g_model_format_tests[];
extern const size_t g_model_format_test_count;
extern const HyperionTestCase g_memory_tests[];
extern const size_t g_memory_test_count;
extern const HyperionTestCase g_io_tests[];
extern const size_t g_io_test_count;
extern const HyperionTestCase g_sampling_tests[];
extern const size_t g_sampling_test_count;
extern const HyperionTestCase g_ros2_tests[];
extern const size_t g_ros2_test_count;
extern const HyperionTestCase g_ros2_pipeline_tests[];
extern const size_t g_ros2_pipeline_test_count;
extern const HyperionTestCase g_ros2_monitor_tests[];
extern const size_t g_ros2_monitor_test_count;
extern const HyperionTestCase g_performance_monitor_tests[];
extern const size_t g_performance_monitor_test_count;
extern const HyperionTestCase g_deployment_tests[];
extern const size_t g_deployment_test_count;
extern const HyperionTestCase g_monitoring_tests[];
extern const size_t g_monitoring_test_count;
extern const HyperionTestCase g_autoscaler_tests[];
extern const size_t g_autoscaler_test_count;

static size_t appendTests(HyperionTestCase *dest, size_t destCapacity, size_t destCount,
                          const HyperionTestCase *src, size_t srcCount)
{
    if (destCount + srcCount > destCapacity) {
        fprintf(stderr, "Test registry overflow.\n");
        return destCount;
    }

    for (size_t i = 0; i < srcCount; ++i) {
        dest[destCount + i] = src[i];
    }

    return destCount + srcCount;
}

int main(int argc, char **argv)
{
    const char *filter = NULL;
    if (argc > 1) {
        filter = argv[1];
    }

    HyperionTestCase tests[128];
    size_t           testCount = 0;

    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_model_format_tests, g_model_format_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_memory_tests, g_memory_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_io_tests, g_io_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_sampling_tests, g_sampling_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_ros2_tests, g_ros2_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_ros2_pipeline_tests, g_ros2_pipeline_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_ros2_monitor_tests, g_ros2_monitor_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_performance_monitor_tests, g_performance_monitor_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_deployment_tests, g_deployment_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_monitoring_tests, g_monitoring_test_count);
    testCount = appendTests(tests, sizeof(tests) / sizeof(tests[0]), testCount,
                            g_autoscaler_tests, g_autoscaler_test_count);

    return hyperionRunTests(tests, testCount, filter);
}
