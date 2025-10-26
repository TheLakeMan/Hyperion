#include "test_framework.h"
#include "../utils/auto_scaler.h"

HYPERION_TEST(test_autoscaler_scale_up_down)
{
    HyperionMonitoringCenter *center = hyperionMonitoringCreate(16);
    HYPERION_ASSERT(center != NULL, "Monitoring center creation failed");

    hyperionMonitoringSetGauge(center, "cpu.utilization", "percent", "CPU usage", 80.0);

    HyperionAutoScalerPolicy policy = {
        "cpu.utilization",
        70.0,
        30.0,
        2,
        2,
        10,
        0,
        0,
    };

    HyperionAutoScaler *scaler = hyperionAutoScalerCreate(&policy, center);
    HYPERION_ASSERT(scaler != NULL, "Autoscaler creation failed");

    HyperionAutoScaleDecision decision;
    size_t desired = 0;
    double metric = 0.0;
    char reason[128];

    HYPERION_ASSERT(hyperionAutoScalerPlan(scaler, 4, &decision, &desired, &metric, reason, sizeof(reason)),
                    "Autoscaler plan should succeed");
    HYPERION_ASSERT(decision == HYPERION_AUTOSCALE_DECISION_SCALE_UP, "Should recommend scaling up");
    HYPERION_ASSERT(desired == 6, "Should increase replicas by scale step");
    hyperionAutoScalerRecord(scaler, decision, desired);

    hyperionMonitoringSetGauge(center, "cpu.utilization", "percent", "CPU usage", 20.0);
    HYPERION_ASSERT(hyperionAutoScalerPlan(scaler, desired, &decision, &desired, &metric, reason, sizeof(reason)),
                    "Autoscaler plan after down metric should succeed");
    HYPERION_ASSERT(decision == HYPERION_AUTOSCALE_DECISION_SCALE_DOWN, "Should recommend scaling down");
    HYPERION_ASSERT(desired == 4, "Should reduce replicas by scale step");
    hyperionAutoScalerRecord(scaler, decision, desired);

    hyperionMonitoringSetGauge(center, "cpu.utilization", "percent", "CPU usage", 10.0);
    HYPERION_ASSERT(hyperionAutoScalerPlan(scaler, desired, &decision, &desired, &metric, reason, sizeof(reason)),
                    "Autoscaler plan to minimum should succeed");
    HYPERION_ASSERT(desired == 2, "Should not go below minimum replicas");
    hyperionAutoScalerRecord(scaler, decision, desired);

    hyperionAutoScalerDestroy(scaler);
    hyperionMonitoringDestroy(center);
    return 0;
}

const HyperionTestCase g_autoscaler_tests[] = {
    {"autoscaler_scale_up_down", "autoscaler", test_autoscaler_scale_up_down},
};

const size_t g_autoscaler_test_count = sizeof(g_autoscaler_tests) / sizeof(g_autoscaler_tests[0]);
