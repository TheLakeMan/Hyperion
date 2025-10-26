#include "test_framework.h"
#include "../ros2/autonomy_monitor.h"

#include <string.h>

#ifndef HYPERION_HAVE_ROS2

HYPERION_TEST(test_ros2_monitor_stub_reports_unavailable)
{
    HyperionRos2AutonomyMonitor *monitor = (HyperionRos2AutonomyMonitor *)0x1;
    HyperionRos2AutonomyConfig cfg;
    HyperionModel *model = (HyperionModel *)0x1;
    HyperionTokenizer *tokenizer = (HyperionTokenizer *)0x1;

    memset(&cfg, 0, sizeof(cfg));

    int rc = hyperion_ros2_autonomy_monitor_init(monitor, &cfg, model, tokenizer);

    HYPERION_ASSERT(rc == HYPERION_ROS2_ERROR_UNAVAILABLE,
                    "Autonomy monitor stub should report ROS2 unavailable");

    const HyperionRos2AutonomyTelemetry *telemetry =
        hyperion_ros2_autonomy_monitor_telemetry(monitor);
    HYPERION_ASSERT(telemetry == NULL, "Monitor telemetry stub should be unavailable");

    const HyperionRos2AutonomyHealth *health =
        hyperion_ros2_autonomy_monitor_health(monitor);
    HYPERION_ASSERT(health == NULL, "Monitor health stub should be unavailable");

    return 0;
}

const HyperionTestCase g_ros2_monitor_tests[] = {
    {"ros2_monitor_stub_reports_unavailable", "ros2", test_ros2_monitor_stub_reports_unavailable},
};

const size_t g_ros2_monitor_test_count = sizeof(g_ros2_monitor_tests) / sizeof(g_ros2_monitor_tests[0]);

#else

const HyperionTestCase g_ros2_monitor_tests[] = {};
const size_t g_ros2_monitor_test_count       = 0;

#endif
