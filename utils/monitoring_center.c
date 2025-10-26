#include "monitoring_center.h"
#include "../core/memory.h"
#include "../core/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <strings.h>
#endif

#define HYPERION_MONITOR_MAX_METRICS      48
#define HYPERION_MONITOR_MAX_ALERTS       16
#define HYPERION_MONITOR_MAX_LOGS        256
#define HYPERION_MONITOR_SERIES_LENGTH    64

typedef struct {
    time_t timestamp;
    double value;
} HyperionMonitorSample;

typedef struct {
    HyperionMonitorMetricType type;
    char name[64];
    char unit[24];
    char description[128];
    double current;
    double sum;
    double min;
    double max;
    double last;
    size_t count;
    HyperionMonitorSample series[HYPERION_MONITOR_SERIES_LENGTH];
    size_t series_count;
    size_t series_index;
} HyperionMonitorMetric;

typedef struct {
    char metric_name[64];
    char description[128];
    double threshold;
    int comparison;
    size_t required_hits;
    size_t current_hits;
    HyperionMonitorAlertCallback callback;
    void *user_data;
} HyperionMonitorAlert;

typedef struct {
    time_t timestamp;
    char level[16];
    char message[192];
} HyperionMonitorLogEntry;

struct HyperionMonitoringCenter {
    HyperionMonitorMetric metrics[HYPERION_MONITOR_MAX_METRICS];
    size_t metric_count;

    HyperionMonitorAlert alerts[HYPERION_MONITOR_MAX_ALERTS];
    size_t alert_count;

    HyperionMonitorLogEntry logs[HYPERION_MONITOR_MAX_LOGS];
    size_t log_count;
    size_t log_index;

#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
};

static HyperionMonitoringCenter *g_monitoring_center = NULL;

static void monitor_lock(HyperionMonitoringCenter *center)
{
#ifdef _WIN32
    EnterCriticalSection(&center->lock);
#else
    pthread_mutex_lock(&center->lock);
#endif
}

static void monitor_unlock(HyperionMonitoringCenter *center)
{
#ifdef _WIN32
    LeaveCriticalSection(&center->lock);
#else
    pthread_mutex_unlock(&center->lock);
#endif
}

static void initialize_metric(HyperionMonitorMetric *metric,
                              HyperionMonitorMetricType type,
                              const char *name,
                              const char *unit,
                              const char *description)
{
    metric->type = type;
    strncpy(metric->name, name, sizeof(metric->name) - 1);
    metric->name[sizeof(metric->name) - 1] = '\0';
    if (unit) {
        strncpy(metric->unit, unit, sizeof(metric->unit) - 1);
        metric->unit[sizeof(metric->unit) - 1] = '\0';
    } else {
        metric->unit[0] = '\0';
    }
    if (description) {
        strncpy(metric->description, description, sizeof(metric->description) - 1);
        metric->description[sizeof(metric->description) - 1] = '\0';
    } else {
        metric->description[0] = '\0';
    }
    metric->current = 0.0;
    metric->sum = 0.0;
    metric->min = 0.0;
    metric->max = 0.0;
    metric->last = 0.0;
    metric->count = 0;
    metric->series_count = 0;
    metric->series_index = 0;
}

static HyperionMonitorMetric *find_metric(HyperionMonitoringCenter *center,
                                          const char *name)
{
    for (size_t i = 0; i < center->metric_count; ++i) {
        if (strcmp(center->metrics[i].name, name) == 0) {
            return &center->metrics[i];
        }
    }
    return NULL;
}

static HyperionMonitorMetric *ensure_metric(HyperionMonitoringCenter *center,
                                            const char *name,
                                            HyperionMonitorMetricType type,
                                            const char *unit,
                                            const char *description)
{
    HyperionMonitorMetric *metric = find_metric(center, name);
    if (metric) {
        if (metric->type != type) {
            return NULL;
        }
        if (unit && metric->unit[0] == '\0') {
            strncpy(metric->unit, unit, sizeof(metric->unit) - 1);
            metric->unit[sizeof(metric->unit) - 1] = '\0';
        }
        if (description && metric->description[0] == '\0') {
            strncpy(metric->description, description, sizeof(metric->description) - 1);
            metric->description[sizeof(metric->description) - 1] = '\0';
        }
        return metric;
    }

    if (center->metric_count >= HYPERION_MONITOR_MAX_METRICS) {
        return NULL;
    }

    metric = &center->metrics[center->metric_count++];
    initialize_metric(metric, type, name, unit, description);
    return metric;
}

static void add_sample(HyperionMonitorMetric *metric, double value)
{
    HyperionMonitorSample *sample = &metric->series[metric->series_index];
    sample->timestamp = time(NULL);
    sample->value = value;

    metric->series_index = (metric->series_index + 1) % HYPERION_MONITOR_SERIES_LENGTH;
    if (metric->series_count < HYPERION_MONITOR_SERIES_LENGTH) {
        metric->series_count++;
    }
}

static void copy_string(char *dest, size_t dest_len, const char *src)
{
    if (dest_len == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_len - 1);
    dest[dest_len - 1] = '\0';
}

HyperionMonitoringCenter *hyperionMonitoringCreate(size_t max_metrics)
{
    (void)max_metrics; /* Reserved for future use */
    HyperionMonitoringCenter *center = (HyperionMonitoringCenter *)HYPERION_CALLOC(1, sizeof(*center));
    if (!center) {
        return NULL;
    }

#ifdef _WIN32
    InitializeCriticalSection(&center->lock);
#else
    pthread_mutex_init(&center->lock, NULL);
#endif

    return center;
}

void hyperionMonitoringDestroy(HyperionMonitoringCenter *center)
{
    if (!center) return;
#ifdef _WIN32
    DeleteCriticalSection(&center->lock);
#else
    pthread_mutex_destroy(&center->lock);
#endif
    HYPERION_FREE(center);
}

void hyperionMonitoringReset(HyperionMonitoringCenter *center)
{
    if (!center) return;
    monitor_lock(center);
    memset(center->metrics, 0, sizeof(center->metrics));
    memset(center->alerts, 0, sizeof(center->alerts));
    memset(center->logs, 0, sizeof(center->logs));
    center->metric_count = 0;
    center->alert_count = 0;
    center->log_count = 0;
    center->log_index = 0;
    monitor_unlock(center);
}

HyperionMonitoringCenter *hyperionMonitoringInstance(void)
{
    if (!g_monitoring_center) {
        g_monitoring_center = hyperionMonitoringCreate(HYPERION_MONITOR_MAX_METRICS);
        if (!g_monitoring_center) {
            fprintf(stderr, "[monitoring] Failed to initialize monitoring center\n");
        }
    }
    return g_monitoring_center;
}

void hyperionMonitoringShutdown(void)
{
    if (g_monitoring_center) {
        hyperionMonitoringDestroy(g_monitoring_center);
        g_monitoring_center = NULL;
    }
}

static void update_stats(HyperionMonitorMetric *metric, double value)
{
    if (metric->count == 0) {
        metric->min = value;
        metric->max = value;
    } else {
        if (value < metric->min) metric->min = value;
        if (value > metric->max) metric->max = value;
    }
    metric->sum += value;
    metric->current = value;
    metric->last = value;
    metric->count++;
    add_sample(metric, value);
}

int hyperionMonitoringIncrementCounter(HyperionMonitoringCenter *center,
                                       const char *name,
                                       const char *unit,
                                       const char *description,
                                       double delta)
{
    if (!center || !name) return 0;
    monitor_lock(center);
    HyperionMonitorMetric *metric = ensure_metric(center, name, HYPERION_MONITOR_METRIC_COUNTER, unit, description);
    if (!metric) {
        monitor_unlock(center);
        return 0;
    }

    metric->current += delta;
    metric->sum += delta;
    if (metric->count == 0 || metric->current < metric->min) metric->min = metric->current;
    if (metric->count == 0 || metric->current > metric->max) metric->max = metric->current;
    metric->last = delta;
    metric->count++;
    add_sample(metric, metric->current);
    monitor_unlock(center);

    hyperionMonitoringEvaluateAlerts(center);
    return 1;
}

int hyperionMonitoringSetGauge(HyperionMonitoringCenter *center,
                               const char *name,
                               const char *unit,
                               const char *description,
                               double value)
{
    if (!center || !name) return 0;
    monitor_lock(center);
    HyperionMonitorMetric *metric = ensure_metric(center, name, HYPERION_MONITOR_METRIC_GAUGE, unit, description);
    if (!metric) {
        monitor_unlock(center);
        return 0;
    }
    update_stats(metric, value);
    monitor_unlock(center);

    hyperionMonitoringEvaluateAlerts(center);
    return 1;
}

int hyperionMonitoringObserveValue(HyperionMonitoringCenter *center,
                                   const char *name,
                                   const char *unit,
                                   const char *description,
                                   double value)
{
    if (!center || !name) return 0;
    monitor_lock(center);
    HyperionMonitorMetric *metric = ensure_metric(center, name, HYPERION_MONITOR_METRIC_HISTOGRAM, unit, description);
    if (!metric) {
        monitor_unlock(center);
        return 0;
    }
    update_stats(metric, value);
    monitor_unlock(center);

    hyperionMonitoringEvaluateAlerts(center);
    return 1;
}

static void append_json_string(char **cursor, size_t *remaining, const char *value)
{
    if (!cursor || !*cursor || !remaining || *remaining == 0) return;
    **cursor = '\0';
    if (!value) value = "";
    while (*value && *remaining > 1) {
        char ch = *value++;
        if (ch == '"' || ch == '\\') {
            if (*remaining <= 2) break;
            **cursor = '\\';
            (*cursor)++;
            (*remaining)--;
        }
        **cursor = ch;
        (*cursor)++;
        (*remaining)--;
    }
    **cursor = '\0';
}

size_t hyperionMonitoringExport(const HyperionMonitoringCenter *center,
                                char *buffer,
                                size_t buffer_length)
{
    if (!center || !buffer || buffer_length == 0) return 0;

    char *cursor = buffer;
    size_t remaining = buffer_length;
    *cursor++ = '{';
    remaining--;

    *cursor++ = '"'; remaining--;
    strcpy(cursor, "metrics"); cursor += strlen("metrics"); remaining -= strlen("metrics");
    *cursor++ = '"'; remaining--;
    *cursor++ = ':'; remaining--;
    *cursor++ = '['; remaining--;

    HyperionMonitoringCenter local_copy = {0};
    monitor_lock((HyperionMonitoringCenter *)center);
    local_copy.metric_count = center->metric_count;
    memcpy(local_copy.metrics, center->metrics, sizeof(center->metrics));
    local_copy.alert_count = center->alert_count;
    memcpy(local_copy.alerts, center->alerts, sizeof(center->alerts));
    monitor_unlock((HyperionMonitoringCenter *)center);

    for (size_t i = 0; i < local_copy.metric_count && remaining > 0; ++i) {
        const HyperionMonitorMetric *metric = &local_copy.metrics[i];
        if (i > 0) {
            *cursor++ = ',';
            remaining--;
        }
        int written = snprintf(cursor, remaining,
                               "{\"name\":\"");
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
        append_json_string(&cursor, &remaining, metric->name);
        written = snprintf(cursor, remaining,
                           "\",\"type\":%d,\"unit\":\"", (int)metric->type);
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
        append_json_string(&cursor, &remaining, metric->unit);
        written = snprintf(cursor, remaining,
                           "\",\"description\":\"");
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
        append_json_string(&cursor, &remaining, metric->description);
        written = snprintf(cursor, remaining,
                           "\",\"current\":%.6f,\"min\":%.6f,\"max\":%.6f,"
                           "\"avg\":%.6f,\"count\":%zu,\"last\":%.6f,\"series\":[",
                           metric->current,
                           metric->min,
                           metric->max,
                           metric->count ? metric->sum / (double)metric->count : 0.0,
                           metric->count,
                           metric->last);
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;

        for (size_t s = 0; s < metric->series_count && remaining > 0; ++s) {
            size_t index = (metric->series_index + HYPERION_MONITOR_SERIES_LENGTH - metric->series_count + s)
                           % HYPERION_MONITOR_SERIES_LENGTH;
            const HyperionMonitorSample *sample = &metric->series[index];
            if (s > 0) {
                *cursor++ = ',';
                remaining--;
            }
            written = snprintf(cursor, remaining, "{\"t\":%ld,\"v\":%.6f}",
                               (long)sample->timestamp, sample->value);
            if (written < 0 || (size_t)written >= remaining) break;
            cursor += written; remaining -= (size_t)written;
        }
        if (remaining > 0) {
            *cursor++ = ']';
            remaining--;
        }
        if (remaining > 0) {
            *cursor++ = '}';
            remaining--;
        }
    }

    if (remaining > 0) {
        *cursor++ = ']';
        remaining--;
    }

    if (remaining > 0) {
        *cursor++ = ',';
        remaining--;
    }

    int written = snprintf(cursor, remaining, "\"alerts\":[");
    if (written < 0 || (size_t)written >= remaining) return buffer_length;
    cursor += written; remaining -= (size_t)written;

    for (size_t i = 0; i < local_copy.alert_count && remaining > 0; ++i) {
        const HyperionMonitorAlert *alert = &local_copy.alerts[i];
        if (i > 0) {
            *cursor++ = ',';
            remaining--;
        }
        written = snprintf(cursor, remaining,
                           "{\"metric\":\"");
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
        append_json_string(&cursor, &remaining, alert->metric_name);
        written = snprintf(cursor, remaining,
                           "\",\"description\":\"");
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
        append_json_string(&cursor, &remaining, alert->description);
        written = snprintf(cursor, remaining,
                           "\",\"threshold\":%.6f,\"comparison\":%d,\"required_hits\":%zu,"
                           "\"current_hits\":%zu}",
                           alert->threshold,
                           alert->comparison,
                           alert->required_hits,
                           alert->current_hits);
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
    }

    if (remaining > 0) {
        *cursor++ = ']';
        remaining--;
    }

    if (remaining > 0) {
        *cursor++ = '}';
        remaining--;
    }

    if (remaining > 0) {
        *cursor = '\0';
    } else {
        buffer[buffer_length - 1] = '\0';
    }

    return buffer_length - remaining;
}

int hyperionMonitoringGetMetric(const HyperionMonitoringCenter *center,
                                const char *name,
                                HyperionMonitorMetricSnapshot *snapshot)
{
    if (!center || !name || !snapshot) return 0;

    HyperionMonitoringCenter *mutable_center = (HyperionMonitoringCenter *)center;
    monitor_lock(mutable_center);
    for (size_t i = 0; i < mutable_center->metric_count; ++i) {
        HyperionMonitorMetric *metric = &mutable_center->metrics[i];
        if (strcmp(metric->name, name) == 0) {
            snapshot->type = metric->type;
            snapshot->current = metric->current;
            snapshot->sum = metric->sum;
            snapshot->min = metric->count ? metric->min : metric->current;
            snapshot->max = metric->count ? metric->max : metric->current;
            snapshot->samples = metric->count;
            monitor_unlock(mutable_center);
            return 1;
        }
    }
    monitor_unlock(mutable_center);
    memset(snapshot, 0, sizeof(*snapshot));
    return 0;
}

void hyperionMonitoringRecordLog(HyperionMonitoringCenter *center,
                                 const char *level,
                                 const char *message)
{
    if (!center || !message) return;
    monitor_lock(center);
    HyperionMonitorLogEntry *entry = &center->logs[center->log_index];
    entry->timestamp = time(NULL);
    copy_string(entry->level, sizeof(entry->level), level ? level : "INFO");
    copy_string(entry->message, sizeof(entry->message), message);

    center->log_index = (center->log_index + 1) % HYPERION_MONITOR_MAX_LOGS;
    if (center->log_count < HYPERION_MONITOR_MAX_LOGS) {
        center->log_count++;
    }
    monitor_unlock(center);
}

size_t hyperionMonitoringExportLogs(const HyperionMonitoringCenter *center,
                                    char *buffer,
                                    size_t buffer_length,
                                    size_t max_entries)
{
    if (!center || !buffer || buffer_length == 0) return 0;
    HyperionMonitoringCenter local_copy = {0};

    monitor_lock((HyperionMonitoringCenter *)center);
    local_copy.log_count = center->log_count;
    local_copy.log_index = center->log_index;
    memcpy(local_copy.logs, center->logs, sizeof(center->logs));
    monitor_unlock((HyperionMonitoringCenter *)center);

    if (max_entries == 0 || max_entries > local_copy.log_count) {
        max_entries = local_copy.log_count;
    }

    char *cursor = buffer;
    size_t remaining = buffer_length;
    int written = snprintf(cursor, remaining, "[");
    if (written < 0 || (size_t)written >= remaining) return buffer_length;
    cursor += written; remaining -= (size_t)written;

    for (size_t i = 0; i < max_entries && remaining > 0; ++i) {
        if (i > 0) {
            *cursor++ = ',';
            remaining--;
        }
        size_t index = (local_copy.log_index + HYPERION_MONITOR_MAX_LOGS - max_entries + i)
                       % HYPERION_MONITOR_MAX_LOGS;
        const HyperionMonitorLogEntry *entry = &local_copy.logs[index];
        written = snprintf(cursor, remaining,
                           "{\"timestamp\":%ld,\"level\":\"%s\",\"message\":\"",
                           (long)entry->timestamp,
                           entry->level);
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
        append_json_string(&cursor, &remaining, entry->message);
        written = snprintf(cursor, remaining, "\"}");
        if (written < 0 || (size_t)written >= remaining) break;
        cursor += written; remaining -= (size_t)written;
    }

    if (remaining > 0) {
        *cursor++ = ']';
        remaining--;
        if (remaining > 0) *cursor = '\0';
    }

    return buffer_length - remaining;
}

int hyperionMonitoringAddAlert(HyperionMonitoringCenter *center,
                               const char *metric_name,
                               const char *description,
                               double threshold,
                               int comparison,
                               size_t required_consecutive_hits,
                               HyperionMonitorAlertCallback callback,
                               void *user_data)
{
    if (!center || !metric_name || !callback) return 0;
    monitor_lock(center);
    if (center->alert_count >= HYPERION_MONITOR_MAX_ALERTS) {
        monitor_unlock(center);
        return 0;
    }
    HyperionMonitorAlert *alert = &center->alerts[center->alert_count++];
    copy_string(alert->metric_name, sizeof(alert->metric_name), metric_name);
    copy_string(alert->description, sizeof(alert->description), description);
    alert->threshold = threshold;
    alert->comparison = comparison;
    alert->required_hits = required_consecutive_hits > 0 ? required_consecutive_hits : 1;
    alert->current_hits = 0;
    alert->callback = callback;
    alert->user_data = user_data;
    monitor_unlock(center);
    return 1;
}

static int alert_comparison_met(int comparison, double value, double threshold)
{
    switch (comparison) {
        case HYPERION_MONITOR_COMPARE_GREATER:
            return value > threshold;
        case HYPERION_MONITOR_COMPARE_LESS:
            return value < threshold;
        case HYPERION_MONITOR_COMPARE_EQUAL:
            return fabs(value - threshold) < 1e-6;
        default:
            return 0;
    }
}

void hyperionMonitoringEvaluateAlerts(HyperionMonitoringCenter *center)
{
    if (!center) return;
    monitor_lock(center);
    for (size_t i = 0; i < center->alert_count; ++i) {
        HyperionMonitorAlert *alert = &center->alerts[i];
        HyperionMonitorMetric *metric = find_metric(center, alert->metric_name);
        if (!metric) {
            alert->current_hits = 0;
            continue;
        }
        double value = metric->current;
        if (alert_comparison_met(alert->comparison, value, alert->threshold)) {
            alert->current_hits++;
            if (alert->current_hits >= alert->required_hits) {
                HyperionMonitorAlertCallback cb = alert->callback;
                void *user_data = alert->user_data;
                monitor_unlock(center);
                cb(alert->metric_name, value, user_data);
                monitor_lock(center);
                alert->current_hits = 0;
            }
        } else {
            alert->current_hits = 0;
        }
    }
    monitor_unlock(center);
}
