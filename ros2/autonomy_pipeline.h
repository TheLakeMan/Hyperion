#ifndef HYPERION_ROS2_AUTONOMY_PIPELINE_H
#define HYPERION_ROS2_AUTONOMY_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

#include "text_to_action_node.h"
#include "perception_bridge.h"
#include "control_bridge.h"

typedef struct {
    const char *perception_node_name;
    const char *perception_topic;
    const char *command_topic;
    const char *action_topic;
    const char *control_node_name;
    const char *velocity_topic;
    HyperionMcpClient *mcp_client;
    HyperionGenerationParams generation;
    uint64_t control_watchdog_timeout_ns;
} HyperionRos2AutonomyConfig;

typedef struct {
    const char *command_text;
    const char *action_text;
    HyperionRos2ControlCommand control;
    bool                       remote_used;
    double                     local_time_ms;
    double                     remote_time_ms;
    double                     tokens_per_second;
} HyperionRos2AutonomyTelemetry;

typedef struct HyperionRos2AutonomyPipeline HyperionRos2AutonomyPipeline;

#ifdef __cplusplus
extern "C" {
#endif

HyperionRos2AutonomyPipeline *hyperion_ros2_autonomy_create(void);

void hyperion_ros2_autonomy_destroy(HyperionRos2AutonomyPipeline *pipeline);

int hyperion_ros2_autonomy_init(HyperionRos2AutonomyPipeline *pipeline,
                                const HyperionRos2AutonomyConfig *config,
                                HyperionModel *model,
                                HyperionTokenizer *tokenizer);

void hyperion_ros2_autonomy_fini(HyperionRos2AutonomyPipeline *pipeline);

int hyperion_ros2_autonomy_spin_some(HyperionRos2AutonomyPipeline *pipeline,
                                     uint64_t timeout_ns);

const char *hyperion_ros2_autonomy_last_action(const HyperionRos2AutonomyPipeline *pipeline);

const HyperionRos2ControlCommand *hyperion_ros2_autonomy_last_control(
    const HyperionRos2AutonomyPipeline *pipeline);

const char *hyperion_ros2_autonomy_last_command(const HyperionRos2AutonomyPipeline *pipeline);

const HyperionRos2AutonomyTelemetry *hyperion_ros2_autonomy_telemetry(
    const HyperionRos2AutonomyPipeline *pipeline);

#ifdef __cplusplus
}
#endif

#endif
