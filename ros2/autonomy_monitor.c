#include "autonomy_monitor.h"

#include <string.h>

struct HyperionRos2AutonomyMonitor {
#ifdef HYPERION_HAVE_ROS2
    HyperionRos2AutonomyPipeline *pipeline;
    HyperionRos2AutonomyConfig    config;
    HyperionRos2AutonomyTelemetry telemetry;
    HyperionRos2AutonomyHealth    health;
    double                        remote_latency_threshold_ms;
    double                        local_latency_threshold_ms;
    uint32_t                      remote_latency_violations;
    uint32_t                      local_latency_violations;
    uint32_t                      samples;
#else
    int unused;
#endif
};

#ifdef HYPERION_HAVE_ROS2

static void hyperion_ros2_monitor_reset(HyperionRos2AutonomyMonitor *monitor)
{
    monitor->telemetry.command_text       = NULL;
    monitor->telemetry.action_text        = NULL;
    monitor->telemetry.control.active     = false;
    monitor->telemetry.control.linear_x   = 0.0f;
    monitor->telemetry.control.linear_y   = 0.0f;
    monitor->telemetry.control.linear_z   = 0.0f;
    monitor->telemetry.control.angular_z  = 0.0f;
    monitor->telemetry.remote_used        = false;
    monitor->telemetry.local_time_ms      = 0.0;
    monitor->telemetry.remote_time_ms     = 0.0;
    monitor->telemetry.tokens_per_second  = 0.0;

    monitor->health.remote_latency_within_limit = true;
    monitor->health.local_latency_within_limit  = true;
    monitor->health.remote_available            = false;
    monitor->health.control_active              = false;
    monitor->health.remote_latency_violations   = 0;
    monitor->health.local_latency_violations    = 0;
    monitor->health.samples                     = 0;

    monitor->remote_latency_threshold_ms = HYPERION_ROS2_MONITOR_DEFAULT_REMOTE_LATENCY_MS;
    monitor->local_latency_threshold_ms  = HYPERION_ROS2_MONITOR_DEFAULT_LOCAL_LATENCY_MS;
    monitor->remote_latency_violations   = 0;
    monitor->local_latency_violations    = 0;
    monitor->samples                     = 0;
}

int hyperion_ros2_autonomy_monitor_init(HyperionRos2AutonomyMonitor *monitor,
                                        const HyperionRos2AutonomyConfig *config,
                                        HyperionModel *model,
                                        HyperionTokenizer *tokenizer)
{
    if (!monitor || !config || !model || !tokenizer)
        return HYPERION_ROS2_ERROR_INIT;

    memset(monitor, 0, sizeof(*monitor));
    monitor->config = *config;
    hyperion_ros2_monitor_reset(monitor);

    monitor->pipeline = hyperion_ros2_autonomy_create();
    if (!monitor->pipeline)
        return HYPERION_ROS2_ERROR_INIT;

    int rc = hyperion_ros2_autonomy_init(monitor->pipeline, &monitor->config, model, tokenizer);
    if (rc != 0) {
        hyperion_ros2_autonomy_destroy(monitor->pipeline);
        monitor->pipeline = NULL;
        return rc;
    }

    const HyperionRos2AutonomyTelemetry *telemetry =
        hyperion_ros2_autonomy_telemetry(monitor->pipeline);
    if (telemetry)
        monitor->telemetry = *telemetry;

    return 0;
}

void hyperion_ros2_autonomy_monitor_fini(HyperionRos2AutonomyMonitor *monitor)
{
    if (!monitor)
        return;

    if (monitor->pipeline) {
        hyperion_ros2_autonomy_fini(monitor->pipeline);
        hyperion_ros2_autonomy_destroy(monitor->pipeline);
        monitor->pipeline = NULL;
    }
}

int hyperion_ros2_autonomy_monitor_spin_some(HyperionRos2AutonomyMonitor *monitor,
                                             uint64_t timeout_ns)
{
    if (!monitor || !monitor->pipeline)
        return HYPERION_ROS2_ERROR_UNAVAILABLE;

    int rc = hyperion_ros2_autonomy_spin_some(monitor->pipeline, timeout_ns);
    if (rc != 0)
        return rc;

    const HyperionRos2AutonomyTelemetry *telemetry =
        hyperion_ros2_autonomy_telemetry(monitor->pipeline);
    if (telemetry)
        monitor->telemetry = *telemetry;
    else
        hyperion_ros2_monitor_reset(monitor);

    monitor->samples++;

    double remote_time = monitor->telemetry.remote_time_ms;
    double local_time  = monitor->telemetry.local_time_ms;

    bool remote_violation = false;
    if (monitor->remote_latency_threshold_ms > 0.0 && monitor->telemetry.remote_used) {
        remote_violation = remote_time > monitor->remote_latency_threshold_ms;
        if (remote_violation)
            monitor->remote_latency_violations++;
    }

    bool local_violation = false;
    if (monitor->local_latency_threshold_ms > 0.0) {
        double observed = monitor->telemetry.remote_used ? remote_time : local_time;
        local_violation = observed > monitor->local_latency_threshold_ms;
        if (local_violation)
            monitor->local_latency_violations++;
    }

    monitor->health.remote_latency_within_limit = !remote_violation;
    monitor->health.local_latency_within_limit  = !local_violation;
    monitor->health.remote_available            = monitor->telemetry.remote_used;
    monitor->health.control_active              = monitor->telemetry.control.active;
    monitor->health.remote_latency_violations   = monitor->remote_latency_violations;
    monitor->health.local_latency_violations    = monitor->local_latency_violations;
    monitor->health.samples                     = monitor->samples;

    return 0;
}

void hyperion_ros2_autonomy_monitor_set_latency_thresholds(HyperionRos2AutonomyMonitor *monitor,
                                                           double remote_ms,
                                                           double local_ms)
{
    if (!monitor)
        return;

    if (remote_ms > 0.0)
        monitor->remote_latency_threshold_ms = remote_ms;
    if (local_ms > 0.0)
        monitor->local_latency_threshold_ms = local_ms;
}

const HyperionRos2AutonomyTelemetry *hyperion_ros2_autonomy_monitor_telemetry(
    const HyperionRos2AutonomyMonitor *monitor)
{
    if (!monitor)
        return NULL;
    return &monitor->telemetry;
}

const HyperionRos2AutonomyHealth *hyperion_ros2_autonomy_monitor_health(
    const HyperionRos2AutonomyMonitor *monitor)
{
    if (!monitor)
        return NULL;
    return &monitor->health;
}

#else

int hyperion_ros2_autonomy_monitor_init(HyperionRos2AutonomyMonitor *monitor,
                                        const HyperionRos2AutonomyConfig *config,
                                        HyperionModel *model,
                                        HyperionTokenizer *tokenizer)
{
    (void)monitor;
    (void)config;
    (void)model;
    (void)tokenizer;
    return HYPERION_ROS2_ERROR_UNAVAILABLE;
}

void hyperion_ros2_autonomy_monitor_fini(HyperionRos2AutonomyMonitor *monitor)
{
    (void)monitor;
}

int hyperion_ros2_autonomy_monitor_spin_some(HyperionRos2AutonomyMonitor *monitor,
                                             uint64_t timeout_ns)
{
    (void)monitor;
    (void)timeout_ns;
    return HYPERION_ROS2_ERROR_UNAVAILABLE;
}

void hyperion_ros2_autonomy_monitor_set_latency_thresholds(HyperionRos2AutonomyMonitor *monitor,
                                                           double remote_ms,
                                                           double local_ms)
{
    (void)monitor;
    (void)remote_ms;
    (void)local_ms;
}

const HyperionRos2AutonomyTelemetry *hyperion_ros2_autonomy_monitor_telemetry(
    const HyperionRos2AutonomyMonitor *monitor)
{
    (void)monitor;
    return NULL;
}

const HyperionRos2AutonomyHealth *hyperion_ros2_autonomy_monitor_health(
    const HyperionRos2AutonomyMonitor *monitor)
{
    (void)monitor;
    return NULL;
}

#endif
