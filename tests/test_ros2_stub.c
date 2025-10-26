#include "test_framework.h"
#include "../ros2/text_to_action_node.h"
#include "../ros2/perception_bridge.h"
#include "../ros2/control_bridge.h"

#include <string.h>

#ifndef HYPERION_HAVE_ROS2

HYPERION_TEST(test_ros2_stub_reports_unavailable)
{
    HyperionRos2TextToActionNode node;
    memset(&node, 0, sizeof(node));

    HyperionModel *dummyModel = (HyperionModel *)0x1;
    HyperionTokenizer *dummyTokenizer = (HyperionTokenizer *)0x1;

    int rc = hyperion_ros2_text_to_action_node_init(&node, "hyperion_ros2_stub",
                                                    "/hyperion/command",
                                                    "/hyperion/action",
                                                    dummyModel, dummyTokenizer);

    HYPERION_ASSERT(rc == HYPERION_ROS2_ERROR_UNAVAILABLE,
                    "Stub should report ROS2 integration as unavailable");

    hyperion_ros2_text_to_action_node_use_hybrid(&node, (HyperionMcpClient *)0x1);
    const HyperionRos2TextToActionTelemetry *telemetry =
        hyperion_ros2_text_to_action_node_telemetry(&node);
    HYPERION_ASSERT(telemetry == NULL, "Stub telemetry should be unavailable");

    return 0;
}

HYPERION_TEST(test_ros2_perception_stub_reports_unavailable)
{
    HyperionRos2PerceptionBridge bridge;
    memset(&bridge, 0, sizeof(bridge));

    int rc = hyperion_ros2_perception_bridge_init(&bridge,
                                                  "hyperion_perception_stub",
                                                  "/hyperion/detections",
                                                  "/hyperion/command");

    HYPERION_ASSERT(rc == HYPERION_ROS2_BRIDGE_ERROR_UNAVAILABLE,
                    "Perception bridge stub should report ROS2 unavailable");

    return 0;
}

HYPERION_TEST(test_ros2_control_stub_reports_unavailable)
{
    HyperionRos2ControlBridge bridge;
    memset(&bridge, 0, sizeof(bridge));

    int rc = hyperion_ros2_control_bridge_init(&bridge,
                                               "hyperion_control_stub",
                                               "/hyperion/action",
                                               "/cmd_vel");

    HYPERION_ASSERT(rc == HYPERION_ROS2_CONTROL_ERROR_UNAVAILABLE,
                    "Control bridge stub should report ROS2 unavailable");

    return 0;
}

const HyperionTestCase g_ros2_tests[] = {
    {"ros2_stub_reports_unavailable", "ros2", test_ros2_stub_reports_unavailable},
    {"ros2_perception_stub_reports_unavailable", "ros2", test_ros2_perception_stub_reports_unavailable},
    {"ros2_control_stub_reports_unavailable", "ros2", test_ros2_control_stub_reports_unavailable},
};

const size_t g_ros2_test_count = sizeof(g_ros2_tests) / sizeof(g_ros2_tests[0]);

#else

const HyperionTestCase g_ros2_tests[] = {};
const size_t g_ros2_test_count       = 0;

#endif
