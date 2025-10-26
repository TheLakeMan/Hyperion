#include "control_bridge.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HYPERION_HAVE_ROS2

static HyperionRos2ControlBridge *g_hyperion_ros2_control_active = NULL;

static void hyperion_ros2_control_reset_command(HyperionRos2ControlBridge *bridge)
{
    bridge->last_command.linear_x = 0.0f;
    bridge->last_command.linear_y = 0.0f;
    bridge->last_command.linear_z = 0.0f;
    bridge->last_command.angular_z = 0.0f;
    bridge->last_command.active = false;
}

static void hyperion_ros2_publish_twist(HyperionRos2ControlBridge *bridge)
{
    bridge->velocity_msg.linear.x = bridge->last_command.linear_x;
    bridge->velocity_msg.linear.y = bridge->last_command.linear_y;
    bridge->velocity_msg.linear.z = bridge->last_command.linear_z;
    bridge->velocity_msg.angular.z = bridge->last_command.angular_z;
    (void)rcl_publish(&bridge->velocity_pub, &bridge->velocity_msg, NULL);
}

static float hyperion_ros2_parse_float(const char **cursor)
{
    while (**cursor && isspace((unsigned char)**cursor))
        (*cursor)++;
    char *end_ptr = NULL;
    float value = strtof(*cursor, &end_ptr);
    if (end_ptr == *cursor)
        return NAN;
    *cursor = end_ptr;
    return value;
}

static bool hyperion_ros2_parse_move_command(const char *text, HyperionRos2ControlBridge *bridge)
{
    const char *cursor = text;
    cursor += strlen("move");
    float vx = hyperion_ros2_parse_float(&cursor);
    float vy = hyperion_ros2_parse_float(&cursor);
    float vz = hyperion_ros2_parse_float(&cursor);

    if (isnan(vx) || isnan(vy) || isnan(vz))
        return false;

    bridge->last_command.linear_x = vx;
    bridge->last_command.linear_y = vy;
    bridge->last_command.linear_z = vz;
    bridge->last_command.angular_z = 0.0f;
    bridge->last_command.active = true;
    return true;
}

static bool hyperion_ros2_parse_rotate_command(const char *text, HyperionRos2ControlBridge *bridge)
{
    const char *cursor = text;
    cursor += strlen("rotate");
    float yaw_rate = hyperion_ros2_parse_float(&cursor);
    if (isnan(yaw_rate))
        return false;

    hyperion_ros2_control_reset_command(bridge);
    bridge->last_command.angular_z = yaw_rate;
    bridge->last_command.active = true;
    return true;
}

static bool hyperion_ros2_parse_navigate_command(const char *text, HyperionRos2ControlBridge *bridge)
{
    const char *cursor = text;
    cursor += strlen("navigate to");
    float x = hyperion_ros2_parse_float(&cursor);
    float y = hyperion_ros2_parse_float(&cursor);
    float z = hyperion_ros2_parse_float(&cursor);

    if (isnan(x) || isnan(y) || isnan(z))
        return false;

    float length = sqrtf(x * x + y * y + z * z);
    if (length < 1e-3f)
        return false;

    const float speed = 0.5f;
    bridge->last_command.linear_x = speed * (x / length);
    bridge->last_command.linear_y = speed * (y / length);
    bridge->last_command.linear_z = speed * (z / length);
    bridge->last_command.angular_z = 0.0f;
    bridge->last_command.active = true;
    return true;
}

static bool hyperion_ros2_parse_command(const char *text, HyperionRos2ControlBridge *bridge)
{
    if (!text || !bridge)
        return false;

    while (*text && isspace((unsigned char)*text))
        ++text;

    if (strncmp(text, "stop", 4) == 0) {
        hyperion_ros2_control_reset_command(bridge);
        return true;
    }

    if (strncmp(text, "move", 4) == 0)
        return hyperion_ros2_parse_move_command(text, bridge);

    if (strncmp(text, "rotate", 6) == 0)
        return hyperion_ros2_parse_rotate_command(text, bridge);

    if (strncmp(text, "navigate to", 11) == 0)
        return hyperion_ros2_parse_navigate_command(text, bridge);

    return false;
}

static void hyperion_ros2_command_callback(const void *msgin, void *context)
{
    HyperionRos2ControlBridge *bridge = (HyperionRos2ControlBridge *)context;
    const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;

    if (!bridge || !msg)
        return;

    const char *text = msg->data.data ? msg->data.data : "";
    if (!hyperion_ros2_parse_command(text, bridge))
        return;

    rcl_timer_reset(&bridge->watchdog_timer);
    hyperion_ros2_publish_twist(bridge);
}

static void hyperion_ros2_watchdog_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    (void)last_call_time;
    (void)timer;

    HyperionRos2ControlBridge *bridge = g_hyperion_ros2_control_active;
    if (!bridge)
        return;

    if (!bridge->last_command.active)
        return;

    hyperion_ros2_control_reset_command(bridge);
    hyperion_ros2_publish_twist(bridge);
}

int hyperion_ros2_control_bridge_init(HyperionRos2ControlBridge *bridge,
                                      const char *node_name,
                                      const char *command_topic,
                                      const char *velocity_topic)
{
    if (!bridge || !node_name || !command_topic || !velocity_topic)
        return HYPERION_ROS2_CONTROL_ERROR_INIT;

    memset(bridge, 0, sizeof(*bridge));
    hyperion_ros2_control_reset_command(bridge);

    bridge->allocator = rcl_get_default_allocator();
    bridge->watchdog_timeout_ns = HYPERION_ROS2_CONTROL_DEFAULT_TIMEOUT_NS;

    rcl_ret_t ret = rclc_support_init(&bridge->support, 0, NULL, &bridge->allocator);
    if (ret != RCL_RET_OK)
        return HYPERION_ROS2_CONTROL_ERROR_INIT;

    ret = rclc_node_init_default(&bridge->node, node_name, "", &bridge->support);
    if (ret != RCL_RET_OK)
        goto fail_support;

    ret = rclc_subscription_init_default(&bridge->command_sub,
                                         &bridge->node,
                                         ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
                                         command_topic);
    if (ret != RCL_RET_OK)
        goto fail_node;

    ret = rclc_publisher_init_default(&bridge->velocity_pub,
                                      &bridge->node,
                                      ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
                                      velocity_topic);
    if (ret != RCL_RET_OK)
        goto fail_subscription;

    std_msgs__msg__String__init(&bridge->command_msg);
    geometry_msgs__msg__Twist__init(&bridge->velocity_msg);

    ret = rclc_executor_init(&bridge->executor, &bridge->support.context, 2, &bridge->allocator);
    if (ret != RCL_RET_OK)
        goto fail_publisher;

    ret = rclc_executor_add_subscription(&bridge->executor,
                                         &bridge->command_sub,
                                         &bridge->command_msg,
                                         hyperion_ros2_command_callback,
                                         bridge,
                                         ON_NEW_DATA);
    if (ret != RCL_RET_OK)
        goto fail_executor;

    ret = rclc_timer_init_default(&bridge->watchdog_timer,
                                  &bridge->support,
                                  bridge->watchdog_timeout_ns,
                                  hyperion_ros2_watchdog_callback);
    if (ret != RCL_RET_OK)
        goto fail_executor;

    g_hyperion_ros2_control_active = bridge;

    ret = rclc_executor_add_timer(&bridge->executor, &bridge->watchdog_timer);
    if (ret != RCL_RET_OK)
        goto fail_timer;

    return HYPERION_ROS2_CONTROL_OK;

fail_timer:
    g_hyperion_ros2_control_active = NULL;
    rcl_timer_fini(&bridge->watchdog_timer);
fail_executor:
    rclc_executor_fini(&bridge->executor);
fail_publisher:
    geometry_msgs__msg__Twist__fini(&bridge->velocity_msg);
    std_msgs__msg__String__fini(&bridge->command_msg);
    rcl_publisher_fini(&bridge->velocity_pub, &bridge->node);
fail_subscription:
    rcl_subscription_fini(&bridge->command_sub, &bridge->node);
fail_node:
    rcl_node_fini(&bridge->node);
fail_support:
    rclc_support_fini(&bridge->support);
    return HYPERION_ROS2_CONTROL_ERROR_INIT;
}

void hyperion_ros2_control_bridge_fini(HyperionRos2ControlBridge *bridge)
{
    if (!bridge)
        return;

    if (g_hyperion_ros2_control_active == bridge)
        g_hyperion_ros2_control_active = NULL;

    rclc_executor_fini(&bridge->executor);
    rcl_timer_fini(&bridge->watchdog_timer);
    geometry_msgs__msg__Twist__fini(&bridge->velocity_msg);
    std_msgs__msg__String__fini(&bridge->command_msg);
    rcl_publisher_fini(&bridge->velocity_pub, &bridge->node);
    rcl_subscription_fini(&bridge->command_sub, &bridge->node);
    rcl_node_fini(&bridge->node);
    rclc_support_fini(&bridge->support);

    hyperion_ros2_control_reset_command(bridge);
}

int hyperion_ros2_control_bridge_spin_some(HyperionRos2ControlBridge *bridge,
                                           uint64_t timeout_ns)
{
    if (!bridge)
        return HYPERION_ROS2_CONTROL_ERROR_UNAVAILABLE;

    rcl_ret_t ret = rclc_executor_spin_some(&bridge->executor, timeout_ns);
    if (ret == RCL_RET_OK || ret == RCL_RET_TIMEOUT)
        return HYPERION_ROS2_CONTROL_OK;

    return HYPERION_ROS2_CONTROL_ERROR_SPIN;
}

void hyperion_ros2_control_bridge_set_watchdog_timeout(HyperionRos2ControlBridge *bridge,
                                                       uint64_t timeout_ns)
{
    if (!bridge || timeout_ns == 0)
        return;

    bridge->watchdog_timeout_ns = timeout_ns;
    rcl_timer_set_period(&bridge->watchdog_timer, timeout_ns);
}

const HyperionRos2ControlCommand *hyperion_ros2_control_bridge_last_command(
    const HyperionRos2ControlBridge *bridge)
{
    if (!bridge)
        return NULL;
    return &bridge->last_command;
}

#else

int hyperion_ros2_control_bridge_init(HyperionRos2ControlBridge *bridge,
                                      const char *node_name,
                                      const char *command_topic,
                                      const char *velocity_topic)
{
    (void)bridge;
    (void)node_name;
    (void)command_topic;
    (void)velocity_topic;
    return HYPERION_ROS2_CONTROL_ERROR_UNAVAILABLE;
}

void hyperion_ros2_control_bridge_fini(HyperionRos2ControlBridge *bridge)
{
    (void)bridge;
}

int hyperion_ros2_control_bridge_spin_some(HyperionRos2ControlBridge *bridge,
                                           uint64_t timeout_ns)
{
    (void)bridge;
    (void)timeout_ns;
    return HYPERION_ROS2_CONTROL_ERROR_UNAVAILABLE;
}

void hyperion_ros2_control_bridge_set_watchdog_timeout(HyperionRos2ControlBridge *bridge,
                                                       uint64_t timeout_ns)
{
    (void)bridge;
    (void)timeout_ns;
}

const HyperionRos2ControlCommand *hyperion_ros2_control_bridge_last_command(
    const HyperionRos2ControlBridge *bridge)
{
    (void)bridge;
    return NULL;
}

#endif
