/**
 * @file layer_scheduler.h
 * @brief Layer-wise computation scheduling for memory-efficient inference
 *
 * This header provides functionality for optimizing memory usage during neural
 * network inference by scheduling layer computations in a memory-aware fashion,
 * implementing activation checkpointing, and managing memory/speed trade-offs.
 */

#ifndef HYPERION_LAYER_SCHEDULER_H
#define HYPERION_LAYER_SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Layer types supported by the scheduler
 */
typedef enum {
    HYPERION_LAYER_LINEAR,        /**< Linear/fully connected layer */
    HYPERION_LAYER_CONV,          /**< Convolutional layer */
    HYPERION_LAYER_ATTENTION,     /**< Attention layer */
    HYPERION_LAYER_NORMALIZATION, /**< Normalization layer (e.g., LayerNorm) */
    HYPERION_LAYER_ACTIVATION,    /**< Activation function layer */
    HYPERION_LAYER_POOLING,       /**< Pooling layer */
    HYPERION_LAYER_EMBEDDING,     /**< Embedding layer */
    HYPERION_LAYER_CUSTOM         /**< Custom layer type */
} HyperionLayerType;

/**
 * @brief Memory usage strategy
 */
typedef enum {
    HYPERION_MEM_STRATEGY_DEFAULT,    /**< Default balanced strategy */
    HYPERION_MEM_STRATEGY_MIN_MEMORY, /**< Minimize memory usage (slower) */
    HYPERION_MEM_STRATEGY_MAX_SPEED,  /**< Maximize speed (more memory) */
    HYPERION_MEM_STRATEGY_ADAPTIVE    /**< Adapt based on runtime conditions */
} HyperionMemoryStrategy;

/**
 * @brief Layer checkpoint policies
 */
typedef enum {
    HYPERION_CHECKPOINT_NONE,      /**< No checkpointing */
    HYPERION_CHECKPOINT_SELECTIVE, /**< Checkpoint selected layers */
    HYPERION_CHECKPOINT_ALL        /**< Checkpoint all eligible layers */
} HyperionCheckpointPolicy;

/**
 * @brief Forward function signature for layer computation
 *
 * @param layerData Layer-specific data
 * @param inputs Input tensor data
 * @param outputs Output tensor data
 * @param userData User-provided data
 * @return 0 on success, non-zero on failure
 */
typedef int (*HyperionLayerForwardFn)(void *layerData, void *inputs, void *outputs, void *userData);

/**
 * @brief Layer descriptor structure
 */
typedef struct {
    HyperionLayerType      type;               /**< Layer type */
    const char          *name;               /**< Layer name */
    size_t               inputSize;          /**< Size of input in bytes */
    size_t               outputSize;         /**< Size of output in bytes */
    size_t               workspaceSize;      /**< Size of workspace needed in bytes */
    HyperionLayerForwardFn forward;            /**< Forward function */
    bool                 checkpointEligible; /**< Whether this layer can be checkpointed */
    void                *layerData;          /**< Layer-specific data */
    bool                 inPlace;            /**< Whether layer can operate in-place */
} HyperionLayerDesc;

/**
 * @brief Scheduler configuration
 */
typedef struct {
    HyperionMemoryStrategy   memoryStrategy;   /**< Memory usage strategy */
    HyperionCheckpointPolicy checkpointPolicy; /**< Checkpoint policy */
    size_t                 maxMemory; /**< Maximum memory budget in bytes (0 for unlimited) */
    size_t                 preferredWorkspaceSize; /**< Preferred workspace size in bytes */
    bool                   allowInPlace;           /**< Whether to allow in-place operations */
    bool                   optimizeOverlap; /**< Whether to optimize tensor lifetime overlap */
    bool                   verbose;         /**< Whether to print verbose information */
} HyperionLayerSchedulerConfig;

/**
 * @brief Layer scheduler handle
 */
typedef struct HyperionLayerScheduler HyperionLayerScheduler;

/**
 * @brief Execution statistics
 */
typedef struct {
    size_t   peakMemoryUsage;      /**< Peak memory usage in bytes */
    size_t   totalMemoryAllocated; /**< Total memory allocated in bytes */
    size_t   numCheckpoints;       /**< Number of checkpoints created */
    size_t   numRecomputations;    /**< Number of layer recomputations */
    size_t   layerExecutionCount;  /**< Total number of layer executions */
    uint64_t totalExecutionTimeNs; /**< Total execution time in nanoseconds */
} HyperionExecutionStats;

/**
 * @brief Get default scheduler configuration
 *
 * @param config Pointer to configuration struct to fill
 */
void hyperionLayerSchedulerGetDefaultConfig(HyperionLayerSchedulerConfig *config);

/**
 * @brief Create a new layer scheduler
 *
 * @param config Scheduler configuration
 * @return New scheduler instance or NULL on failure
 */
HyperionLayerScheduler *hyperionLayerSchedulerCreate(const HyperionLayerSchedulerConfig *config);

/**
 * @brief Destroy a layer scheduler
 *
 * @param scheduler Scheduler to destroy
 */
void hyperionLayerSchedulerDestroy(HyperionLayerScheduler *scheduler);

/**
 * @brief Add a layer to the scheduler
 *
 * @param scheduler Target scheduler
 * @param layer Layer descriptor
 * @return Layer ID on success, negative value on failure
 */
int hyperionLayerSchedulerAddLayer(HyperionLayerScheduler *scheduler, const HyperionLayerDesc *layer);

/**
 * @brief Add a dependency between layers
 *
 * @param scheduler Target scheduler
 * @param sourceLayerId Source layer ID
 * @param targetLayerId Target layer ID
 * @return 0 on success, non-zero on failure
 */
int hyperionLayerSchedulerAddDependency(HyperionLayerScheduler *scheduler, int sourceLayerId,
                                      int targetLayerId);

/**
 * @brief Prepare the scheduler for execution
 *
 * This function analyzes the layer graph, determines execution order, and
 * allocates required memory.
 *
 * @param scheduler Target scheduler
 * @return 0 on success, non-zero on failure
 */
int hyperionLayerSchedulerPrepare(HyperionLayerScheduler *scheduler);

/**
 * @brief Execute all layers in the scheduler
 *
 * @param scheduler Target scheduler
 * @param inputData Input data for the first layer
 * @param outputData Output data buffer for the final layer
 * @param userData User data passed to layer forward functions
 * @return 0 on success, non-zero on failure
 */
int hyperionLayerSchedulerExecute(HyperionLayerScheduler *scheduler, void *inputData, void *outputData,
                                void *userData);

/**
 * @brief Set checkpoint policy for a specific layer
 *
 * @param scheduler Target scheduler
 * @param layerId Layer ID
 * @param shouldCheckpoint Whether to checkpoint this layer
 * @return 0 on success, non-zero on failure
 */
int hyperionLayerSchedulerSetLayerCheckpoint(HyperionLayerScheduler *scheduler, int layerId,
                                           bool shouldCheckpoint);

/**
 * @brief Get execution statistics
 *
 * @param scheduler Target scheduler
 * @param stats Pointer to stats struct to fill
 */
void hyperionLayerSchedulerGetStats(HyperionLayerScheduler *scheduler, HyperionExecutionStats *stats);

/**
 * @brief Reset scheduler state
 *
 * Clears execution state without changing the layer graph.
 *
 * @param scheduler Target scheduler
 */
void hyperionLayerSchedulerReset(HyperionLayerScheduler *scheduler);

/**
 * @brief Estimate memory requirements
 *
 * @param scheduler Target scheduler
 * @param peakMemory Pointer to store peak memory estimate
 * @param totalMemory Pointer to store total memory estimate
 * @return 0 on success, non-zero on failure
 */
int hyperionLayerSchedulerEstimateMemory(HyperionLayerScheduler *scheduler, size_t *peakMemory,
                                       size_t *totalMemory);

/**
 * @brief Set memory strategy
 *
 * @param scheduler Target scheduler
 * @param strategy New memory strategy
 */
void hyperionLayerSchedulerSetMemoryStrategy(HyperionLayerScheduler *scheduler,
                                           HyperionMemoryStrategy  strategy);

/**
 * @brief Set checkpoint policy
 *
 * @param scheduler Target scheduler
 * @param policy New checkpoint policy
 */
void hyperionLayerSchedulerSetCheckpointPolicy(HyperionLayerScheduler  *scheduler,
                                             HyperionCheckpointPolicy policy);

/**
 * @brief Dump scheduler information for debugging
 *
 * @param scheduler Target scheduler
 */
void hyperionLayerSchedulerDump(HyperionLayerScheduler *scheduler);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HYPERION_LAYER_SCHEDULER_H */
