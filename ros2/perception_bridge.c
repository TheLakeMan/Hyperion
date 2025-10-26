#include "perception_bridge.h"

#include <stdio.h>
#include <string.h>

#ifdef HYPERION_HAVE_ROS2

static void hyperion_ros2_default_formatter(const geometry_msgs__msg__PointStamped *target,
                                            char *buffer,
                                            size_t buffer_size)
{
    if (!buffer || buffer_size == 0)
        return;

    if (!target) {
        buffer[0] = '\0';
        return;
    }

    snprintf(buffer, buffer_size, "navigate to %.2f %.2f %.2f",
             target->point.x, target->point.y, target->point.z);
}

static void hyperion_ros2_publish_command(HyperionRos2PerceptionBridge *bridge, const char *command)
{
    if (!command)
        return;

    std_msgs__msg__String msg;
    std_msgs__msg__String__init(&msg);
    if (!rosidl_runtime_c__String__assign(&msg.data, command)) {
        std_msgs__msg__String__fini(&msg);
        return;
    }

    (void)rcl_publish(&bridge->command_pub, &msg, NULL);
    std_msgs__msg__String__fini(&msg);
}

static void hyperion_ros2_handle_target(const void *msgin, void *context)
{
    HyperionRos2PerceptionBridge *bridge = (HyperionRos2PerceptionBridge *)context;
    const geometry_msgs__msg__PointStamped *target =
        (const geometry_msgs__msg__PointStamped *)msgin;

    if (!bridge || !target)
        return;

    HyperionRos2BridgeFormatter formatter = bridge->formatter;
    if (!formatter)
        formatter = hyperion_ros2_default_formatter;

    formatter(target, bridge->last_command, sizeof(bridge->last_command));
    if (bridge->last_command[0] == '\0')
        return;

    hyperion_ros2_publish_command(bridge, bridge->last_command);
}

int hyperion_ros2_perception_bridge_init(HyperionRos2PerceptionBridge *bridge,
                                         const char *node_name,
                                         const char *target_topic,
                                         const char *command_topic)
{
    if (!bridge || !node_name || !target_topic || !command_topic)
        return HYPERION_ROS2_BRIDGE_ERROR_INIT;

    memset(bridge, 0, sizeof(*bridge));
    bridge->formatter = hyperion_ros2_default_formatter;

    bridge->allocator = rcl_get_default_allocator();
    rcl_ret_t ret = rclc_support_init(&bridge->support, 0, NULL, &bridge->allocator);
    if (ret != RCL_RET_OK)
        return HYPERION_ROS2_BRIDGE_ERROR_INIT;

    ret = rclc_node_init_default(&bridge->node, node_name, "", &bridge->support);
    if (ret != RCL_RET_OK)
        goto fail_support;

    ret = rclc_subscription_init_default(
        &bridge->target_sub,
        &bridge->node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, PointStamped),
        target_topic);
    if (ret != RCL_RET_OK)
        goto fail_node;

    ret = rclc_publisher_init_default(&bridge->command_pub,
                                      &bridge->node,
                                      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
                                      command_topic);
    if (ret != RCL_RET_OK)
        goto fail_subscription;

    geometry_msgs__msg__PointStamped__init(&bridge->target_msg);

    ret = rclc_executor_init(&bridge->executor, &bridge->support.context, 1, &bridge->allocator);
    if (ret != RCL_RET_OK)
        goto fail_publisher;

    ret = rclc_executor_add_subscription(&bridge->executor,
                                         &bridge->target_sub,
                                         &bridge->target_msg,
                                         hyperion_ros2_handle_target,
                                         bridge,
                                         ON_NEW_DATA);
    if (ret != RCL_RET_OK)
        goto fail_executor;

    bridge->last_command[0] = '\0';
    return HYPERION_ROS2_BRIDGE_OK;

fail_executor:
    rclc_executor_fini(&bridge->executor);
fail_publisher:
    geometry_msgs__msg__PointStamped__fini(&bridge->target_msg);
    rcl_publisher_fini(&bridge->command_pub, &bridge->node);
fail_subscription:
    rcl_subscription_fini(&bridge->target_sub, &bridge->node);
fail_node:
    rcl_node_fini(&bridge->node);
fail_support:
    rclc_support_fini(&bridge->support);
    return HYPERION_ROS2_BRIDGE_ERROR_INIT;
}

void hyperion_ros2_perception_bridge_fini(HyperionRos2PerceptionBridge *bridge)
{
    if (!bridge)
        return;

    rclc_executor_fini(&bridge->executor);
    geometry_msgs__msg__PointStamped__fini(&bridge->target_msg);
    rcl_publisher_fini(&bridge->command_pub, &bridge->node);
    rcl_subscription_fini(&bridge->target_sub, &bridge->node);
    rcl_node_fini(&bridge->node);
    rclc_support_fini(&bridge->support);
    bridge->formatter = NULL;
    bridge->last_command[0] = '\0';
}

int hyperion_ros2_perception_bridge_spin_some(HyperionRos2PerceptionBridge *bridge,
                                              uint64_t timeout_ns)
{
    if (!bridge)
        return HYPERION_ROS2_BRIDGE_ERROR_UNAVAILABLE;

    rcl_ret_t ret = rclc_executor_spin_some(&bridge->executor, timeout_ns);
    if (ret == RCL_RET_OK || ret == RCL_RET_TIMEOUT)
        return HYPERION_ROS2_BRIDGE_OK;
    return HYPERION_ROS2_BRIDGE_ERROR_SPIN;
}

void hyperion_ros2_perception_bridge_set_formatter(HyperionRos2PerceptionBridge *bridge,
                                                   HyperionRos2BridgeFormatter formatter)
{
    if (!bridge)
        return;
    bridge->formatter = formatter ? formatter : hyperion_ros2_default_formatter;
}

const char *hyperion_ros2_perception_bridge_last_command(const HyperionRos2PerceptionBridge *bridge)
{
    if (!bridge)
        return NULL;
    return bridge->last_command;
}

#else

int hyperion_ros2_perception_bridge_init(HyperionRos2PerceptionBridge *bridge,
                                         const char *node_name,
                                         const char *target_topic,
                                         const char *command_topic)
{
    (void)bridge;
    (void)node_name;
    (void)target_topic;
    (void)command_topic;
    return HYPERION_ROS2_BRIDGE_ERROR_UNAVAILABLE;
}

void hyperion_ros2_perception_bridge_fini(HyperionRos2PerceptionBridge *bridge)
{
    (void)bridge;
}

int hyperion_ros2_perception_bridge_spin_some(HyperionRos2PerceptionBridge *bridge,
                                              uint64_t timeout_ns)
{
    (void)bridge;
    (void)timeout_ns;
    return HYPERION_ROS2_BRIDGE_ERROR_UNAVAILABLE;
}

void hyperion_ros2_perception_bridge_set_formatter(HyperionRos2PerceptionBridge *bridge,
                                                   HyperionRos2BridgeFormatter formatter)
{
    (void)bridge;
    (void)formatter;
}

const char *hyperion_ros2_perception_bridge_last_command(const HyperionRos2PerceptionBridge *bridge)
{
    (void)bridge;
    return NULL;
}

#endif
