#ifndef HYPERION_ROS2_CONTROL_BRIDGE_H
#define HYPERION_ROS2_CONTROL_BRIDGE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define HYPERION_ROS2_CONTROL_OK                 0
#define HYPERION_ROS2_CONTROL_ERROR_INIT        -3000
#define HYPERION_ROS2_CONTROL_ERROR_SPIN        -3001
#define HYPERION_ROS2_CONTROL_ERROR_UNAVAILABLE -3002

#define HYPERION_ROS2_CONTROL_DEFAULT_TIMEOUT_NS 500000000ull

typedef struct {
    float linear_x;
    float linear_y;
    float linear_z;
    float angular_z;
    bool  active;
} HyperionRos2ControlCommand;

#ifdef HYPERION_HAVE_ROS2
#include <rcl/rcl.h>
#include <rcl/timer.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/string.h>
#include <rosidl_runtime_c/string_functions.h>
#endif

typedef struct HyperionRos2ControlBridge {
#ifdef HYPERION_HAVE_ROS2
    rclc_support_t support;
    rcl_allocator_t allocator;
    rcl_node_t node;
    rcl_subscription_t command_sub;
    rcl_publisher_t velocity_pub;
    rclc_executor_t executor;
    rcl_timer_t watchdog_timer;
    std_msgs__msg__String command_msg;
    geometry_msgs__msg__Twist velocity_msg;
    uint64_t watchdog_timeout_ns;
#endif
    HyperionRos2ControlCommand last_command;
} HyperionRos2ControlBridge;

#ifdef __cplusplus
extern "C" {
#endif

int hyperion_ros2_control_bridge_init(HyperionRos2ControlBridge *bridge,
                                      const char *node_name,
                                      const char *command_topic,
                                      const char *velocity_topic);

void hyperion_ros2_control_bridge_fini(HyperionRos2ControlBridge *bridge);

int hyperion_ros2_control_bridge_spin_some(HyperionRos2ControlBridge *bridge,
                                           uint64_t timeout_ns);

void hyperion_ros2_control_bridge_set_watchdog_timeout(HyperionRos2ControlBridge *bridge,
                                                       uint64_t timeout_ns);

const HyperionRos2ControlCommand *hyperion_ros2_control_bridge_last_command(
    const HyperionRos2ControlBridge *bridge);

#ifdef __cplusplus
}
#endif

#endif
