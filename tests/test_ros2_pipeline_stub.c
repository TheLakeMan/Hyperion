#include "test_framework.h"
#include "../ros2/autonomy_pipeline.h"

#include <string.h>

#ifndef HYPERION_HAVE_ROS2

HYPERION_TEST(test_ros2_pipeline_stub_reports_unavailable)
{
    HyperionRos2AutonomyPipeline *pipeline = hyperion_ros2_autonomy_create();
    HYPERION_ASSERT(pipeline == NULL, "Autonomy pipeline stub create should return NULL");

    HyperionRos2AutonomyConfig cfg;
    HyperionModel *model = (HyperionModel *)0x1;
    HyperionTokenizer *tokenizer = (HyperionTokenizer *)0x1;

    memset(&cfg, 0, sizeof(cfg));

    int rc = hyperion_ros2_autonomy_init(pipeline, &cfg, model, tokenizer);

    HYPERION_ASSERT(rc == HYPERION_ROS2_ERROR_UNAVAILABLE,
                    "Autonomy pipeline stub should report ROS2 unavailable");

    const HyperionRos2AutonomyTelemetry *telemetry =
        hyperion_ros2_autonomy_telemetry(pipeline);
    HYPERION_ASSERT(telemetry == NULL, "Autonomy telemetry stub should be unavailable");

    hyperion_ros2_autonomy_destroy(pipeline);

    return 0;
}

const HyperionTestCase g_ros2_pipeline_tests[] = {
    {"ros2_pipeline_stub_reports_unavailable", "ros2", test_ros2_pipeline_stub_reports_unavailable},
};

const size_t g_ros2_pipeline_test_count = sizeof(g_ros2_pipeline_tests) / sizeof(g_ros2_pipeline_tests[0]);

#else

const HyperionTestCase g_ros2_pipeline_tests[] = {};
const size_t g_ros2_pipeline_test_count       = 0;

#endif
