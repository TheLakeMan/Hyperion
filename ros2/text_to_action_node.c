#include "text_to_action_node.h"

#include <string.h>

#ifdef HYPERION_HAVE_ROS2

static void hyperion_ros2_reset_telemetry(HyperionRos2TextToActionNode *node)
{
    if (!node)
        return;

    node->telemetry.command_text = NULL;
    node->telemetry.action_text = NULL;
    node->telemetry.remote_used = false;
    node->telemetry.local_time_ms = 0.0;
    node->telemetry.remote_time_ms = 0.0;
    node->telemetry.tokens_per_second = 0.0;
}

static void hyperion_ros2_set_default_params(HyperionGenerationParams *params)
{
    if (!params)
        return;
    params->maxTokens = 64;
    params->samplingMethod = HYPERION_SAMPLING_TOP_P;
    params->temperature = 0.7f;
    params->topK = 40;
    params->topP = 0.9f;
    params->seed = 0;
    params->promptTokens = NULL;
    params->promptLength = 0;
    params->style = HYPERION_STYLE_CONCISE;
}

static void hyperion_ros2_publish_action(HyperionRos2TextToActionNode *node,
                                         const char *text)
{
    std_msgs__msg__String action_msg;
    std_msgs__msg__String__init(&action_msg);
    rosidl_runtime_c__String__assign(&action_msg.data, text);
    rcl_publish(&node->action_pub, &action_msg, NULL);
    std_msgs__msg__String__fini(&action_msg);
}

static void hyperion_ros2_handle_command(const void *msgin, void *context)
{
    HyperionRos2TextToActionNode *node = (HyperionRos2TextToActionNode *)context;
    const std_msgs__msg__String *msg = (const std_msgs__msg__String *)msgin;

    if (!node || !node->model || !node->tokenizer || !msg)
        return;

    int promptTokens[HYPERION_ROS2_MAX_PROMPT_TOKENS];
    int outputTokens[HYPERION_ROS2_MAX_OUTPUT_TOKENS];
    char action_text[HYPERION_ROS2_MAX_OUTPUT_CHARS];

    const char *command_text = msg->data.data ? msg->data.data : "";
    strncpy(node->last_command, command_text, sizeof(node->last_command) - 1);
    node->last_command[sizeof(node->last_command) - 1] = '\0';
    node->telemetry.command_text = node->last_command;
    node->last_action[0] = '\0';
    node->telemetry.action_text = NULL;

    int prompt_len = hyperionEncodeText(node->tokenizer, node->last_command, promptTokens,
                                        HYPERION_ROS2_MAX_PROMPT_TOKENS);
    if (prompt_len <= 0)
        return;

    HyperionGenerationParams params = node->params;
    params.promptTokens = promptTokens;
    params.promptLength = prompt_len;

    int generated = 0;
    if (node->hybrid) {
        generated = hyperionHybridGenerateText(node->hybrid, &params, outputTokens,
                                               HYPERION_ROS2_MAX_OUTPUT_TOKENS);
        node->telemetry.remote_used = hyperionHybridGenerateUsedRemote(node->hybrid);
        hyperionHybridGenerateGetStats(node->hybrid,
                                       &node->telemetry.local_time_ms,
                                       &node->telemetry.remote_time_ms,
                                       &node->telemetry.tokens_per_second);
    }
    else {
        generated = hyperionGenerateText(node->model, &params, outputTokens,
                                         HYPERION_ROS2_MAX_OUTPUT_TOKENS);
        node->telemetry.remote_used = false;
        node->telemetry.local_time_ms = 0.0;
        node->telemetry.remote_time_ms = 0.0;
        node->telemetry.tokens_per_second = 0.0;
    }

    if (generated <= 0)
        return;

    int written = hyperionDecodeTokens(node->tokenizer, outputTokens, generated,
                                       action_text, HYPERION_ROS2_MAX_OUTPUT_CHARS - 1);
    if (written < 0)
        return;

    action_text[written] = '\0';
    strncpy(node->last_action, action_text, sizeof(node->last_action) - 1);
    node->last_action[sizeof(node->last_action) - 1] = '\0';

    node->telemetry.action_text = node->last_action;

    hyperion_ros2_publish_action(node, node->last_action);
}

int hyperion_ros2_text_to_action_node_init(HyperionRos2TextToActionNode *node,
                                           const char *node_name,
                                           const char *command_topic,
                                           const char *action_topic,
                                           HyperionModel *model,
                                           HyperionTokenizer *tokenizer)
{
    if (!node || !node_name || !command_topic || !action_topic || !model || !tokenizer)
        return HYPERION_ROS2_ERROR_INIT;

    memset(node, 0, sizeof(*node));

    node->model = model;
    node->tokenizer = tokenizer;
    node->hybrid = NULL;
    node->mcp_client = NULL;
    hyperion_ros2_set_default_params(&node->params);
    hyperion_ros2_reset_telemetry(node);

    node->allocator = rcl_get_default_allocator();
    rcl_ret_t ret = rclc_support_init(&node->support, 0, NULL, &node->allocator);
    if (ret != RCL_RET_OK)
        return HYPERION_ROS2_ERROR_INIT;

    ret = rclc_node_init_default(&node->node, node_name, "", &node->support);
    if (ret != RCL_RET_OK)
        goto fail_support;

    ret = rclc_subscription_init_default(&node->command_sub, &node->node,
                                         ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
                                         command_topic);
    if (ret != RCL_RET_OK)
        goto fail_node;

    ret = rclc_publisher_init_default(&node->action_pub, &node->node,
                                      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
                                      action_topic);
    if (ret != RCL_RET_OK)
        goto fail_subscription;

    std_msgs__msg__String__init(&node->command_msg);

    ret = rclc_executor_init(&node->executor, &node->support.context, 1, &node->allocator);
    if (ret != RCL_RET_OK)
        goto fail_publisher;

    ret = rclc_executor_add_subscription(&node->executor, &node->command_sub,
                                         &node->command_msg, hyperion_ros2_handle_command,
                                         node, ON_NEW_DATA);
    if (ret != RCL_RET_OK)
        goto fail_executor;

    node->last_action[0] = '\0';
    node->last_command[0] = '\0';
    return 0;

fail_executor:
    rclc_executor_fini(&node->executor);
fail_publisher:
    std_msgs__msg__String__fini(&node->command_msg);
    rcl_publisher_fini(&node->action_pub, &node->node);
fail_subscription:
    rcl_subscription_fini(&node->command_sub, &node->node);
fail_node:
    rcl_node_fini(&node->node);
fail_support:
    rclc_support_fini(&node->support);
    return HYPERION_ROS2_ERROR_INIT;
}

void hyperion_ros2_text_to_action_node_fini(HyperionRos2TextToActionNode *node)
{
    if (!node)
        return;

    rclc_executor_fini(&node->executor);
    std_msgs__msg__String__fini(&node->command_msg);
    rcl_publisher_fini(&node->action_pub, &node->node);
    rcl_subscription_fini(&node->command_sub, &node->node);
    rcl_node_fini(&node->node);
    rclc_support_fini(&node->support);

    if (node->hybrid) {
        hyperionDestroyHybridGenerate(node->hybrid);
        node->hybrid = NULL;
    }

    node->model = NULL;
    node->tokenizer = NULL;
    node->mcp_client = NULL;
    node->last_action[0] = '\0';
    node->last_command[0] = '\0';
    hyperion_ros2_reset_telemetry(node);
}

int hyperion_ros2_text_to_action_node_spin_some(HyperionRos2TextToActionNode *node,
                                                uint64_t timeout_ns)
{
    if (!node)
        return HYPERION_ROS2_ERROR_UNAVAILABLE;

    rcl_ret_t ret = rclc_executor_spin_some(&node->executor, timeout_ns);
    return (ret == RCL_RET_OK || ret == RCL_RET_TIMEOUT) ? 0 : HYPERION_ROS2_ERROR_INFERENCE;
}

void hyperion_ros2_text_to_action_node_set_params(HyperionRos2TextToActionNode *node,
                                                  const HyperionGenerationParams *params)
{
    if (!node || !params)
        return;
    node->params = *params;
}

const char *hyperion_ros2_text_to_action_node_last_action(const HyperionRos2TextToActionNode *node)
{
    if (!node)
        return NULL;
    return node->last_action;
}

void hyperion_ros2_text_to_action_node_use_hybrid(HyperionRos2TextToActionNode *node,
                                                  HyperionMcpClient *mcp_client)
{
    if (!node)
        return;

    if (node->hybrid) {
        hyperionDestroyHybridGenerate(node->hybrid);
        node->hybrid = NULL;
    }

    node->mcp_client = mcp_client;
    if (mcp_client)
        node->hybrid = hyperionCreateHybridGenerate(node->model, mcp_client);
}

const HyperionRos2TextToActionTelemetry *
hyperion_ros2_text_to_action_node_telemetry(const HyperionRos2TextToActionNode *node)
{
    if (!node)
        return NULL;
    return &node->telemetry;
}

#else

int hyperion_ros2_text_to_action_node_init(HyperionRos2TextToActionNode *node,
                                           const char *node_name,
                                           const char *command_topic,
                                           const char *action_topic,
                                           HyperionModel *model,
                                           HyperionTokenizer *tokenizer)
{
    (void)node;
    (void)node_name;
    (void)command_topic;
    (void)action_topic;
    (void)model;
    (void)tokenizer;
    return HYPERION_ROS2_ERROR_UNAVAILABLE;
}

void hyperion_ros2_text_to_action_node_fini(HyperionRos2TextToActionNode *node)
{
    (void)node;
}

int hyperion_ros2_text_to_action_node_spin_some(HyperionRos2TextToActionNode *node,
                                                uint64_t timeout_ns)
{
    (void)node;
    (void)timeout_ns;
    return HYPERION_ROS2_ERROR_UNAVAILABLE;
}

void hyperion_ros2_text_to_action_node_set_params(HyperionRos2TextToActionNode *node,
                                                  const HyperionGenerationParams *params)
{
    (void)node;
    (void)params;
}

const char *hyperion_ros2_text_to_action_node_last_action(const HyperionRos2TextToActionNode *node)
{
    (void)node;
    return NULL;
}

void hyperion_ros2_text_to_action_node_use_hybrid(HyperionRos2TextToActionNode *node,
                                                  HyperionMcpClient *mcp_client)
{
    (void)node;
    (void)mcp_client;
}

const HyperionRos2TextToActionTelemetry *
hyperion_ros2_text_to_action_node_telemetry(const HyperionRos2TextToActionNode *node)
{
    (void)node;
    return NULL;
}

#endif
