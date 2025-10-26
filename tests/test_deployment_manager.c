#include "test_framework.h"
#include "../utils/deployment_manager.h"

#include <stdio.h>
#include <string.h>

static void write_temp_config(const char *path)
{
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    fputs("environment=staging\n", fp);
    fputs("version=1.2.3\n", fp);
    fputs("artifact=gs://hyperion/models/model.tar.gz\n", fp);
    fputs("cluster=staging-cluster\n", fp);
    fputs("replicas=4\n", fp);
    fputs("enable_canary=true\n", fp);
    fputs("canary_percent=15\n", fp);
    fputs("health_initial_delay_seconds=5\n", fp);
    fputs("health_interval_seconds=20\n", fp);
    fclose(fp);
}

HYPERION_TEST(test_deployment_config_load)
{
    const char *path = "test_deployment_config.tmp";
    write_temp_config(path);

    HyperionDeploymentConfig config;
    HYPERION_ASSERT(hyperionDeploymentLoadConfig(&config, path), "Config load should succeed");
    remove(path);

    HYPERION_ASSERT(strcmp(config.environment, "staging") == 0, "Environment should match");
    HYPERION_ASSERT(strcmp(config.version, "1.2.3") == 0, "Version should match");
    HYPERION_ASSERT(strcmp(config.cluster, "staging-cluster") == 0, "Cluster should match");
    HYPERION_ASSERT(config.desired_replicas == 4, "Replica count should parse");
    HYPERION_ASSERT(config.enable_canary == 1, "Canary flag should parse");
    HYPERION_ASSERT(config.canary_traffic_percent == 15.0, "Canary percent should parse");
    HYPERION_ASSERT(config.health_interval_seconds == 20, "Health interval should parse");
    return 0;
}

static void fill_config(HyperionDeploymentConfig *config, const char *environment, const char *version,
                        int replicas)
{
    memset(config, 0, sizeof(*config));
    strncpy(config->environment, environment, sizeof(config->environment) - 1);
    strncpy(config->version, version, sizeof(config->version) - 1);
    strncpy(config->artifact_path, "/opt/hyperion/model.tar.gz", sizeof(config->artifact_path) - 1);
    strncpy(config->cluster, "prod-cluster", sizeof(config->cluster) - 1);
    config->desired_replicas = replicas;
    config->enable_canary = 0;
    config->canary_traffic_percent = 0.0;
    config->health_initial_delay_seconds = 5;
    config->health_interval_seconds = 15;
    config->max_parallel = 2;
}

HYPERION_TEST(test_deployment_apply_and_rollback)
{
    HyperionDeploymentManager *manager = hyperionDeploymentManagerCreate(8);
    HYPERION_ASSERT(manager != NULL, "Manager creation should succeed");

    HyperionDeploymentConfig configA;
    fill_config(&configA, "production", "1.0.0", 6);
    HYPERION_ASSERT(hyperionDeploymentApply(manager, &configA, "Initial deployment", NULL, 0),
                    "Apply should succeed");

    HyperionDeploymentStatus status;
    HYPERION_ASSERT(hyperionDeploymentGetStatus(manager, &status), "Status should be available");
    HYPERION_ASSERT(status.has_active == 1, "Active deployment expected");
    HYPERION_ASSERT(strcmp(status.active_config.version, "1.0.0") == 0, "Version should match active");
    HYPERION_ASSERT(status.total_deployments == 1, "Should record one deployment");

    HyperionDeploymentConfig configB;
    fill_config(&configB, "production", "1.1.0", 8);
    configB.enable_canary = 1;
    configB.canary_traffic_percent = 20.0;
    HYPERION_ASSERT(hyperionDeploymentApply(manager, &configB, "Rolling update", NULL, 0),
                    "Second apply should succeed");

    HyperionDeploymentHistoryEntry history[4];
    size_t copied = hyperionDeploymentCopyHistory(manager, history, 4);
    HYPERION_ASSERT(copied >= 2, "History should contain deployments");
    HYPERION_ASSERT(strcmp(history[0].config.version, "1.1.0") == 0, "Latest history entry should match");

    char rollback_message[128];
    HYPERION_ASSERT(hyperionDeploymentRollback(manager, "1.0.0", rollback_message,
                                               sizeof(rollback_message)), "Rollback should succeed");

    HYPERION_ASSERT(hyperionDeploymentGetStatus(manager, &status), "Status after rollback should succeed");
    HYPERION_ASSERT(strcmp(status.active_config.version, "1.0.0") == 0,
                    "Active config should revert to previous version");
    HYPERION_ASSERT(status.rollback_count >= 1, "Rollback count should increment");

    hyperionDeploymentManagerDestroy(manager);
    return 0;
}

HYPERION_TEST(test_deployment_plan_generation)
{
    HyperionDeploymentConfig config;
    fill_config(&config, "staging", "2.0.0", 5);
    config.enable_canary = 1;
    config.canary_traffic_percent = 25.0;

    char buffer[512];
    size_t written = hyperionDeploymentGeneratePlan(&config, buffer, sizeof(buffer));
    HYPERION_ASSERT(written > 0, "Plan generation should produce output");
    HYPERION_ASSERT(strstr(buffer, "Deployment Plan for staging") != NULL, "Plan should include environment");
    HYPERION_ASSERT(strstr(buffer, "Provision cluster") != NULL, "Plan should include provisioning step");
    HYPERION_ASSERT(strstr(buffer, "Route 25.0% traffic") != NULL, "Plan should mention canary traffic");
    return 0;
}

const HyperionTestCase g_deployment_tests[] = {
    {"deployment_config_load", "deployment", test_deployment_config_load},
    {"deployment_apply_rollback", "deployment", test_deployment_apply_and_rollback},
    {"deployment_plan_generation", "deployment", test_deployment_plan_generation},
};

const size_t g_deployment_test_count = sizeof(g_deployment_tests) / sizeof(g_deployment_tests[0]);
