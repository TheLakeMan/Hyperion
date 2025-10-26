#include "auto_scaler.h"
#include "monitoring_center.h"
#include "../core/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

struct HyperionAutoScaler {
    HyperionAutoScalerPolicy policy;
    HyperionMonitoringCenter *monitor;
    time_t last_scale_up;
    time_t last_scale_down;
    size_t last_known_replicas;
};

static void copy_policy(HyperionAutoScalerPolicy *dst, const HyperionAutoScalerPolicy *src)
{
    if (!dst) return;
    if (src) {
        *dst = *src;
    } else {
        memset(dst, 0, sizeof(*dst));
        dst->scale_step = 1;
        dst->min_replicas = 1;
        dst->max_replicas = 1;
    }

    if (dst->scale_step == 0) dst->scale_step = 1;
    if (dst->min_replicas == 0) dst->min_replicas = 1;
    if (dst->max_replicas < dst->min_replicas) dst->max_replicas = dst->min_replicas;
    if (dst->scale_up_cooldown_seconds < 0) dst->scale_up_cooldown_seconds = 0;
    if (dst->scale_down_cooldown_seconds < 0) dst->scale_down_cooldown_seconds = 0;
}

HyperionAutoScaler *hyperionAutoScalerCreate(const HyperionAutoScalerPolicy *policy,
                                             HyperionMonitoringCenter *monitoring_center)
{
    HyperionAutoScaler *scaler = (HyperionAutoScaler *)HYPERION_CALLOC(1, sizeof(*scaler));
    if (!scaler) {
        return NULL;
    }

    scaler->monitor = monitoring_center;
    hyperionAutoScalerReset(scaler, policy);
    return scaler;
}

void hyperionAutoScalerDestroy(HyperionAutoScaler *scaler)
{
    if (!scaler) return;
    HYPERION_FREE(scaler);
}

void hyperionAutoScalerReset(HyperionAutoScaler *scaler, const HyperionAutoScalerPolicy *policy)
{
    if (!scaler) return;
    copy_policy(&scaler->policy, policy);
    scaler->last_scale_up = 0;
    scaler->last_scale_down = 0;
    scaler->last_known_replicas = scaler->policy.min_replicas;
}

static void write_reason(char *buffer, size_t length, const char *format, ...)
{
    if (!buffer || length == 0) return;
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, length, format, args);
    va_end(args);
}

int hyperionAutoScalerPlan(HyperionAutoScaler *scaler,
                           size_t current_replicas,
                           HyperionAutoScaleDecision *decision_out,
                           size_t *desired_replicas,
                           double *metric_value,
                           char *reason_buffer,
                           size_t reason_length)
{
    if (decision_out) *decision_out = HYPERION_AUTOSCALE_DECISION_NONE;
    if (desired_replicas) *desired_replicas = current_replicas;
    if (metric_value) *metric_value = 0.0;
    if (reason_buffer && reason_length > 0) reason_buffer[0] = '\0';

    if (!scaler || scaler->policy.metric_name[0] == '\0') {
        write_reason(reason_buffer, reason_length, "Autoscaler disabled: no metric configured");
        return 0;
    }

    HyperionMonitorMetricSnapshot snapshot;
    if (!hyperionMonitoringGetMetric(scaler->monitor, scaler->policy.metric_name, &snapshot)) {
        write_reason(reason_buffer, reason_length, "Metric '%s' not available", scaler->policy.metric_name);
        return 0;
    }

    double value = snapshot.current;
    if (metric_value) {
        *metric_value = value;
    }

    time_t now = time(NULL);
    HyperionAutoScaleDecision decision = HYPERION_AUTOSCALE_DECISION_NONE;
    size_t target = current_replicas;

    if (current_replicas < scaler->policy.min_replicas) {
        target = scaler->policy.min_replicas;
        decision = HYPERION_AUTOSCALE_DECISION_SCALE_UP;
        write_reason(reason_buffer, reason_length, "Below minimum replica count (%zu)", scaler->policy.min_replicas);
    } else if (value >= scaler->policy.scale_up_threshold && current_replicas < scaler->policy.max_replicas) {
        if (now - scaler->last_scale_up >= scaler->policy.scale_up_cooldown_seconds) {
            size_t step = scaler->policy.scale_step;
            size_t proposed = current_replicas + step;
            if (proposed > scaler->policy.max_replicas) proposed = scaler->policy.max_replicas;
            if (proposed > current_replicas) {
                target = proposed;
                decision = HYPERION_AUTOSCALE_DECISION_SCALE_UP;
                write_reason(reason_buffer, reason_length,
                             "Metric %.2f >= %.2f (scale up)", value, scaler->policy.scale_up_threshold);
            }
        } else {
            write_reason(reason_buffer, reason_length, "Scale-up cooldown active");
        }
    } else if (value <= scaler->policy.scale_down_threshold && current_replicas > scaler->policy.min_replicas) {
        if (now - scaler->last_scale_down >= scaler->policy.scale_down_cooldown_seconds) {
            size_t step = scaler->policy.scale_step;
            size_t min_allowed = scaler->policy.min_replicas;
            size_t proposed = (current_replicas > step) ? current_replicas - step : min_allowed;
            if (proposed < min_allowed) proposed = min_allowed;
            if (proposed < current_replicas) {
                target = proposed;
                decision = HYPERION_AUTOSCALE_DECISION_SCALE_DOWN;
                write_reason(reason_buffer, reason_length,
                             "Metric %.2f <= %.2f (scale down)", value, scaler->policy.scale_down_threshold);
            }
        } else {
            write_reason(reason_buffer, reason_length, "Scale-down cooldown active");
        }
    } else if (reason_buffer && reason_length > 0 && reason_buffer[0] == '\0') {
        write_reason(reason_buffer, reason_length, "Within target range");
    }

    if (decision_out) *decision_out = decision;
    if (desired_replicas) *desired_replicas = target;

    return 1;
}

void hyperionAutoScalerRecord(HyperionAutoScaler *scaler,
                              HyperionAutoScaleDecision decision,
                              size_t applied_replicas)
{
    if (!scaler || decision == HYPERION_AUTOSCALE_DECISION_NONE) return;

    time_t now = time(NULL);
    if (decision == HYPERION_AUTOSCALE_DECISION_SCALE_UP) {
        scaler->last_scale_up = now;
        if (scaler->monitor) {
            char message[160];
            snprintf(message, sizeof(message), "Scaled up to %zu replicas", applied_replicas);
            hyperionMonitoringRecordLog(scaler->monitor, "INFO", message);
            hyperionMonitoringIncrementCounter(scaler->monitor, "autoscale.scale_up", "count",
                                               "Autoscale scale-up actions", 1.0);
        }
    } else if (decision == HYPERION_AUTOSCALE_DECISION_SCALE_DOWN) {
        scaler->last_scale_down = now;
        if (scaler->monitor) {
            char message[160];
            snprintf(message, sizeof(message), "Scaled down to %zu replicas", applied_replicas);
            hyperionMonitoringRecordLog(scaler->monitor, "INFO", message);
            hyperionMonitoringIncrementCounter(scaler->monitor, "autoscale.scale_down", "count",
                                               "Autoscale scale-down actions", 1.0);
        }
    }
    scaler->last_known_replicas = applied_replicas;
    if (scaler->monitor) {
        hyperionMonitoringSetGauge(scaler->monitor, "autoscale.desired_replicas", "count",
                                   "Recommended replica count", (double)applied_replicas);
    }
}

void hyperionAutoScalerSync(HyperionAutoScaler *scaler, size_t current_replicas)
{
    if (!scaler) return;
    scaler->last_known_replicas = current_replicas;
}

const HyperionAutoScalerPolicy *hyperionAutoScalerPolicy(const HyperionAutoScaler *scaler)
{
    if (!scaler) return NULL;
    return &scaler->policy;
}
