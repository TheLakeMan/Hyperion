#include "test_framework.h"
#include "../utils/monitoring_center.h"

#include <string.h>

static int g_alert_triggered = 0;
static char g_alert_metric[64];

static void test_alert_callback(const char *metric, double value, void *user_data)
{
    (void)value;
    (void)user_data;
    g_alert_triggered++;
    strncpy(g_alert_metric, metric, sizeof(g_alert_metric) - 1);
    g_alert_metric[sizeof(g_alert_metric) - 1] = '\0';
}

HYPERION_TEST(test_monitoring_metrics_and_export)
{
    HyperionMonitoringCenter *center = hyperionMonitoringCreate(16);
    HYPERION_ASSERT(center != NULL, "Monitoring center should initialize");

    HYPERION_ASSERT(hyperionMonitoringIncrementCounter(center, "http.requests", "count", "Total HTTP requests", 5.0),
                    "Counter increment should succeed");
    HYPERION_ASSERT(hyperionMonitoringSetGauge(center, "cpu.utilization", "percent", "CPU usage", 42.0),
                    "Gauge set should succeed");
    HYPERION_ASSERT(hyperionMonitoringObserveValue(center, "latency", "ms", "Request latency", 12.5),
                    "Histogram observation should succeed");

    char buffer[512];
    size_t written = hyperionMonitoringExport(center, buffer, sizeof(buffer));
    HYPERION_ASSERT(written > 0, "Export should write data");
    HYPERION_ASSERT(strstr(buffer, "http.requests") != NULL, "Export should include counter");
    HYPERION_ASSERT(strstr(buffer, "cpu.utilization") != NULL, "Export should include gauge");
    HYPERION_ASSERT(strstr(buffer, "latency") != NULL, "Export should include histogram");

    HyperionMonitorMetricSnapshot snapshot;
    HYPERION_ASSERT(hyperionMonitoringGetMetric(center, "cpu.utilization", &snapshot),
                    "Should retrieve gauge metric snapshot");
    HYPERION_ASSERT(snapshot.type == HYPERION_MONITOR_METRIC_GAUGE, "Snapshot type should match");
    HYPERION_ASSERT(snapshot.current == 42.0, "Snapshot current value should match gauge");
    HYPERION_ASSERT(snapshot.samples > 0, "Snapshot should report samples");

    hyperionMonitoringDestroy(center);
    return 0;
}

HYPERION_TEST(test_monitoring_alerts_and_logs)
{
    HyperionMonitoringCenter *center = hyperionMonitoringCreate(16);
    HYPERION_ASSERT(center != NULL, "Monitoring center should initialize");

    g_alert_triggered = 0;
    g_alert_metric[0] = '\0';
    HYPERION_ASSERT(hyperionMonitoringAddAlert(center, "error.rate", "Error rate alert", 10.0,
                                               HYPERION_MONITOR_COMPARE_GREATER, 2,
                                               test_alert_callback, NULL),
                    "Alert registration should succeed");

    hyperionMonitoringRecordLog(center, "INFO", "Starting monitoring test");
    hyperionMonitoringIncrementCounter(center, "error.rate", "count", "Errors per minute", 8.0);
    hyperionMonitoringEvaluateAlerts(center);
    HYPERION_ASSERT(g_alert_triggered == 0, "Alert should not fire yet");

    hyperionMonitoringIncrementCounter(center, "error.rate", "count", "Errors per minute", 5.0);
    hyperionMonitoringEvaluateAlerts(center);
    HYPERION_ASSERT(g_alert_triggered == 1, "Alert should fire after threshold exceeded twice");
    HYPERION_ASSERT(strcmp(g_alert_metric, "error.rate") == 0, "Alert metric name should match");

    char log_buffer[256];
    size_t logs_written = hyperionMonitoringExportLogs(center, log_buffer, sizeof(log_buffer), 10);
    HYPERION_ASSERT(logs_written > 0, "Log export should have data");
    HYPERION_ASSERT(strstr(log_buffer, "Starting monitoring test") != NULL, "Log export should include message");

    hyperionMonitoringDestroy(center);
    return 0;
}

const HyperionTestCase g_monitoring_tests[] = {
    {"monitoring_metrics_export", "monitoring", test_monitoring_metrics_and_export},
    {"monitoring_alerts_logs", "monitoring", test_monitoring_alerts_and_logs},
};

const size_t g_monitoring_test_count = sizeof(g_monitoring_tests) / sizeof(g_monitoring_tests[0]);
