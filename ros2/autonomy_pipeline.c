#include "autonomy_pipeline.h"

#include <string.h>
#include <stdlib.h>

#ifdef HYPERION_HAVE_ROS2

struct HyperionRos2AutonomyPipeline {
    HyperionRos2PerceptionBridge perception;
    HyperionRos2TextToActionNode brain;
    HyperionRos2ControlBridge control;
    HyperionRos2AutonomyConfig cfg;
    HyperionRos2AutonomyTelemetry telemetry;
};

HyperionRos2AutonomyPipeline *hyperion_ros2_autonomy_create(void)
{
#ifdef HYPERION_HAVE_ROS2
    HyperionRos2AutonomyPipeline *pipeline =
        (HyperionRos2AutonomyPipeline *)malloc(sizeof(HyperionRos2AutonomyPipeline));
    if (pipeline)
        memset(pipeline, 0, sizeof(*pipeline));
    return pipeline;
#else
    return NULL;
#endif
}

void hyperion_ros2_autonomy_destroy(HyperionRos2AutonomyPipeline *pipeline)
{
#ifdef HYPERION_HAVE_ROS2
    free(pipeline);
#else
    (void)pipeline;
#endif
}

static void hyperion_ros2_autonomy_apply_defaults(HyperionRos2AutonomyConfig *cfg)
{
    if (!cfg)
        return;

    if (!cfg->perception_node_name)
        cfg->perception_node_name = "hyperion_perception";
    if (!cfg->perception_topic)
        cfg->perception_topic = "/hyperion/targets";
    if (!cfg->command_topic)
        cfg->command_topic = "/hyperion/command";
    if (!cfg->action_topic)
        cfg->action_topic = "/hyperion/action";
    if (!cfg->control_node_name)
        cfg->control_node_name = "hyperion_control";
    if (!cfg->velocity_topic)
        cfg->velocity_topic = "/cmd_vel";
    if (cfg->control_watchdog_timeout_ns == 0)
        cfg->control_watchdog_timeout_ns = HYPERION_ROS2_CONTROL_DEFAULT_TIMEOUT_NS;

    HyperionGenerationParams *gen = &cfg->generation;
    if (gen->maxTokens <= 0)
        gen->maxTokens = 64;
    if (gen->samplingMethod < HYPERION_SAMPLING_GREEDY ||
        gen->samplingMethod > HYPERION_SAMPLING_TOP_P)
        gen->samplingMethod = HYPERION_SAMPLING_TOP_P;
    if (gen->samplingMethod == HYPERION_SAMPLING_GREEDY)
        gen->samplingMethod = HYPERION_SAMPLING_TOP_P;
    if (gen->temperature <= 0.0f)
        gen->temperature = 0.7f;
    if (gen->topK == 0)
        gen->topK = 40;
    if (gen->topP <= 0.0f)
        gen->topP = 0.9f;
    if (gen->style > HYPERION_STYLE_DESCRIPTIVE)
        gen->style = HYPERION_STYLE_CONCISE;
    gen->promptTokens = NULL;
    gen->promptLength = 0;
}

int hyperion_ros2_autonomy_init(HyperionRos2AutonomyPipeline *pipeline,
                                const HyperionRos2AutonomyConfig *config,
                                HyperionModel *model,
                                HyperionTokenizer *tokenizer)
{
    if (!pipeline || !config || !model || !tokenizer)
        return HYPERION_ROS2_ERROR_INIT;

    memset(pipeline, 0, sizeof(*pipeline));

    pipeline->cfg = *config;

    hyperion_ros2_autonomy_apply_defaults(&pipeline->cfg);

    int rc = hyperion_ros2_perception_bridge_init(&pipeline->perception,
                                                  pipeline->cfg.perception_node_name,
                                                  pipeline->cfg.perception_topic,
                                                  pipeline->cfg.command_topic);
    if (rc != HYPERION_ROS2_BRIDGE_OK)
        return rc;

    rc = hyperion_ros2_text_to_action_node_init(&pipeline->brain,
                                                "hyperion_brain",
                                                pipeline->cfg.command_topic,
                                                pipeline->cfg.action_topic,
                                                model,
                                                tokenizer);
    if (rc != 0)
        goto fail_perception;

    hyperion_ros2_text_to_action_node_set_params(&pipeline->brain, &pipeline->cfg.generation);

    if (pipeline->cfg.mcp_client)
        hyperion_ros2_text_to_action_node_use_hybrid(&pipeline->brain, pipeline->cfg.mcp_client);

    rc = hyperion_ros2_control_bridge_init(&pipeline->control,
                                           pipeline->cfg.control_node_name,
                                           pipeline->cfg.action_topic,
                                           pipeline->cfg.velocity_topic);
    if (rc != HYPERION_ROS2_CONTROL_OK)
        goto fail_brain;

    hyperion_ros2_control_bridge_set_watchdog_timeout(&pipeline->control,
                                                      pipeline->cfg.control_watchdog_timeout_ns);

    hyperion_ros2_perception_bridge_set_formatter(&pipeline->perception, NULL);

    memset(&pipeline->telemetry, 0, sizeof(pipeline->telemetry));

    return 0;

fail_brain:
    hyperion_ros2_text_to_action_node_fini(&pipeline->brain);
fail_perception:
    hyperion_ros2_perception_bridge_fini(&pipeline->perception);
    return rc;
}

void hyperion_ros2_autonomy_fini(HyperionRos2AutonomyPipeline *pipeline)
{
    if (!pipeline)
        return;

    hyperion_ros2_control_bridge_fini(&pipeline->control);
    hyperion_ros2_text_to_action_node_fini(&pipeline->brain);
    hyperion_ros2_perception_bridge_fini(&pipeline->perception);
    memset(pipeline, 0, sizeof(*pipeline));
}

int hyperion_ros2_autonomy_spin_some(HyperionRos2AutonomyPipeline *pipeline,
                                     uint64_t timeout_ns)
{
    if (!pipeline)
        return HYPERION_ROS2_ERROR_UNAVAILABLE;

    int rc = hyperion_ros2_perception_bridge_spin_some(&pipeline->perception, timeout_ns);
    if (rc != HYPERION_ROS2_BRIDGE_OK)
        return rc;

    rc = hyperion_ros2_text_to_action_node_spin_some(&pipeline->brain, timeout_ns);
    if (rc != 0)
        return rc;

    rc = hyperion_ros2_control_bridge_spin_some(&pipeline->control, timeout_ns);
    if (rc != HYPERION_ROS2_CONTROL_OK)
        return rc;

    const HyperionRos2TextToActionTelemetry *brainTelemetry =
        hyperion_ros2_text_to_action_node_telemetry(&pipeline->brain);

    if (brainTelemetry) {
        pipeline->telemetry.command_text = brainTelemetry->command_text;
        pipeline->telemetry.action_text = brainTelemetry->action_text;
        pipeline->telemetry.remote_used = brainTelemetry->remote_used;
        pipeline->telemetry.local_time_ms = brainTelemetry->local_time_ms;
        pipeline->telemetry.remote_time_ms = brainTelemetry->remote_time_ms;
        pipeline->telemetry.tokens_per_second = brainTelemetry->tokens_per_second;
    }
    else {
        pipeline->telemetry.command_text = NULL;
        pipeline->telemetry.action_text = NULL;
        pipeline->telemetry.remote_used = false;
        pipeline->telemetry.local_time_ms = 0.0;
        pipeline->telemetry.remote_time_ms = 0.0;
        pipeline->telemetry.tokens_per_second = 0.0;
    }

    if (!pipeline->telemetry.command_text)
        pipeline->telemetry.command_text =
            hyperion_ros2_perception_bridge_last_command(&pipeline->perception);

    const HyperionRos2ControlCommand *control =
        hyperion_ros2_control_bridge_last_command(&pipeline->control);
    if (control)
        pipeline->telemetry.control = *control;
    else
        memset(&pipeline->telemetry.control, 0, sizeof(pipeline->telemetry.control));

    return 0;
}

const char *hyperion_ros2_autonomy_last_action(const HyperionRos2AutonomyPipeline *pipeline)
{
    if (!pipeline)
        return NULL;
    if (pipeline->telemetry.action_text)
        return pipeline->telemetry.action_text;
    return hyperion_ros2_text_to_action_node_last_action(&pipeline->brain);
}

const HyperionRos2ControlCommand *hyperion_ros2_autonomy_last_control(
    const HyperionRos2AutonomyPipeline *pipeline)
{
    if (!pipeline)
        return NULL;
    return &pipeline->telemetry.control;
}

const char *hyperion_ros2_autonomy_last_command(const HyperionRos2AutonomyPipeline *pipeline)
{
    if (!pipeline)
        return NULL;
    if (pipeline->telemetry.command_text)
        return pipeline->telemetry.command_text;
    return hyperion_ros2_perception_bridge_last_command(&pipeline->perception);
}

const HyperionRos2AutonomyTelemetry *hyperion_ros2_autonomy_telemetry(
    const HyperionRos2AutonomyPipeline *pipeline)
{
    if (!pipeline)
        return NULL;
    return &pipeline->telemetry;
}

#else

struct HyperionRos2AutonomyPipeline {
    int unused;
};

HyperionRos2AutonomyPipeline *hyperion_ros2_autonomy_create(void)
{
    return NULL;
}

void hyperion_ros2_autonomy_destroy(HyperionRos2AutonomyPipeline *pipeline)
{
    (void)pipeline;
}

int hyperion_ros2_autonomy_init(HyperionRos2AutonomyPipeline *pipeline,
                                const HyperionRos2AutonomyConfig *config,
                                HyperionModel *model,
                                HyperionTokenizer *tokenizer)
{
    (void)pipeline;
    (void)config;
    (void)model;
    (void)tokenizer;
    return HYPERION_ROS2_ERROR_UNAVAILABLE;
}

void hyperion_ros2_autonomy_fini(HyperionRos2AutonomyPipeline *pipeline)
{
    (void)pipeline;
}

int hyperion_ros2_autonomy_spin_some(HyperionRos2AutonomyPipeline *pipeline,
                                     uint64_t timeout_ns)
{
    (void)pipeline;
    (void)timeout_ns;
    return HYPERION_ROS2_ERROR_UNAVAILABLE;
}

const char *hyperion_ros2_autonomy_last_action(const HyperionRos2AutonomyPipeline *pipeline)
{
    (void)pipeline;
    return NULL;
}

const HyperionRos2ControlCommand *hyperion_ros2_autonomy_last_control(
    const HyperionRos2AutonomyPipeline *pipeline)
{
    (void)pipeline;
    return NULL;
}

const char *hyperion_ros2_autonomy_last_command(const HyperionRos2AutonomyPipeline *pipeline)
{
    (void)pipeline;
    return NULL;
}

const HyperionRos2AutonomyTelemetry *hyperion_ros2_autonomy_telemetry(
    const HyperionRos2AutonomyPipeline *pipeline)
{
    (void)pipeline;
    return NULL;
}

#endif
