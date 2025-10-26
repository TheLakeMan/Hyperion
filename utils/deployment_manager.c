#include "deployment_manager.h"
#include "../core/memory.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <strings.h>
#endif

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

struct HyperionDeploymentManager {
    HyperionDeploymentHistoryEntry *history;
    size_t history_count;
    size_t history_capacity;
    HyperionDeploymentConfig active_config;
    int has_active;
    size_t rollback_count;
};

static void trim(char *value)
{
    if (!value) return;
    size_t len = strlen(value);
    size_t start = 0;
    while (start < len && isspace((unsigned char)value[start])) start++;
    size_t end = len;
    while (end > start && isspace((unsigned char)value[end - 1])) end--;
    if (start > 0 || end < len) {
        memmove(value, value + start, end - start);
        value[end - start] = '\0';
    }
}

static int parse_bool(const char *value)
{
    if (!value) return 0;
    if (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcasecmp(value, "yes") == 0) {
        return 1;
    }
    return 0;
}

static int parse_int(const char *value, int default_value)
{
    if (!value || !*value) return default_value;
    return (int)strtol(value, NULL, 10);
}

static double parse_double(const char *value, double default_value)
{
    if (!value || !*value) return default_value;
    return strtod(value, NULL);
}

static void reset_config(HyperionDeploymentConfig *config)
{
    if (!config) return;
    memset(config, 0, sizeof(*config));
    config->desired_replicas = 1;
    config->health_interval_seconds = 30;
    config->health_initial_delay_seconds = 10;
    config->max_parallel = 1;
}

static void push_history(HyperionDeploymentManager *manager, const HyperionDeploymentConfig *config,
                         HyperionDeploymentState state, const char *notes)
{
    if (!manager || !config) return;
    if (manager->history_capacity == 0) return;

    if (manager->history_count == manager->history_capacity) {
        memmove(&manager->history[0], &manager->history[1],
                (manager->history_capacity - 1) * sizeof(HyperionDeploymentHistoryEntry));
        manager->history_count = manager->history_capacity - 1;
    }

    HyperionDeploymentHistoryEntry *entry = &manager->history[manager->history_count++];
    entry->state = state;
    entry->config = *config;
    entry->timestamp = time(NULL);
    entry->notes[0] = '\0';
    if (notes && *notes) {
        strncpy(entry->notes, notes, sizeof(entry->notes) - 1);
        entry->notes[sizeof(entry->notes) - 1] = '\0';
    }
    if (state == HYPERION_DEPLOYMENT_STATE_ROLLED_BACK) {
        manager->rollback_count++;
    }
}

HyperionDeploymentManager *hyperionDeploymentManagerCreate(size_t history_capacity)
{
    if (history_capacity == 0) history_capacity = 16;
    HyperionDeploymentManager *manager = (HyperionDeploymentManager *)HYPERION_CALLOC(1, sizeof(*manager));
    if (!manager) return NULL;

    manager->history = (HyperionDeploymentHistoryEntry *)HYPERION_CALLOC(history_capacity,
                                                                         sizeof(HyperionDeploymentHistoryEntry));
    if (!manager->history) {
        HYPERION_FREE(manager);
        return NULL;
    }

    manager->history_capacity = history_capacity;
    reset_config(&manager->active_config);
    return manager;
}

void hyperionDeploymentManagerDestroy(HyperionDeploymentManager *manager)
{
    if (!manager) return;
    if (manager->history) {
        HYPERION_FREE(manager->history);
    }
    HYPERION_FREE(manager);
}

void hyperionDeploymentManagerReset(HyperionDeploymentManager *manager)
{
    if (!manager) return;
    manager->history_count = 0;
    manager->rollback_count = 0;
    manager->has_active = 0;
    reset_config(&manager->active_config);
}

static int parse_line(HyperionDeploymentConfig *config, const char *key, const char *value)
{
    if (strcasecmp(key, "environment") == 0) {
        strncpy(config->environment, value, sizeof(config->environment) - 1);
        config->environment[sizeof(config->environment) - 1] = '\0';
        return 1;
    }
    if (strcasecmp(key, "version") == 0) {
        strncpy(config->version, value, sizeof(config->version) - 1);
        config->version[sizeof(config->version) - 1] = '\0';
        return 1;
    }
    if (strcasecmp(key, "artifact") == 0 || strcasecmp(key, "artifact_path") == 0) {
        strncpy(config->artifact_path, value, sizeof(config->artifact_path) - 1);
        config->artifact_path[sizeof(config->artifact_path) - 1] = '\0';
        return 1;
    }
    if (strcasecmp(key, "cluster") == 0) {
        strncpy(config->cluster, value, sizeof(config->cluster) - 1);
        config->cluster[sizeof(config->cluster) - 1] = '\0';
        return 1;
    }
    if (strcasecmp(key, "replicas") == 0 || strcasecmp(key, "desired_replicas") == 0) {
        config->desired_replicas = parse_int(value, config->desired_replicas);
        return 1;
    }
    if (strcasecmp(key, "enable_canary") == 0) {
        config->enable_canary = parse_bool(value);
        return 1;
    }
    if (strcasecmp(key, "canary_percent") == 0 || strcasecmp(key, "canary_traffic_percent") == 0) {
        config->canary_traffic_percent = parse_double(value, config->canary_traffic_percent);
        return 1;
    }
    if (strcasecmp(key, "health_initial_delay_seconds") == 0) {
        config->health_initial_delay_seconds = parse_int(value, config->health_initial_delay_seconds);
        return 1;
    }
    if (strcasecmp(key, "health_interval_seconds") == 0) {
        config->health_interval_seconds = parse_int(value, config->health_interval_seconds);
        return 1;
    }
    if (strcasecmp(key, "max_parallel") == 0) {
        config->max_parallel = parse_int(value, config->max_parallel);
        return 1;
    }
    return 0;
}

int hyperionDeploymentLoadConfig(HyperionDeploymentConfig *config, const char *path)
{
    if (!config || !path) return 0;
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;

    reset_config(config);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *value = eq + 1;
        trim(key);
        trim(value);
        parse_line(config, key, value);
    }

    fclose(fp);
    return 1;
}

int hyperionDeploymentValidate(const HyperionDeploymentConfig *config, char *error_buffer, size_t error_len)
{
    if (!config) return 0;
    if (!config->environment[0]) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Environment is required");
        return 0;
    }
    if (!config->version[0]) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Version is required");
        return 0;
    }
    if (!config->artifact_path[0]) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Artifact path is required");
        return 0;
    }
    if (!config->cluster[0]) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Cluster is required");
        return 0;
    }
    if (config->desired_replicas <= 0) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Replicas must be positive");
        return 0;
    }
    if (config->enable_canary && (config->canary_traffic_percent <= 0.0 || config->canary_traffic_percent >= 100.0)) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Canary percent must be between 0 and 100");
        return 0;
    }
    if (config->health_interval_seconds <= 0) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Health interval must be positive");
        return 0;
    }
    if (config->health_initial_delay_seconds < 0) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "Initial delay cannot be negative");
        return 0;
    }
    if (config->max_parallel <= 0) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "max_parallel must be positive");
        return 0;
    }
    return 1;
}

size_t hyperionDeploymentGeneratePlan(const HyperionDeploymentConfig *config, char *buffer, size_t buffer_len)
{
    if (!config || !buffer || buffer_len == 0) return 0;
    size_t written = 0;
    written += (size_t)snprintf(buffer + written, buffer_len - written,
                                "Deployment Plan for %s (%s)\n",
                                config->environment[0] ? config->environment : "unknown",
                                config->version[0] ? config->version : "unversioned");
    if (written >= buffer_len) return buffer_len - 1;

    written += (size_t)snprintf(buffer + written, buffer_len - written,
                                "1. Validate artifact at %s\n",
                                config->artifact_path[0] ? config->artifact_path : "<missing>");
    if (written >= buffer_len) return buffer_len - 1;

    written += (size_t)snprintf(buffer + written, buffer_len - written,
                                "2. Provision cluster %s with %d replicas\n",
                                config->cluster[0] ? config->cluster : "<unknown>",
                                config->desired_replicas);
    if (written >= buffer_len) return buffer_len - 1;

    if (config->enable_canary) {
        written += (size_t)snprintf(buffer + written, buffer_len - written,
                                    "3. Route %.1f%% traffic to canary deployment\n",
                                    config->canary_traffic_percent);
    } else {
        written += (size_t)snprintf(buffer + written, buffer_len - written,
                                    "3. Perform rolling update with max parallel %d\n",
                                    config->max_parallel);
    }
    if (written >= buffer_len) return buffer_len - 1;

    written += (size_t)snprintf(buffer + written, buffer_len - written,
                                "4. Monitor health checks every %d seconds (initial delay %d seconds)\n",
                                config->health_interval_seconds,
                                config->health_initial_delay_seconds);
    if (written >= buffer_len) return buffer_len - 1;

    written += (size_t)snprintf(buffer + written, buffer_len - written,
                                "5. Finalize deployment and update service registry\n");
    if (written >= buffer_len) return buffer_len - 1;

    return written;
}

int hyperionDeploymentApply(HyperionDeploymentManager *manager, const HyperionDeploymentConfig *config,
                            const char *notes, char *error_buffer, size_t error_len)
{
    if (!manager || !config) return 0;
    char validate_error[128];
    if (!hyperionDeploymentValidate(config, validate_error, sizeof(validate_error))) {
        if (error_buffer && error_len) snprintf(error_buffer, error_len, "%s", validate_error);
        return 0;
    }

    push_history(manager, config, HYPERION_DEPLOYMENT_STATE_SUCCEEDED, notes);
    manager->active_config = *config;
    manager->has_active = 1;
    return 1;
}

int hyperionDeploymentRollback(HyperionDeploymentManager *manager, const char *target_version,
                               char *message_buffer, size_t message_len)
{
    if (!manager || manager->history_count == 0) {
        if (message_buffer && message_len) snprintf(message_buffer, message_len, "No deployment history");
        return 0;
    }

    const HyperionDeploymentHistoryEntry *target_entry = NULL;
    for (size_t i = manager->history_count; i-- > 0;) {
        const HyperionDeploymentHistoryEntry *entry = &manager->history[i];
        if (entry->state == HYPERION_DEPLOYMENT_STATE_SUCCEEDED) {
            if (!target_version || target_version[0] == '\0' ||
                strcasecmp(entry->config.version, target_version) == 0) {
                target_entry = entry;
                break;
            }
        }
    }

    if (!target_entry) {
        if (message_buffer && message_len)
            snprintf(message_buffer, message_len, "Deployment version not found");
        return 0;
    }

    manager->active_config = target_entry->config;
    manager->has_active = 1;

    char note[160];
    snprintf(note, sizeof(note), "Rolled back to version %s", target_entry->config.version);
    push_history(manager, &target_entry->config, HYPERION_DEPLOYMENT_STATE_ROLLED_BACK, note);

    if (message_buffer && message_len) snprintf(message_buffer, message_len, "%s", note);
    return 1;
}

size_t hyperionDeploymentCopyHistory(const HyperionDeploymentManager *manager,
                                     HyperionDeploymentHistoryEntry *entries, size_t max_entries)
{
    if (!manager || !entries || max_entries == 0) return 0;
    size_t count = manager->history_count;
    if (count > max_entries) count = max_entries;
    for (size_t i = 0; i < count; ++i) {
        entries[i] = manager->history[manager->history_count - 1 - i];
    }
    return count;
}

int hyperionDeploymentGetStatus(const HyperionDeploymentManager *manager, HyperionDeploymentStatus *status)
{
    if (!manager || !status) return 0;

    memset(status, 0, sizeof(*status));
    status->has_active = manager->has_active;
    if (manager->has_active) {
        status->active_config = manager->active_config;
    }

    size_t successes = 0;
    size_t attempts = 0;
    status->rollback_count = manager->rollback_count;

    if (manager->history_count > 0) {
        const HyperionDeploymentHistoryEntry *latest = &manager->history[manager->history_count - 1];
        status->last_state = latest->state;
        status->last_timestamp = latest->timestamp;
    } else {
        status->last_state = HYPERION_DEPLOYMENT_STATE_PENDING;
        status->last_timestamp = 0;
    }

    for (size_t i = 0; i < manager->history_count; ++i) {
        HyperionDeploymentState state = manager->history[i].state;
        if (state == HYPERION_DEPLOYMENT_STATE_SUCCEEDED) {
            successes++;
            attempts++;
        } else if (state == HYPERION_DEPLOYMENT_STATE_FAILED) {
            attempts++;
        }
    }

    status->total_deployments = attempts;
    status->success_rate = (attempts > 0) ? ((double)successes / (double)attempts) : 0.0;
    return 1;
}

void hyperionDeploymentEvaluateHealth(const HyperionDeploymentManager *manager, HyperionDeploymentHealth *health)
{
    if (!health) return;
    memset(health, 0, sizeof(*health));
    if (!manager) return;

    HyperionDeploymentStatus status;
    if (!hyperionDeploymentGetStatus(manager, &status)) return;

    health->ready = status.has_active && status.last_state == HYPERION_DEPLOYMENT_STATE_SUCCEEDED;
    health->last_state = status.last_state;
    health->success_rate = status.success_rate;
    health->rollback_count = status.rollback_count;
    health->active_replicas = status.has_active ? (size_t)status.active_config.desired_replicas : 0;
    if (!status.has_active && status.last_state == HYPERION_DEPLOYMENT_STATE_PENDING) {
        health->ready = 0;
    }
}

const char *hyperionDeploymentStateName(HyperionDeploymentState state)
{
    switch (state) {
        case HYPERION_DEPLOYMENT_STATE_PENDING: return "pending";
        case HYPERION_DEPLOYMENT_STATE_SUCCEEDED: return "succeeded";
        case HYPERION_DEPLOYMENT_STATE_FAILED: return "failed";
        case HYPERION_DEPLOYMENT_STATE_ROLLED_BACK: return "rolled_back";
        default: return "unknown";
    }
}
