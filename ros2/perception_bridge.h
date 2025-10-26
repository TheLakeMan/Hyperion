#ifndef HYPERION_ROS2_PERCEPTION_BRIDGE_H
#define HYPERION_ROS2_PERCEPTION_BRIDGE_H

#include <stdint.h>
#include <stddef.h>

#ifdef HYPERION_HAVE_ROS2
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/point_stamped.h>
#include <std_msgs/msg/string.h>
#include <rosidl_runtime_c/string_functions.h>
#endif

#define HYPERION_ROS2_BRIDGE_MAX_COMMAND 256

#define HYPERION_ROS2_BRIDGE_OK             0
#define HYPERION_ROS2_BRIDGE_ERROR_INIT    -2000
#define HYPERION_ROS2_BRIDGE_ERROR_SPIN    -2001
#define HYPERION_ROS2_BRIDGE_ERROR_UNAVAILABLE -2002

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HyperionRos2PerceptionBridge HyperionRos2PerceptionBridge;

#ifdef HYPERION_HAVE_ROS2
typedef void (*HyperionRos2BridgeFormatter)(const geometry_msgs__msg__PointStamped *target,
                                            char *buffer,
                                            size_t buffer_size);
#else
typedef void (*HyperionRos2BridgeFormatter)(const void *target, char *buffer, size_t buffer_size);
#endif

struct HyperionRos2PerceptionBridge {
#ifdef HYPERION_HAVE_ROS2
    rclc_support_t support;
    rcl_allocator_t allocator;
    rcl_node_t node;
    rcl_subscription_t target_sub;
    rcl_publisher_t command_pub;
    rclc_executor_t executor;
    geometry_msgs__msg__PointStamped target_msg;
    HyperionRos2BridgeFormatter formatter;
    char last_command[HYPERION_ROS2_BRIDGE_MAX_COMMAND];
#else
    int unused;
#endif
};

int hyperion_ros2_perception_bridge_init(HyperionRos2PerceptionBridge *bridge,
                                         const char *node_name,
                                         const char *target_topic,
                                         const char *command_topic);

void hyperion_ros2_perception_bridge_fini(HyperionRos2PerceptionBridge *bridge);

int hyperion_ros2_perception_bridge_spin_some(HyperionRos2PerceptionBridge *bridge,
                                              uint64_t timeout_ns);

void hyperion_ros2_perception_bridge_set_formatter(HyperionRos2PerceptionBridge *bridge,
                                                   HyperionRos2BridgeFormatter formatter);

const char *hyperion_ros2_perception_bridge_last_command(const HyperionRos2PerceptionBridge *bridge);

#ifdef __cplusplus
}
#endif

#endif
