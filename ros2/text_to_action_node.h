#ifndef HYPERION_ROS2_TEXT_TO_ACTION_NODE_H
#define HYPERION_ROS2_TEXT_TO_ACTION_NODE_H

#include <stdint.h>

#include <stdbool.h>

#include "models/text/generate.h"
#include "models/text/hybrid_generate.h"
#include "models/text/tokenizer.h"
#include "core/mcp/mcp_client.h"

#ifdef HYPERION_HAVE_ROS2
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/string.h>
#include <rosidl_runtime_c/string_functions.h>
#endif

#define HYPERION_ROS2_ERROR_UNAVAILABLE  -1000
#define HYPERION_ROS2_ERROR_INIT         -1001
#define HYPERION_ROS2_ERROR_PUBLISH      -1002
#define HYPERION_ROS2_ERROR_INFERENCE    -1003

#define HYPERION_ROS2_MAX_PROMPT_TOKENS  256
#define HYPERION_ROS2_MAX_OUTPUT_TOKENS  128
#define HYPERION_ROS2_MAX_OUTPUT_CHARS   512

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *command_text;
    const char *action_text;
    bool        remote_used;
    double      local_time_ms;
    double      remote_time_ms;
    double      tokens_per_second;
} HyperionRos2TextToActionTelemetry;

typedef struct HyperionRos2TextToActionNode {
#ifdef HYPERION_HAVE_ROS2
    rclc_support_t support;
    rcl_allocator_t allocator;
    rcl_node_t node;
    rcl_subscription_t command_sub;
    rcl_publisher_t action_pub;
    rclc_executor_t executor;
    std_msgs__msg__String command_msg;
    HyperionModel *model;
    HyperionTokenizer *tokenizer;
    HyperionHybridGenerate *hybrid;
    HyperionMcpClient *mcp_client;
    HyperionGenerationParams params;
    HyperionRos2TextToActionTelemetry telemetry;
    char last_action[HYPERION_ROS2_MAX_OUTPUT_CHARS];
    char last_command[HYPERION_ROS2_MAX_OUTPUT_CHARS];
#else
    int unused;
#endif
} HyperionRos2TextToActionNode;

int hyperion_ros2_text_to_action_node_init(HyperionRos2TextToActionNode *node,
                                           const char *node_name,
                                           const char *command_topic,
                                           const char *action_topic,
                                           HyperionModel *model,
                                           HyperionTokenizer *tokenizer);

void hyperion_ros2_text_to_action_node_fini(HyperionRos2TextToActionNode *node);

int hyperion_ros2_text_to_action_node_spin_some(HyperionRos2TextToActionNode *node,
                                                uint64_t timeout_ns);

void hyperion_ros2_text_to_action_node_set_params(HyperionRos2TextToActionNode *node,
                                                  const HyperionGenerationParams *params);

const char *hyperion_ros2_text_to_action_node_last_action(const HyperionRos2TextToActionNode *node);

void hyperion_ros2_text_to_action_node_use_hybrid(HyperionRos2TextToActionNode *node,
                                                  HyperionMcpClient *mcp_client);

const HyperionRos2TextToActionTelemetry *
hyperion_ros2_text_to_action_node_telemetry(const HyperionRos2TextToActionNode *node);

#ifdef __cplusplus
}
#endif

#endif
