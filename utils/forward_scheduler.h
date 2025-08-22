/**
 * @file forward_scheduler.h
 * @brief Forward pass scheduler for layer-wise computation with memory optimization
 *
 * This header provides utilities for optimizing memory usage during model inference
 * by scheduling layer computations and intelligently managing weight loading/unloading.
 */

#ifndef HYPERION_FORWARD_SCHEDULER_H
#define HYPERION_FORWARD_SCHEDULER_H

#include "mmap_loader.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward pass execution mode
 */
typedef enum {
    HYPERION_EXEC_NORMAL,     /* Standard execution - all layers loaded at once */
    HYPERION_EXEC_MEMORY_OPT, /* Memory optimized - only load weights when needed */
    HYPERION_EXEC_STREAMING,  /* Streaming - process inputs in chunks */
    HYPERION_EXEC_ADAPTIVE    /* Adaptive - automatically choose best strategy */
} HyperionExecutionMode;

/**
 * Layer dependency type
 */
typedef enum {
    HYPERION_DEP_NONE,       /* No dependency (parallel execution possible) */
    HYPERION_DEP_SEQUENTIAL, /* Must execute after previous layer */
    HYPERION_DEP_RESIDUAL,   /* Residual connection (depends on earlier layer) */
    HYPERION_DEP_ATTENTION   /* Attention dependency (complex pattern) */
} HyperionDependencyType;

/**
 * Layer execution descriptor
 */
typedef struct {
    int                  layerIndex;     /* Index of the layer in the model */
    HyperionDependencyType depType;        /* Dependency type */
    int                  dependsOnLayer; /* Index of layer this depends on (-1 for none) */
    bool                 executed;       /* Whether this layer has been executed */
    bool                 outputNeeded;   /* Whether this layer's output is still needed */
    float                memoryUsage;    /* Memory usage of this layer's activation */
    void                *outputPtr;      /* Pointer to layer's output activation */
    size_t               outputSize;     /* Size of output activation in bytes */
} HyperionExecLayer;

/**
 * Forward pass scheduler structure
 */
typedef struct HyperionForwardScheduler HyperionForwardScheduler;

/**
 * Create a new forward pass scheduler
 *
 * @param model Mapped model to schedule execution for
 * @param mode Execution mode
 * @param maxMemory Maximum memory to use (in bytes, 0 for unlimited)
 * @return Scheduler instance or NULL on failure
 */
HyperionForwardScheduler *hyperionCreateForwardScheduler(HyperionMappedModel  *model,
                                                     HyperionExecutionMode mode, size_t maxMemory);

/**
 * Destroy a forward pass scheduler
 *
 * @param scheduler Scheduler to destroy
 */
void hyperionDestroyForwardScheduler(HyperionForwardScheduler *scheduler);

/**
 * Add a layer to the execution schedule
 *
 * @param scheduler Scheduler to add layer to
 * @param layerIndex Index of the layer in the model
 * @param dependsOnLayer Index of layer this depends on (-1 for none)
 * @param depType Dependency type
 * @param outputSize Size of the layer's output activation in bytes
 * @return true on success, false on failure
 */
bool hyperionAddLayerToSchedule(HyperionForwardScheduler *scheduler, int layerIndex, int dependsOnLayer,
                              HyperionDependencyType depType, size_t outputSize);

/**
 * Prepare for forward pass execution
 *
 * @param scheduler Scheduler to prepare
 * @return true on success, false on failure
 */
bool hyperionPrepareForwardPass(HyperionForwardScheduler *scheduler);

/**
 * Execute the next layer in the schedule
 *
 * @param scheduler Scheduler to execute next layer from
 * @param input Input data for the layer (if entry layer) or NULL
 * @param output Output pointer to receive result (if final layer) or NULL
 * @param layerIndex Pointer to receive the executed layer index or NULL
 * @return true if a layer was executed, false if no more layers or error
 */
bool hyperionExecuteNextLayer(HyperionForwardScheduler *scheduler, const void *input, void *output,
                            int *layerIndex);

/**
 * Calculate the optimal batch size based on available memory
 *
 * @param scheduler Scheduler to calculate for
 * @param inputSize Size of a single input in bytes
 * @param outputSize Size of a single output in bytes
 * @param maxBatchSize Maximum batch size to consider
 * @return Optimal batch size (1 if insufficient memory)
 */
int hyperionCalculateOptimalBatchSize(HyperionForwardScheduler *scheduler, size_t inputSize,
                                    size_t outputSize, int maxBatchSize);

/**
 * Get the current memory usage of the scheduler
 *
 * @param scheduler Scheduler to get memory usage for
 * @return Current memory usage in bytes
 */
size_t hyperionGetSchedulerMemoryUsage(const HyperionForwardScheduler *scheduler);

/**
 * Get the maximum memory usage during execution
 *
 * @param scheduler Scheduler to get max memory usage for
 * @return Maximum memory usage in bytes
 */
size_t hyperionGetSchedulerPeakMemoryUsage(const HyperionForwardScheduler *scheduler);

/**
 * Reset the scheduler for a new forward pass
 *
 * @param scheduler Scheduler to reset
 */
void hyperionResetScheduler(HyperionForwardScheduler *scheduler);

/**
 * Get the execution status of a layer
 *
 * @param scheduler Scheduler to check
 * @param layerIndex Index of the layer to check
 * @return true if the layer has been executed, false otherwise
 */
bool hyperionIsLayerExecuted(const HyperionForwardScheduler *scheduler, int layerIndex);

/**
 * Get the output of a layer
 *
 * @param scheduler Scheduler to get output from
 * @param layerIndex Index of the layer to get output for
 * @return Pointer to layer output or NULL if not available
 */
void *hyperionGetLayerOutput(const HyperionForwardScheduler *scheduler, int layerIndex);

/**
 * Mark a layer's output as no longer needed (allows memory to be freed)
 *
 * @param scheduler Scheduler to update
 * @param layerIndex Index of the layer to mark
 */
void hyperionMarkLayerOutputUnneeded(HyperionForwardScheduler *scheduler, int layerIndex);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_FORWARD_SCHEDULER_H */