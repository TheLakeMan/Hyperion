/**
 * @file progressive_loader.c
 * @brief Implementation of progressive model loading utilities for Hyperion
 */

#include "progressive_loader.h"
#include "../core/memory.h"
#include "mmap_loader.h"
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Layer information structure
 */
typedef struct {
    int              index;            /* Layer index */
    HyperionLayerState state;            /* Current state */
    void            *weights;          /* Pointer to weights if loaded */
    size_t           size;             /* Size in bytes */
    int              precision;        /* Precision of weights (bits) */
    uint64_t         last_access_time; /* Last access timestamp */
    uint64_t         access_count;     /* Number of times accessed */
    float            custom_priority;  /* Custom priority (if used) */
    int             *dependencies;     /* Array of layer indices this depends on */
    int              dependency_count; /* Number of dependencies */
    int             *dependents;       /* Array of layer indices that depend on this */
    int              dependent_count;  /* Number of dependents */
    double           avg_load_time;    /* Average time to load in milliseconds */
    int              load_count;       /* Number of times loaded */
} HyperionLayerInfo;

/**
 * Progressive loader structure
 */
struct HyperionProgressiveLoader {
    HyperionMappedModel            *mapped_model;        /* Underlying memory-mapped model */
    bool                          owns_mapped_model;   /* Whether we own the mapped model */
    HyperionProgressiveLoaderConfig config;              /* Configuration */
    HyperionLayerInfo              *layers;              /* Array of layer information */
    int                           layer_count;         /* Number of layers */
    size_t                        current_memory;      /* Current memory usage */
    size_t                        peak_memory;         /* Peak memory usage */
    size_t                        total_model_size;    /* Total model size in bytes */
    uint64_t                      access_counter;      /* Counter for tracking access order */
    uint64_t                     *access_history;      /* Circular buffer of recent accesses */
    int                           history_size;        /* Size of the access history buffer */
    int                           history_pos;         /* Current position in history buffer */
    bool                          track_usage;         /* Whether to track usage patterns */
    clock_t                       last_timestamp;      /* Last operation timestamp */
    HyperionMemoryStats             stats;               /* Memory statistics */
    HyperionLayerState             *layer_states;        /* Array of layer states */
    void                         *layer_weights;       /* Array of layer weights */
    size_t                       *layer_sizes;         /* Array of layer sizes */
    float                        *layer_priorities;    /* Array of layer priorities */
    int                          *layer_access_counts; /* Array of layer access counts */
    bool                         *layer_dependencies;  /* Array of layer dependencies */
};

/**
 * Default configuration
 */
static const HyperionProgressiveConfig DEFAULT_CONFIG = {.max_memory     = 1024 * 1024 * 1024, // 1GB
                                                       .min_memory     = 128 * 1024 * 1024, // 128MB
                                                       .load_threshold = 768 * 1024 * 1024, // 768MB
                                                       .unload_threshold =
                                                           896 * 1024 * 1024,     // 896MB
                                                       .priority_window   = 1000, // 1 second
                                                       .enable_prefetch   = true,
                                                       .prefetch_distance = 2};

/**
 * Get current timestamp in milliseconds
 */
static uint64_t get_timestamp_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

/**
 * Create a default progressive loader configuration
 */
HyperionProgressiveLoaderConfig hyperionCreateDefaultProgressiveLoaderConfig(void)
{
    HyperionProgressiveLoaderConfig config;

    config.max_memory_budget          = 1024 * 1024 * 1024; /* 1GB default */
    config.enable_layer_unloading     = true;
    config.priority_strategy          = HYPERION_PRIORITY_LRU;
    config.prefetch_threshold         = 0.7f;
    config.max_prefetch_layers        = 2;
    config.enable_compression         = false;
    config.enable_dependency_tracking = true;
    config.cache_alignment            = 64; /* Common cache line size */

    return config;
}

/**
 * Create a progressive loader for a model
 */
HyperionProgressiveLoader *hyperionCreateProgressiveLoader(const char                    *model_path,
                                                       HyperionProgressiveLoaderConfig *config)
{
    if (!model_path || !config) {
        return NULL;
    }

    /* Create a memory-mapped model loader with default configuration */
    HyperionMmapConfig   mmap_config  = hyperionCreateDefaultMmapConfig();
    HyperionMappedModel *mapped_model = hyperionOpenMappedModel(model_path, &mmap_config);
    if (!mapped_model) {
        return NULL;
    }

    /* Create the progressive loader from the mapped model */
    HyperionProgressiveLoader *loader = hyperionCreateProgressiveLoaderFromMapped(mapped_model, config);
    if (!loader) {
        hyperionCloseMappedModel(mapped_model);
        return NULL;
    }

    /* Mark that we own the mapped model and should free it */
    loader->owns_mapped_model = true;

    return loader;
}

/**
 * Create a progressive loader from an existing mapped model
 */
HyperionProgressiveLoader *
hyperionCreateProgressiveLoaderFromMapped(HyperionMappedModel             *mapped_model,
                                        HyperionProgressiveLoaderConfig *config)
{
    if (!mapped_model || !config) {
        return NULL;
    }

    /* Allocate the loader structure */
    HyperionProgressiveLoader *loader =
        (HyperionProgressiveLoader *)malloc(sizeof(HyperionProgressiveLoader));
    if (!loader) {
        return NULL;
    }

    /* Initialize the loader */
    memset(loader, 0, sizeof(HyperionProgressiveLoader));
    loader->mapped_model      = mapped_model;
    loader->owns_mapped_model = false;
    loader->config            = *config;
    loader->current_memory    = 0;
    loader->peak_memory       = 0;
    loader->access_counter    = 0;
    loader->last_timestamp    = clock();

    /* Get layer count from mapped model */
    loader->layer_count = hyperionGetMappedLayerCount(mapped_model);
    if (loader->layer_count <= 0) {
        free(loader);
        return NULL;
    }

    /* Allocate layer information structures */
    loader->layers = (HyperionInternalLayerInfo *)malloc(loader->layer_count * sizeof(HyperionInternalLayerInfo));
    if (!loader->layers) {
        free(loader);
        return NULL;
    }

    /* Initialize layer information */
    for (int i = 0; i < loader->layer_count; i++) {
        const HyperionLayerDescriptor *desc = hyperionGetLayerDescriptor(mapped_model, i);
        if (!desc) {
            free(loader->layers);
            free(loader);
            return NULL;
        }

        loader->layers[i].index            = i;
        loader->layers[i].state            = HYPERION_LAYER_UNLOADED;
        loader->layers[i].weights          = NULL;
        loader->layers[i].size             = desc->size;
        loader->layers[i].precision        = desc->precision;
        loader->layers[i].last_access_time = 0;
        loader->layers[i].access_count     = 0;
        loader->layers[i].custom_priority  = 0.0f;
        loader->layers[i].dependencies     = NULL;
        loader->layers[i].dependency_count = 0;
        loader->layers[i].dependents       = NULL;
        loader->layers[i].dependent_count  = 0;
        loader->layers[i].avg_load_time    = 0.0;
        loader->layers[i].load_count       = 0;

        loader->total_model_size += desc->size;
    }

    /* Initialize usage tracking if enabled */
    if (config->enable_dependency_tracking) {
        loader->track_usage    = true;
        loader->history_size   = 100; /* Track last 100 accesses */
        loader->access_history = (uint64_t *)malloc(loader->history_size * sizeof(uint64_t));
        if (!loader->access_history) {
            free(loader->layers);
            free(loader);
            return NULL;
        }
        memset(loader->access_history, 0, loader->history_size * sizeof(uint64_t));
        loader->history_pos = 0;
    }

    return loader;
}

/**
 * Free progressive loader and release all resources
 */
void hyperionFreeProgressiveLoader(HyperionProgressiveLoader *loader)
{
    if (!loader) {
        return;
    }

    /* Free all layer dependencies */
    for (int i = 0; i < loader->layer_count; i++) {
        if (loader->layers[i].dependencies) {
            free(loader->layers[i].dependencies);
        }
        if (loader->layers[i].dependents) {
            free(loader->layers[i].dependents);
        }
    }

    /* Free layers array */
    if (loader->layers) {
        free(loader->layers);
    }

    /* Free access history */
    if (loader->access_history) {
        free(loader->access_history);
    }

    /* Close mapped model if we own it */
    if (loader->owns_mapped_model && loader->mapped_model) {
        hyperionCloseMappedModel(loader->mapped_model);
    }

    /* Free the loader structure */
    free(loader);
}

/**
 * Load a specific layer from the model
 */
bool hyperionLoadModelLayer(HyperionProgressiveLoader *loader, int layer_index)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return false;
    }

    HyperionLayerInfo *layer = &loader->layers[layer_index];

    /* Check if already loaded */
    if (layer->state == HYPERION_LAYER_LOADED) {
        /* Update access statistics */
        hyperionUpdateLayerAccess(loader, layer_index);
        return true;
    }

    /* Update state */
    layer->state = HYPERION_LAYER_LOADING;

    /* Check if we need to free memory first */
    if (loader->config.enable_layer_unloading &&
        loader->current_memory + layer->size > loader->config.max_memory_budget) {
        /* Find layers to unload based on priority strategy */
        while (loader->current_memory + layer->size > loader->config.max_memory_budget) {
            int layer_to_unload = -1;

            /* Find layer to unload based on priority strategy */
            switch (loader->config.priority_strategy) {
            case HYPERION_PRIORITY_LRU: {
                /* Find least recently used layer */
                uint64_t oldest_time = UINT64_MAX;
                for (int i = 0; i < loader->layer_count; i++) {
                    if (loader->layers[i].state == HYPERION_LAYER_LOADED && i != layer_index &&
                        hyperionCanUnloadLayer(loader, i) &&
                        loader->layers[i].last_access_time < oldest_time) {
                        oldest_time     = loader->layers[i].last_access_time;
                        layer_to_unload = i;
                    }
                }
                break;
            }
            case HYPERION_PRIORITY_MFU: {
                /* Find most frequently used layer */
                uint64_t lowest_count = UINT64_MAX;
                for (int i = 0; i < loader->layer_count; i++) {
                    if (loader->layers[i].state == HYPERION_LAYER_LOADED && i != layer_index &&
                        hyperionCanUnloadLayer(loader, i) &&
                        loader->layers[i].access_count < lowest_count) {
                        lowest_count    = loader->layers[i].access_count;
                        layer_to_unload = i;
                    }
                }
                break;
            }
            case HYPERION_PRIORITY_CUSTOM: {
                /* Find layer with lowest custom priority */
                float lowest_priority = FLT_MAX;
                for (int i = 0; i < loader->layer_count; i++) {
                    if (loader->layers[i].state == HYPERION_LAYER_LOADED && i != layer_index &&
                        hyperionCanUnloadLayer(loader, i) &&
                        loader->layers[i].custom_priority < lowest_priority) {
                        lowest_priority = loader->layers[i].custom_priority;
                        layer_to_unload = i;
                    }
                }
                break;
            }
            default:
                /* FIFO (sequential) - unload earliest loaded layer */
                for (int i = 0; i < loader->layer_count; i++) {
                    if (loader->layers[i].state == HYPERION_LAYER_LOADED && i != layer_index &&
                        hyperionCanUnloadLayer(loader, i)) {
                        layer_to_unload = i;
                        break;
                    }
                }
                break;
            }

            /* If we found a layer to unload, do it */
            if (layer_to_unload >= 0) {
                if (!hyperionUnloadLayer(loader, layer_to_unload)) {
                    layer->state = HYPERION_LAYER_UNLOADED;
                    return false;
                }
            }
            else {
                /* No more layers can be unloaded */
                fprintf(stderr, "Cannot free enough memory to load layer %d\n", layer_index);
                layer->state = HYPERION_LAYER_UNLOADED;
                return false;
            }
        }
    }

    /* Start timing the load operation */
    clock_t start = clock();

    /* Get layer weights from mapped model */
    void *weights = hyperionGetLayerWeights(loader->mapped_model, layer_index);
    if (!weights) {
        layer->state = HYPERION_LAYER_UNLOADED;
        return false;
    }

    /* Update layer info */
    layer->weights = weights;
    layer->state   = HYPERION_LAYER_LOADED;

    /* Update memory usage */
    loader->current_memory += layer->size;
    if (loader->current_memory > loader->peak_memory) {
        loader->peak_memory = loader->current_memory;
    }

    /* Calculate load time and update statistics */
    clock_t end          = clock();
    double  load_time_ms = ((double)(end - start)) * 1000.0 / CLOCKS_PER_SEC;

    /* Update layer load statistics */
    if (layer->load_count == 0) {
        layer->avg_load_time = load_time_ms;
    }
    else {
        /* Running average */
        layer->avg_load_time =
            (layer->avg_load_time * layer->load_count + load_time_ms) / (layer->load_count + 1);
    }
    layer->load_count++;

    /* Update access statistics */
    hyperionUpdateLayerAccess(loader, layer_index);

    return true;
}

/**
 * Unload a specific layer from the model
 */
bool hyperionUnloadLayer(HyperionProgressiveLoader *loader, int layer_index)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return false;
    }

    HyperionLayerInfo *layer = &loader->layers[layer_index];

    /* Check if already unloaded */
    if (layer->state != HYPERION_LAYER_LOADED) {
        return true;
    }

    /* Check if the layer can be unloaded */
    if (!hyperionCanUnloadLayer(loader, layer_index)) {
        return false;
    }

    /* Update state */
    layer->state = HYPERION_LAYER_UNLOADING;

    /* Release the layer weights in the mapped model */
    hyperionReleaseLayerWeights(loader->mapped_model, layer_index);

    /* Update layer info */
    layer->weights = NULL;
    layer->state   = HYPERION_LAYER_UNLOADED;

    /* Update memory usage */
    loader->current_memory -= layer->size;

    return true;
}

/**
 * Get pointer to a layer's weights, loading from disk if necessary
 */
void *hyperionGetProgressiveLayerWeights(HyperionProgressiveLoader *loader, int layer_index)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return NULL;
    }

    HyperionLayerInfo *layer = &loader->layers[layer_index];

    /* Load the layer if it's not already loaded */
    if (layer->state != HYPERION_LAYER_LOADED) {
        if (!hyperionLoadModelLayer(loader, layer_index)) {
            return NULL;
        }
    }
    else {
        /* Update access statistics if already loaded */
        hyperionUpdateLayerAccess(loader, layer_index);
    }

    /* Prefetch next layers if enabled */
    if (loader->config.prefetch_threshold > 0 && loader->config.max_prefetch_layers > 0) {
        int  count              = 0;
        int *layers_to_prefetch = hyperionGetLayersToPreload(loader, layer_index, &count);
        if (layers_to_prefetch) {
            /* Limit the number of layers to prefetch based on configuration */
            count = (count > loader->config.max_prefetch_layers)
                        ? loader->config.max_prefetch_layers
                        : count;

            /* Prefetch layers */
            for (int i = 0; i < count; i++) {
                int next_layer = layers_to_prefetch[i];
                if (loader->layers[next_layer].state == HYPERION_LAYER_UNLOADED) {
                    loader->layers[next_layer].state = HYPERION_LAYER_PREFETCHING;
                    hyperionPrefetchLayerWeights(loader->mapped_model, next_layer);
                }
            }

            free(layers_to_prefetch);
        }
    }

    return layer->weights;
}

/**
 * Get memory usage statistics for progressive loader
 */
HyperionMemoryStats hyperionGetProgressiveLoaderMemoryStats(const HyperionProgressiveLoader *loader)
{
    if (!loader) {
        HyperionMemoryStats empty = {0};
        return empty;
    }
    return loader->stats;
}

/**
 * Set memory budget for progressive loading
 */
bool hyperionSetProgressiveLoaderMemoryBudget(HyperionProgressiveLoader *loader, size_t budget_bytes)
{
    if (!loader || budget_bytes == 0) {
        return false;
    }

    /* Check if we need to unload layers to meet the new budget */
    if (budget_bytes < loader->current_memory) {
        /* We need to free some memory */
        size_t to_free = loader->current_memory - budget_bytes;

        /* Keep freeing until we meet the budget or can't free more */
        while (loader->current_memory > budget_bytes) {
            int    layer_to_unload = -1;
            size_t largest_size    = 0;

            /* Find largest loadable layer that can be freed */
            for (int i = 0; i < loader->layer_count; i++) {
                if (loader->layers[i].state == HYPERION_LAYER_LOADED &&
                    hyperionCanUnloadLayer(loader, i) && loader->layers[i].size > largest_size) {
                    largest_size    = loader->layers[i].size;
                    layer_to_unload = i;
                }
            }

            if (layer_to_unload >= 0) {
                if (!hyperionUnloadLayer(loader, layer_to_unload)) {
                    /* Failed to unload */
                    return false;
                }
            }
            else {
                /* Can't unload any more layers */
                fprintf(stderr, "Cannot meet memory budget of %zu bytes (current usage: %zu)\n",
                        budget_bytes, loader->current_memory);
                return false;
            }
        }
    }

    /* Update the budget */
    loader->config.max_memory_budget = budget_bytes;
    return true;
}

/**
 * Add a dependency between layers
 */
bool hyperionAddLayerDependency(HyperionProgressiveLoader *loader, int dependent_layer,
                              int dependency_layer)
{
    if (!loader || dependent_layer < 0 || dependent_layer >= loader->layer_count ||
        dependency_layer < 0 || dependency_layer >= loader->layer_count ||
        dependent_layer == dependency_layer) {
        return false;
    }

    HyperionLayerInfo *dependent  = &loader->layers[dependent_layer];
    HyperionLayerInfo *dependency = &loader->layers[dependency_layer];

    /* Check if dependency already exists */
    for (int i = 0; i < dependent->dependency_count; i++) {
        if (dependent->dependencies[i] == dependency_layer) {
            return true; /* Already exists */
        }
    }

    /* Add to dependent's dependencies */
    int *new_deps =
        (int *)realloc(dependent->dependencies, (dependent->dependency_count + 1) * sizeof(int));
    if (!new_deps) {
        return false;
    }

    dependent->dependencies                                = new_deps;
    dependent->dependencies[dependent->dependency_count++] = dependency_layer;

    /* Add to dependency's dependents */
    int *new_deps2 =
        (int *)realloc(dependency->dependents, (dependency->dependent_count + 1) * sizeof(int));
    if (!new_deps2) {
        /* Rollback */
        dependent->dependency_count--;
        return false;
    }

    dependency->dependents                                = new_deps2;
    dependency->dependents[dependency->dependent_count++] = dependent_layer;

    return true;
}

/**
 * Check if a layer can be safely unloaded
 */
bool hyperionCanUnloadLayer(const HyperionProgressiveLoader *loader, int layer_index)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return false;
    }

    const HyperionLayerInfo *layer = &loader->layers[layer_index];

    /* Can't unload if not loaded */
    if (layer->state != HYPERION_LAYER_LOADED) {
        return false;
    }

    /* Check if any dependent layers are loaded - if so, we can't unload this */
    if (loader->config.enable_dependency_tracking) {
        for (int i = 0; i < layer->dependent_count; i++) {
            int dep_idx = layer->dependents[i];
            if (loader->layers[dep_idx].state == HYPERION_LAYER_LOADED) {
                return false;
            }
        }
    }

    return true;
}

/**
 * Update layer access statistics
 */
void hyperionUpdateLayerAccess(HyperionProgressiveLoader *loader, int layer_index)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return;
    }

    HyperionLayerInfo *layer = &loader->layers[layer_index];

    /* Update access time and count */
    layer->last_access_time = ++loader->access_counter;
    layer->access_count++;

    /* Update access history if tracking is enabled */
    if (loader->track_usage) {
        loader->access_history[loader->history_pos] = layer_index;
        loader->history_pos = (loader->history_pos + 1) % loader->history_size;
    }
}

/**
 * Get next layers to preload based on usage patterns
 */
int *hyperionGetLayersToPreload(HyperionProgressiveLoader *loader, int current_layer, int *count)
{
    if (!loader || current_layer < 0 || current_layer >= loader->layer_count || !count) {
        *count = 0;
        return NULL;
    }

    *count = 0;

    /* If no history tracking or not enough history, use sequential strategy */
    if (!loader->track_usage || loader->access_counter < 10) {
        /* Simple sequential prediction: next layer is current + 1 */
        int next = current_layer + 1;
        if (next < loader->layer_count) {
            int *result = (int *)malloc(sizeof(int));
            if (!result) {
                return NULL;
            }
            result[0] = next;
            *count    = 1;
            return result;
        }
        return NULL;
    }

    /* Analyze usage pattern */
    HyperionUsagePattern pattern = hyperionGetUsagePattern(loader);

    /* Allocate result array (max size we might need) */
    int *result = (int *)malloc(loader->config.max_prefetch_layers * sizeof(int));
    if (!result) {
        return NULL;
    }

    switch (pattern) {
    case HYPERION_USAGE_SEQUENTIAL:
        /* Predict next N layers sequentially */
        for (int i = 0; i < loader->config.max_prefetch_layers; i++) {
            int next = current_layer + i + 1;
            if (next < loader->layer_count) {
                result[(*count)++] = next;
            }
        }
        break;

    case HYPERION_USAGE_REPEATED:
        /* Find most frequently accessed layers that aren't loaded */
        {
            /* Create array of layers with access counts */
            typedef struct {
                int      layer_index;
                uint64_t access_count;
            } LayerAccess;

            LayerAccess *access_counts =
                (LayerAccess *)malloc(loader->layer_count * sizeof(LayerAccess));

            if (!access_counts) {
                free(result);
                return NULL;
            }

            /* Fill access counts */
            int count_idx = 0;
            for (int i = 0; i < loader->layer_count; i++) {
                if (i != current_layer && loader->layers[i].state == HYPERION_LAYER_UNLOADED &&
                    loader->layers[i].access_count > 0) {
                    access_counts[count_idx].layer_index  = i;
                    access_counts[count_idx].access_count = loader->layers[i].access_count;
                    count_idx++;
                }
            }

            /* Sort by access count (simple bubble sort) */
            for (int i = 0; i < count_idx - 1; i++) {
                for (int j = 0; j < count_idx - i - 1; j++) {
                    if (access_counts[j].access_count < access_counts[j + 1].access_count) {
                        LayerAccess temp     = access_counts[j];
                        access_counts[j]     = access_counts[j + 1];
                        access_counts[j + 1] = temp;
                    }
                }
            }

            /* Take top N layers */
            for (int i = 0; i < count_idx && *count < loader->config.max_prefetch_layers; i++) {
                result[(*count)++] = access_counts[i].layer_index;
            }

            free(access_counts);
        }
        break;

    case HYPERION_USAGE_RANDOM:
    default:
        /* For random access or unknown patterns, don't prefetch */
        *count = 0;
        free(result);
        return NULL;
    }

    /* If we found no layers to prefetch, free the result array */
    if (*count == 0) {
        free(result);
        return NULL;
    }

    return result;
}

/**
 * Get usage pattern for the model based on layer access history
 */
HyperionUsagePattern hyperionGetUsagePattern(const HyperionProgressiveLoader *loader)
{
    if (!loader || !loader->track_usage || loader->access_counter < 10) {
        return HYPERION_USAGE_UNKNOWN;
    }

    /* Count sequential accesses in history */
    int sequential_count = 0;
    int repeat_count     = 0;

    for (int i = 1; i < loader->history_size && i < (int)loader->access_counter; i++) {
        int prev_idx = (loader->history_pos - i - 1 + loader->history_size) % loader->history_size;
        int curr_idx = (loader->history_pos - i + loader->history_size) % loader->history_size;

        if (loader->access_history[curr_idx] == loader->access_history[prev_idx] + 1) {
            sequential_count++;
        }

        /* Check for repeats (same layer accessed again) */
        for (int j = i + 1; j < loader->history_size && j < (int)loader->access_counter; j++) {
            int past_idx = (loader->history_pos - j + loader->history_size) % loader->history_size;
            if (loader->access_history[curr_idx] == loader->access_history[past_idx]) {
                repeat_count++;
                break;
            }
        }
    }

    /* Determine pattern based on counts */
    float seq_ratio    = (float)sequential_count / (float)loader->history_size;
    float repeat_ratio = (float)repeat_count / (float)loader->history_size;

    if (seq_ratio > 0.6f) {
        return HYPERION_USAGE_SEQUENTIAL;
    }
    else if (repeat_ratio > 0.4f) {
        return HYPERION_USAGE_REPEATED;
    }
    else {
        return HYPERION_USAGE_RANDOM;
    }
}

/**
 * Set custom priority for a layer
 */
bool hyperionSetLayerCustomPriority(HyperionProgressiveLoader *loader, int layer_index, float priority)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return false;
    }

    loader->layers[layer_index].custom_priority = priority;
    return true;
}

/**
 * Get the current state of a layer
 */
HyperionLayerState hyperionGetLayerState(const HyperionProgressiveLoader *loader, int layer_index)
{
    if (!loader || layer_index < 0 || layer_index >= loader->layer_count) {
        return HYPERION_LAYER_UNLOADED;
    }

    return loader->layers[layer_index].state;
}

/**
 * Optimize memory allocation across layers based on importance
 */
bool hyperionOptimizeLayerMemoryAllocation(HyperionProgressiveLoader *loader)
{
    if (!loader) {
        return false;
    }

    /* Switch to custom priority strategy */
    loader->config.priority_strategy = HYPERION_PRIORITY_CUSTOM;

    /* Analyze access patterns and set priorities */
    for (int i = 0; i < loader->layer_count; i++) {
        HyperionLayerInfo *layer = &loader->layers[i];

        /* Calculate priority based on access frequency and recency */
        float freq_score    = (float)layer->access_count / (float)(loader->access_counter + 1);
        float recency_score = (float)layer->last_access_time / (float)(loader->access_counter + 1);

        /* Also consider dependencies */
        float dep_score = 0.0f;
        if (loader->config.enable_dependency_tracking && layer->dependent_count > 0) {
            dep_score = (float)layer->dependent_count / (float)loader->layer_count;
        }

        /* Combine scores (weighted) */
        float priority = (freq_score * 0.4f) + (recency_score * 0.4f) + (dep_score * 0.2f);

        /* Set the custom priority */
        layer->custom_priority = priority;
    }

    return true;
}

/**
 * Preload a fixed sequence of layers
 */
bool hyperionPreloadLayers(HyperionProgressiveLoader *loader, const int *layer_indices, int count)
{
    if (!loader || !layer_indices || count <= 0) {
        return false;
    }

    bool success = true;

    /* Try to load each layer */
    for (int i = 0; i < count; i++) {
        int layer_idx = layer_indices[i];
        if (layer_idx >= 0 && layer_idx < loader->layer_count) {
            if (loader->layers[layer_idx].state == HYPERION_LAYER_UNLOADED) {
                if (!hyperionLoadModelLayer(loader, layer_idx)) {
                    success = false;
                }
            }
        }
    }

    return success;
}

/**
 * Clear all loaded layers to free memory
 */
bool hyperionClearAllLayers(HyperionProgressiveLoader *loader)
{
    if (!loader) {
        return false;
    }

    bool success = true;

    /* Try to unload each loaded layer */
    for (int i = 0; i < loader->layer_count; i++) {
        if (loader->layers[i].state == HYPERION_LAYER_LOADED) {
            if (!hyperionUnloadLayer(loader, i)) {
                success = false;
            }
        }
    }

    return success;
}

// Create progressive loader
HyperionProgressiveLoader *hyperionCreateProgressiveLoader(const HyperionProgressiveConfig *config)
{
    HyperionProgressiveLoader *loader = malloc(sizeof(HyperionProgressiveLoader));
    if (!loader)
        return NULL;

    // Initialize with default or provided config
    if (config) {
        loader->config = *config;
    }
    else {
        loader->config = DEFAULT_CONFIG;
    }

    // Initialize loader state
    loader->layers         = NULL;
    loader->layer_count    = 0;
    loader->current_memory = 0;
    loader->peak_memory    = 0;
    loader->access_counter = 0;
    loader->last_timestamp = clock();
    loader->track_usage    = false;
    loader->history_size   = 0;
    loader->history_pos    = 0;
    loader->is_initialized = false;

    return loader;
}

// Initialize layer information
bool hyperionInitLayerInfo(HyperionProgressiveLoader *loader, size_t layer_id, size_t memory_usage,
                         HyperionLayerPriority priority, const size_t *dependencies,
                         size_t num_dependencies)
{
    if (!loader || !dependencies || num_dependencies == 0)
        return false;

    // Resize layers array if needed
    if (layer_id >= loader->layer_count) {
        size_t           new_size   = layer_id + 1;
        HyperionLayerInfo *new_layers = realloc(loader->layers, new_size * sizeof(HyperionLayerInfo));
        if (!new_layers)
            return false;
        loader->layers      = new_layers;
        loader->layer_count = new_size;
    }

    // Initialize layer info
    HyperionLayerInfo *layer  = &loader->layers[layer_id];
    layer->index            = layer_id;
    layer->size             = memory_usage;
    layer->priority         = priority;
    layer->state            = HYPERION_LAYER_UNLOADED;
    layer->access_count     = 0;
    layer->last_access_time = 0;

    // Copy dependencies
    layer->dependencies = malloc(num_dependencies * sizeof(int));
    if (!layer->dependencies)
        return false;
    memcpy(layer->dependencies, dependencies, num_dependencies * sizeof(int));
    layer->dependency_count = num_dependencies;

    loader->is_initialized = true;
    return true;
}

// Request layer loading
bool hyperionRequestLayer(HyperionProgressiveLoader *loader, size_t layer_id)
{
    if (!loader || layer_id >= loader->layer_count)
        return false;

    HyperionLayerInfo *layer = &loader->layers[layer_id];

    // Check if layer is already loaded
    if (layer->state == HYPERION_LAYER_LOADED) {
        hyperionUpdateLayerAccess(loader, layer_id);
        return true;
    }

    // Check if we can load the layer
    if (!hyperionCanLoadLayer(loader, layer_id)) {
        return false;
    }

    // Load dependencies first
    for (size_t i = 0; i < layer->dependency_count; i++) {
        size_t dep_id = layer->dependencies[i];
        if (!hyperionRequestLayer(loader, dep_id)) {
            return false;
        }
    }

    // Update layer state and memory usage
    layer->state = HYPERION_LAYER_LOADED;
    loader->current_memory += layer->size;
    if (loader->current_memory > loader->peak_memory) {
        loader->peak_memory = loader->current_memory;
    }

    hyperionUpdateLayerAccess(loader, layer_id);
    return true;
}

// Unload layer
bool hyperionUnloadLayer(HyperionProgressiveLoader *loader, size_t layer_id)
{
    if (!loader || layer_id >= loader->layer_count)
        return false;

    HyperionLayerInfo *layer = &loader->layers[layer_id];

    // Check if layer is loaded
    if (layer->state != HYPERION_LAYER_LOADED)
        return true;

    // Check if any dependent layers are loaded
    for (size_t i = 0; i < loader->layer_count; i++) {
        if (i == layer_id)
            continue;
        HyperionLayerInfo *other = &loader->layers[i];
        if (other->state == HYPERION_LAYER_LOADED) {
            for (size_t j = 0; j < other->dependency_count; j++) {
                if (other->dependencies[j] == layer_id) {
                    return false; // Cannot unload, dependent layer is loaded
                }
            }
        }
    }

    // Unload the layer
    layer->state = HYPERION_LAYER_UNLOADED;
    loader->current_memory -= layer->size;
    return true;
}

// Get layer state
HyperionLayerState hyperionGetLayerState(const HyperionProgressiveLoader *loader, size_t layer_id)
{
    if (!loader || layer_id >= loader->layer_count) {
        return HYPERION_LAYER_UNLOADED;
    }
    return loader->layers[layer_id].state;
}

// Update layer priority
bool hyperionUpdateLayerPriority(HyperionProgressiveLoader *loader, size_t layer_id,
                               HyperionLayerPriority priority)
{
    if (!loader || layer_id >= loader->layer_count)
        return false;
    loader->layers[layer_id].priority = priority;
    return true;
}

// Get memory usage
size_t hyperionGetMemoryUsage(const HyperionProgressiveLoader *loader)
{
    return loader ? loader->current_memory : 0;
}

// Get peak memory usage
size_t hyperionGetPeakMemoryUsage(const HyperionProgressiveLoader *loader)
{
    return loader ? loader->peak_memory : 0;
}

// Check if layer can be loaded
bool hyperionCanLoadLayer(const HyperionProgressiveLoader *loader, size_t layer_id)
{
    if (!loader || layer_id >= loader->layer_count)
        return false;

    const HyperionLayerInfo *layer           = &loader->layers[layer_id];
    size_t                 required_memory = loader->current_memory + layer->size;

    // Check memory constraints
    if (required_memory > loader->config.max_memory) {
        return false;
    }

    // Check if we need to unload other layers
    if (required_memory > loader->config.load_threshold) {
        // TODO: Implement smart unloading strategy
        return false;
    }

    return true;
}

// Get layer dependencies
const size_t *hyperionGetLayerDependencies(const HyperionProgressiveLoader *loader, size_t layer_id,
                                         size_t *num_dependencies)
{
    if (!loader || layer_id >= loader->layer_count || !num_dependencies) {
        return NULL;
    }

    *num_dependencies = loader->layers[layer_id].dependency_count;
    return loader->layers[layer_id].dependencies;
}

// Update layer access
void hyperionUpdateLayerAccess(HyperionProgressiveLoader *loader, size_t layer_id)
{
    if (!loader || layer_id >= loader->layer_count)
        return;

    HyperionLayerInfo *layer = &loader->layers[layer_id];
    layer->access_count++;
    layer->last_access_time = get_timestamp_ms();
}

// Reset loader state
void hyperionResetProgressiveLoader(HyperionProgressiveLoader *loader)
{
    if (!loader)
        return;

    // Reset all layers to unloaded state
    for (size_t i = 0; i < loader->layer_count; i++) {
        loader->layers[i].state            = HYPERION_LAYER_UNLOADED;
        loader->layers[i].access_count     = 0;
        loader->layers[i].last_access_time = 0;
    }

    loader->current_memory = 0;
    loader->peak_memory    = 0;
    loader->access_counter = 0;
    loader->last_timestamp = clock();
}

// Enable/disable prefetching
void hyperionEnablePrefetching(HyperionProgressiveLoader *loader, bool enable)
{
    if (!loader)
        return;
    loader->track_usage = enable;
}

// Set prefetch distance
void hyperionSetPrefetchDistance(HyperionProgressiveLoader *loader, size_t distance)
{
    if (!loader)
        return;
    loader->config.prefetch_distance = distance;
}

// Get loader configuration
const HyperionProgressiveConfig *hyperionGetLoaderConfig(const HyperionProgressiveLoader *loader)
{
    return loader ? &loader->config : NULL;
}

// Set loader configuration
bool hyperionSetLoaderConfig(HyperionProgressiveLoader *loader, const HyperionProgressiveConfig *config)
{
    if (!loader || !config)
        return false;
    loader->config = *config;
    return true;
}