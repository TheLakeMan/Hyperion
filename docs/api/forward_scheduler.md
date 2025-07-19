# Forward Pass Scheduler API

This document provides detailed reference for Hyperion's Forward Pass Scheduler API, which enables layer-wise computation with automatic memory management.

## Overview

The Forward Pass Scheduler API allows for execution of neural network layers with optimized memory usage. It can schedule layer execution based on dependencies and automatically manage memory for intermediate activations.

## API Reference

### Types

#### `HyperionExecutionMode`

Enum defining forward pass execution modes.

```c
typedef enum {
    HYPERION_EXEC_NORMAL,     /* Standard execution - all layers loaded at once */
    HYPERION_EXEC_MEMORY_OPT, /* Memory optimized - only load weights when needed */
    HYPERION_EXEC_STREAMING,  /* Streaming - process inputs in chunks */
    HYPERION_EXEC_ADAPTIVE    /* Adaptive - automatically choose best strategy */
} HyperionExecutionMode;
```

#### `HyperionDependencyType`

Enum defining layer dependency types.

```c
typedef enum {
    HYPERION_DEP_NONE,       /* No dependency (parallel execution possible) */
    HYPERION_DEP_SEQUENTIAL, /* Must execute after previous layer */
    HYPERION_DEP_RESIDUAL,   /* Residual connection (depends on earlier layer) */
    HYPERION_DEP_ATTENTION   /* Attention dependency (complex pattern) */
} HyperionDependencyType;
```

#### `HyperionExecLayer`

Structure describing a layer in the execution schedule.

```c
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
```

#### `HyperionForwardScheduler`

Opaque structure representing a forward pass scheduler.

### Functions

#### Scheduler Creation and Destruction

```c
// Create a new forward pass scheduler
HyperionForwardScheduler *hyperionCreateForwardScheduler(
    HyperionMappedModel *model, 
    HyperionExecutionMode mode, 
    size_t maxMemory);

// Destroy a forward pass scheduler
void hyperionDestroyForwardScheduler(HyperionForwardScheduler *scheduler);
```

#### Layer Scheduling

```c
// Add a layer to the execution schedule
bool hyperionAddLayerToSchedule(
    HyperionForwardScheduler *scheduler, 
    int layerIndex, 
    int dependsOnLayer, 
    HyperionDependencyType depType, 
    size_t outputSize);

// Prepare for forward pass execution
bool hyperionPrepareForwardPass(HyperionForwardScheduler *scheduler);
```

#### Execution

```c
// Execute the next layer in the schedule
bool hyperionExecuteNextLayer(
    HyperionForwardScheduler *scheduler, 
    const void *input, 
    void *output, 
    int *layerIndex);

// Reset the scheduler for a new forward pass
void hyperionResetScheduler(HyperionForwardScheduler *scheduler);
```

#### Memory Management

```c
// Calculate the optimal batch size based on available memory
int hyperionCalculateOptimalBatchSize(
    HyperionForwardScheduler *scheduler,
    size_t inputSize,
    size_t outputSize,
    int maxBatchSize);

// Get the current memory usage of the scheduler
size_t hyperionGetSchedulerMemoryUsage(const HyperionForwardScheduler *scheduler);

// Get the maximum memory usage during execution
size_t hyperionGetSchedulerPeakMemoryUsage(const HyperionForwardScheduler *scheduler);

// Mark a layer's output as no longer needed (allows memory to be freed)
void hyperionMarkLayerOutputUnneeded(HyperionForwardScheduler *scheduler, int layerIndex);
```

#### Layer State

```c
// Get the execution status of a layer
bool hyperionIsLayerExecuted(const HyperionForwardScheduler *scheduler, int layerIndex);

// Get the output of a layer
void *hyperionGetLayerOutput(const HyperionForwardScheduler *scheduler, int layerIndex);
```

## Usage Example

This example demonstrates how to use the Forward Pass Scheduler to run a neural network with minimal memory usage:

```c
#include <hyperion/utils/mmap_loader.h>
#include <hyperion/utils/forward_scheduler.h>
#include <stdio.h>

int main() {
    // Open model with memory mapping
    HyperionMmapConfig mmapConfig = hyperionCreateDefaultMmapConfig();
    mmapConfig.maxCacheSize = 50 * 1024 * 1024; // 50MB cache
    
    HyperionMappedModel* model = hyperionOpenMappedModel("large_model.tmai", &mmapConfig);
    if (!model) {
        printf("Failed to open model\n");
        return 1;
    }
    
    // Create scheduler with memory limit
    HyperionForwardScheduler* scheduler = hyperionCreateForwardScheduler(
        model, HYPERION_EXEC_MEMORY_OPT, 100 * 1024 * 1024); // 100MB limit
    
    if (!scheduler) {
        printf("Failed to create scheduler\n");
        hyperionCloseMappedModel(model);
        return 1;
    }
    
    // Get layer count from the model
    int layerCount = hyperionGetMappedLayerCount(model);
    
    // Add layers to the schedule (sequential network example)
    for (int i = 0; i < layerCount; i++) {
        int dependsOn = (i > 0) ? i - 1 : -1;
        HyperionDependencyType depType = (i > 0) ? HYPERION_DEP_SEQUENTIAL : HYPERION_DEP_NONE;
        
        // Each layer produces activations of this size (for example)
        size_t outputSize = 1024 * 1024; // 1MB per layer
        
        if (!hyperionAddLayerToSchedule(scheduler, i, dependsOn, depType, outputSize)) {
            printf("Failed to add layer %d to schedule\n", i);
            hyperionDestroyForwardScheduler(scheduler);
            hyperionCloseMappedModel(model);
            return 1;
        }
    }
    
    // Prepare for execution
    if (!hyperionPrepareForwardPass(scheduler)) {
        printf("Failed to prepare forward pass\n");
        hyperionDestroyForwardScheduler(scheduler);
        hyperionCloseMappedModel(model);
        return 1;
    }
    
    // Execute the network layer by layer
    int layerIndex;
    while (hyperionExecuteNextLayer(scheduler, NULL, NULL, &layerIndex)) {
        printf("Executed layer %d\n", layerIndex);
        printf("Current memory usage: %.2f MB\n", 
               hyperionGetSchedulerMemoryUsage(scheduler) / (1024.0 * 1024.0));
        
        // If this was the last layer that needed a previous layer's output,
        // mark that previous layer's output as no longer needed
        if (layerIndex > 0 && !isOutputNeededByFutureLayers(layerIndex - 1)) {
            hyperionMarkLayerOutputUnneeded(scheduler, layerIndex - 1);
        }
    }
    
    // Print peak memory usage
    printf("Peak memory usage: %.2f MB\n", 
           hyperionGetSchedulerPeakMemoryUsage(scheduler) / (1024.0 * 1024.0));
    
    // Clean up
    hyperionDestroyForwardScheduler(scheduler);
    hyperionCloseMappedModel(model);
    
    return 0;
}

// Helper function to check if a layer's output is still needed
bool isOutputNeededByFutureLayers(int layerIndex) {
    // Implementation would depend on network architecture
    // This is a placeholder
    return false;
}
```

## Advanced Usage

### Example with Residual Connections

This example demonstrates how to schedule a network with residual connections:

```c
// Main branch
hyperionAddLayerToSchedule(scheduler, 0, -1, HYPERION_DEP_NONE, size0);     // Input layer
hyperionAddLayerToSchedule(scheduler, 1, 0, HYPERION_DEP_SEQUENTIAL, size1); // Conv1
hyperionAddLayerToSchedule(scheduler, 2, 1, HYPERION_DEP_SEQUENTIAL, size2); // Conv2

// Residual connection (depends on input layer)
hyperionAddLayerToSchedule(scheduler, 3, 0, HYPERION_DEP_RESIDUAL, size3);   // Residual

// Output layer (depends on both Conv2 and Residual)
hyperionAddLayerToSchedule(scheduler, 4, 2, HYPERION_DEP_SEQUENTIAL, size4); // Output
hyperionAddLayerToSchedule(scheduler, 4, 3, HYPERION_DEP_SEQUENTIAL, size4); // Output (same layer)
```

### Example with Attention Mechanism

This example demonstrates how to schedule a transformer-like network with attention:

```c
// Input embedding
hyperionAddLayerToSchedule(scheduler, 0, -1, HYPERION_DEP_NONE, embedSize);

// Self-attention layers
hyperionAddLayerToSchedule(scheduler, 1, 0, HYPERION_DEP_SEQUENTIAL, qSize);  // Query
hyperionAddLayerToSchedule(scheduler, 2, 0, HYPERION_DEP_SEQUENTIAL, kSize);  // Key
hyperionAddLayerToSchedule(scheduler, 3, 0, HYPERION_DEP_SEQUENTIAL, vSize);  // Value

// Attention mechanism (depends on Q, K, V)
hyperionAddLayerToSchedule(scheduler, 4, 1, HYPERION_DEP_ATTENTION, attSize); // Attention
hyperionAddLayerToSchedule(scheduler, 4, 2, HYPERION_DEP_ATTENTION, attSize); // (same layer)
hyperionAddLayerToSchedule(scheduler, 4, 3, HYPERION_DEP_ATTENTION, attSize); // (same layer)

// Residual connection and layer norm (depends on input and attention)
hyperionAddLayerToSchedule(scheduler, 5, 0, HYPERION_DEP_RESIDUAL, normSize);  // Residual
hyperionAddLayerToSchedule(scheduler, 5, 4, HYPERION_DEP_SEQUENTIAL, normSize); // (same layer)

// Feed-forward network
hyperionAddLayerToSchedule(scheduler, 6, 5, HYPERION_DEP_SEQUENTIAL, ffnSize);  // FFN

// Final residual and layer norm
hyperionAddLayerToSchedule(scheduler, 7, 5, HYPERION_DEP_RESIDUAL, outSize);    // Output
hyperionAddLayerToSchedule(scheduler, 7, 6, HYPERION_DEP_SEQUENTIAL, outSize);  // (same layer)
```

## Best Practices

1. **Add layers in topological order**: Add layers to the schedule in an order that respects their dependencies.

2. **Specify dependencies accurately**: Make sure to correctly specify all layer dependencies to ensure proper execution order.

3. **Mark outputs as unneeded**: When a layer's output is no longer needed by any future layers, mark it as unneeded to free memory.

4. **Consider execution mode**: Use the appropriate execution mode based on your memory vs. speed requirements.

5. **Monitor memory usage**: Use the memory usage functions to ensure your memory usage stays within acceptable limits.

## See Also

- [Memory-Mapped Model API](mmap_loader.md)
- [Memory Optimization Guide](../guides/memory_optimization.md)