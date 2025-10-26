#ifndef HYPERION_ROS2_AUTONOMY_MONITOR_H
#define HYPERION_ROS2_AUTONOMY_MONITOR_H

#include <stdint.h>
#include <stdbool.h>

#include "autonomy_pipeline.h"

#define HYPERION_ROS2_MONITOR_DEFAULT_REMOTE_LATENCY_MS 200.0
#define HYPERION_ROS2_MONITOR_DEFAULT_LOCAL_LATENCY_MS   50.0

typedef struct {
    bool     remote_latency_within_limit;
    bool     local_latency_within_limit;
    bool     remote_available;
    bool     control_active;
    uint32_t remote_latency_violations;
    uint32_t local_latency_violations;
    uint32_t samples;
} HyperionRos2AutonomyHealth;

typedef struct HyperionRos2AutonomyMonitor HyperionRos2AutonomyMonitor;

#ifdef __cplusplus
extern "C" {
#endif

int hyperion_ros2_autonomy_monitor_init(HyperionRos2AutonomyMonitor *monitor,
                                        const HyperionRos2AutonomyConfig *config,
                                        HyperionModel *model,
                                        HyperionTokenizer *tokenizer);

void hyperion_ros2_autonomy_monitor_fini(HyperionRos2AutonomyMonitor *monitor);

int hyperion_ros2_autonomy_monitor_spin_some(HyperionRos2AutonomyMonitor *monitor,
                                             uint64_t timeout_ns);

void hyperion_ros2_autonomy_monitor_set_latency_thresholds(HyperionRos2AutonomyMonitor *monitor,
                                                           double remote_ms,
                                                           double local_ms);

const HyperionRos2AutonomyTelemetry *hyperion_ros2_autonomy_monitor_telemetry(
    const HyperionRos2AutonomyMonitor *monitor);

const HyperionRos2AutonomyHealth *hyperion_ros2_autonomy_monitor_health(
    const HyperionRos2AutonomyMonitor *monitor);

#ifdef __cplusplus
}
#endif

#endif
