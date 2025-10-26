#ifndef HYPERION_MONITORING_CENTER_H
#define HYPERION_MONITORING_CENTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HYPERION_MONITOR_METRIC_COUNTER = 0,
    HYPERION_MONITOR_METRIC_GAUGE,
    HYPERION_MONITOR_METRIC_HISTOGRAM
} HyperionMonitorMetricType;

#define HYPERION_MONITOR_COMPARE_GREATER  1
#define HYPERION_MONITOR_COMPARE_LESS    -1
#define HYPERION_MONITOR_COMPARE_EQUAL    0

typedef struct HyperionMonitoringCenter HyperionMonitoringCenter;

typedef void (*HyperionMonitorAlertCallback)(const char *metric_name,
                                             double current_value,
                                             void *user_data);

HyperionMonitoringCenter *hyperionMonitoringCreate(size_t max_metrics);
void hyperionMonitoringDestroy(HyperionMonitoringCenter *center);
void hyperionMonitoringReset(HyperionMonitoringCenter *center);

HyperionMonitoringCenter *hyperionMonitoringInstance(void);
void hyperionMonitoringShutdown(void);

int hyperionMonitoringIncrementCounter(HyperionMonitoringCenter *center,
                                       const char *name,
                                       const char *unit,
                                       const char *description,
                                       double delta);

int hyperionMonitoringSetGauge(HyperionMonitoringCenter *center,
                               const char *name,
                               const char *unit,
                               const char *description,
                               double value);

int hyperionMonitoringObserveValue(HyperionMonitoringCenter *center,
                                   const char *name,
                                   const char *unit,
                                   const char *description,
                                   double value);

size_t hyperionMonitoringExport(const HyperionMonitoringCenter *center,
                                char *buffer,
                                size_t buffer_length);

typedef struct {
    HyperionMonitorMetricType type;
    double current;
    double sum;
    double min;
    double max;
    size_t samples;
} HyperionMonitorMetricSnapshot;

int hyperionMonitoringGetMetric(const HyperionMonitoringCenter *center,
                                const char *name,
                                HyperionMonitorMetricSnapshot *snapshot);

void hyperionMonitoringRecordLog(HyperionMonitoringCenter *center,
                                 const char *level,
                                 const char *message);

size_t hyperionMonitoringExportLogs(const HyperionMonitoringCenter *center,
                                    char *buffer,
                                    size_t buffer_length,
                                    size_t max_entries);

int hyperionMonitoringAddAlert(HyperionMonitoringCenter *center,
                               const char *metric_name,
                               const char *description,
                               double threshold,
                               int comparison,
                               size_t required_consecutive_hits,
                               HyperionMonitorAlertCallback callback,
                               void *user_data);

void hyperionMonitoringEvaluateAlerts(HyperionMonitoringCenter *center);

#ifdef __cplusplus
}
#endif

#endif
