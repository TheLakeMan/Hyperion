# Memory-Mapped Model Loading API

This document provides detailed reference for Hyperion's memory-mapped model loading API.

## Overview

The memory-mapped model loading API allows Hyperion to access model weights directly from storage without loading the entire model into memory. This enables running larger models on constrained hardware.

## API Reference

### Types

#### `HyperionMappedModel`

Opaque structure representing a memory-mapped model file.

#### `HyperionLayerDescriptor`

Structure describing a layer in a memory-mapped model.

```c
typedef struct {
    size_t   offset;        /* Offset of layer weights in the mapped file */
    size_t   size;          /* Size of layer weights in bytes */
    int      layerIndex;    /* Index of the layer in the model */
    int      precision;     /* Precision of the layer weights (bits) */
    void    *cachedWeights; /* Pointer to cached weights (NULL if not cached) */
    bool     isActive;      /* Whether this layer is currently loaded */
    float    priority;      /* Priority for cache retention */
    uint64_t lastAccessed;  /* Timestamp of last access */
    uint64_t accessCount;   /* Number of times this layer has been accessed */
} HyperionLayerDescriptor;
```

#### `HyperionMmapConfig`

Configuration structure for memory-mapped model loading.

```c
typedef struct {
    size_t maxCacheSize;      /* Maximum size of in-memory cache in bytes */
    bool   prefetchEnabled;   /* Whether to enable weight prefetching */
    int    prefetchThreads;   /* Number of threads to use for prefetching */
    bool   adaptiveCaching;   /* Whether to use adaptive caching */
    size_t minLayerCacheSize; /* Minimum cache size per layer in bytes */
} HyperionMmapConfig;
```

### Functions

#### Opening and Closing Models

```c
// Create default configuration
HyperionMmapConfig hyperionCreateDefaultMmapConfig(void);

// Open a memory-mapped model file
HyperionMappedModel *hyperionOpenMappedModel(const char *filepath, 
                                         const HyperionMmapConfig *config);

// Close a memory-mapped model and release resources
void hyperionCloseMappedModel(HyperionMappedModel *model);
```

#### Layer Information

```c
// Get the number of layers in a memory-mapped model
int hyperionGetMappedLayerCount(const HyperionMappedModel *model);

// Get descriptor for a specific layer in the model
const HyperionLayerDescriptor *hyperionGetLayerDescriptor(
    const HyperionMappedModel *model, int layerIndex);
```

#### Layer Weight Management

```c
// Get pointer to a layer's weights, loading from disk if necessary
void *hyperionGetLayerWeights(HyperionMappedModel *model, int layerIndex);

// Prefetch a layer's weights into memory in anticipation of use
bool hyperionPrefetchLayerWeights(HyperionMappedModel *model, int layerIndex);

// Release a layer's weights from memory to free up space
void hyperionReleaseLayerWeights(HyperionMappedModel *model, int layerIndex);
```

#### Cache Management

```c
// Get current memory usage of the mapped model cache
size_t hyperionGetMappedModelMemoryUsage(const HyperionMappedModel *model);

// Set layer access priority for caching
void hyperionSetLayerPriority(HyperionMappedModel *model, 
                            int layerIndex, float priority);

// Reset all layer cache priorities to default values
void hyperionResetLayerPriorities(HyperionMappedModel *model);
```

## Usage Example

```c
#include <hyperion/utils/mmap_loader.h>
#include <stdio.h>

int main() {
    // Create a custom configuration
    HyperionMmapConfig config = hyperionCreateDefaultMmapConfig();
    config.maxCacheSize = 100 * 1024 * 1024; // 100MB cache
    
    // Open the model
    HyperionMappedModel* model = hyperionOpenMappedModel("model.tmai", &config);
    if (!model) {
        printf("Failed to open model\n");
        return 1;
    }
    
    // Get layer count
    int layerCount = hyperionGetMappedLayerCount(model);
    printf("Model has %d layers\n", layerCount);
    
    // Load and use each layer
    for (int i = 0; i < layerCount; i++) {
        void* weights = hyperionGetLayerWeights(model, i);
        if (weights) {
            // Use the weights...
            printf("Layer %d loaded successfully\n", i);
        }
        
        // Optionally release the layer to save memory
        if (i % 2 == 0) {
            hyperionReleaseLayerWeights(model, i);
        }
    }
    
    // Print memory usage
    printf("Memory usage: %.2f MB\n", 
           hyperionGetMappedModelMemoryUsage(model) / (1024.0 * 1024.0));
    
    // Close the model
    hyperionCloseMappedModel(model);
    
    return 0;
}
```

## See Also

- [Forward Pass Scheduler API](forward_scheduler.md)
- [Memory Optimization Guide](../guides/memory_optimization.md)