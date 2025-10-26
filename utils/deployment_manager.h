#ifndef HYPERION_DEPLOYMENT_MANAGER_H
#define HYPERION_DEPLOYMENT_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HYPERION_DEPLOYMENT_STATE_PENDING = 0,
    HYPERION_DEPLOYMENT_STATE_SUCCEEDED,
    HYPERION_DEPLOYMENT_STATE_FAILED,
    HYPERION_DEPLOYMENT_STATE_ROLLED_BACK
} HyperionDeploymentState;

typedef struct {
    char environment[64];
    char version[64];
    char artifact_path[160];
    char cluster[64];
    int  desired_replicas;
    int  enable_canary;
    double canary_traffic_percent;
    int  health_initial_delay_seconds;
    int  health_interval_seconds;
    int  max_parallel;
} HyperionDeploymentConfig;

typedef struct {
    HyperionDeploymentState state;
    HyperionDeploymentConfig config;
    time_t timestamp;
    char notes[160];
} HyperionDeploymentHistoryEntry;

typedef struct {
    int   has_active;
    HyperionDeploymentConfig active_config;
    HyperionDeploymentState  last_state;
    time_t last_timestamp;
    double success_rate;
    size_t total_deployments;
    size_t rollback_count;
} HyperionDeploymentStatus;

typedef struct {
    int ready;
    HyperionDeploymentState last_state;
    double success_rate;
    size_t active_replicas;
    size_t rollback_count;
} HyperionDeploymentHealth;

typedef struct HyperionDeploymentManager HyperionDeploymentManager;

HyperionDeploymentManager *hyperionDeploymentManagerCreate(size_t history_capacity);
void hyperionDeploymentManagerDestroy(HyperionDeploymentManager *manager);
void hyperionDeploymentManagerReset(HyperionDeploymentManager *manager);

int hyperionDeploymentLoadConfig(HyperionDeploymentConfig *config, const char *path);
int hyperionDeploymentValidate(const HyperionDeploymentConfig *config, char *error_buffer, size_t error_len);

size_t hyperionDeploymentGeneratePlan(const HyperionDeploymentConfig *config, char *buffer, size_t buffer_len);

int hyperionDeploymentApply(HyperionDeploymentManager *manager, const HyperionDeploymentConfig *config,
                            const char *notes, char *error_buffer, size_t error_len);

int hyperionDeploymentRollback(HyperionDeploymentManager *manager, const char *target_version,
                               char *message_buffer, size_t message_len);

size_t hyperionDeploymentCopyHistory(const HyperionDeploymentManager *manager,
                                     HyperionDeploymentHistoryEntry *entries, size_t max_entries);

int hyperionDeploymentGetStatus(const HyperionDeploymentManager *manager, HyperionDeploymentStatus *status);
void hyperionDeploymentEvaluateHealth(const HyperionDeploymentManager *manager, HyperionDeploymentHealth *health);

const char *hyperionDeploymentStateName(HyperionDeploymentState state);

#ifdef __cplusplus
}
#endif

#endif
