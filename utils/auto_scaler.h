#ifndef HYPERION_AUTO_SCALER_H
#define HYPERION_AUTO_SCALER_H

#include <stddef.h>
#include <time.h>

#include "monitoring_center.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HYPERION_AUTOSCALE_DECISION_NONE = 0,
    HYPERION_AUTOSCALE_DECISION_SCALE_UP,
    HYPERION_AUTOSCALE_DECISION_SCALE_DOWN
} HyperionAutoScaleDecision;

typedef struct {
    char metric_name[64];
    double scale_up_threshold;
    double scale_down_threshold;
    size_t scale_step;
    size_t min_replicas;
    size_t max_replicas;
    int scale_up_cooldown_seconds;
    int scale_down_cooldown_seconds;
} HyperionAutoScalerPolicy;

typedef struct HyperionAutoScaler HyperionAutoScaler;

HyperionAutoScaler *hyperionAutoScalerCreate(const HyperionAutoScalerPolicy *policy,
                                             HyperionMonitoringCenter *monitoring_center);
void hyperionAutoScalerDestroy(HyperionAutoScaler *scaler);
void hyperionAutoScalerReset(HyperionAutoScaler *scaler, const HyperionAutoScalerPolicy *policy);

int hyperionAutoScalerPlan(HyperionAutoScaler *scaler,
                           size_t current_replicas,
                           HyperionAutoScaleDecision *decision_out,
                           size_t *desired_replicas,
                           double *metric_value,
                           char *reason_buffer,
                           size_t reason_length);

void hyperionAutoScalerRecord(HyperionAutoScaler *scaler,
                              HyperionAutoScaleDecision decision,
                              size_t applied_replicas);

void hyperionAutoScalerSync(HyperionAutoScaler *scaler, size_t current_replicas);

const HyperionAutoScalerPolicy *hyperionAutoScalerPolicy(const HyperionAutoScaler *scaler);

#ifdef __cplusplus
}
#endif

#endif
