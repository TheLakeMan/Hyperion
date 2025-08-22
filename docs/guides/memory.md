# Hyperion Memory Guide

## Overview

This comprehensive guide covers all aspects of memory management and optimization in Hyperion, from basic allocation strategies to advanced optimization techniques.

## Table of Contents
1. [Basic Memory Management](#basic-memory-management)
   - Memory Allocation Strategies
   - Memory Pools
   - Resource Management
2. [Memory Optimization Techniques](#memory-optimization-techniques)
   - Memory-Mapped Model Loading
   - Forward Pass Scheduling
   - Progressive Loading
   - Mixed Precision Quantization
   - Model Pruning
3. [Memory Monitoring](#memory-monitoring)
   - Usage Tracking
   - Performance Analysis
   - Troubleshooting
4. [Best Practices](#best-practices)
   - Configuration Guidelines
   - Common Patterns
   - Error Handling

## Basic Memory Management

### Memory Allocation Strategies

1. **Static Allocation**
   - Best for fixed-size, frequently accessed data
   - Low overhead, predictable performance
   ```c
   HyperionMemoryConfig config = {
       .strategy = HYPERION_MEMORY_STRATEGY_STATIC,
       .initial_pool_size = 1024 * 1024  // 1MB
   };
   ```

2. **Pooled Allocation**
   - Best for frequent allocations/deallocations
   - Reduces fragmentation
   ```c
   HyperionMemoryConfig config = {
       .strategy = HYPERION_MEMORY_STRATEGY_POOLED,
       .initial_pool_size = 2 * 1024 * 1024,  // 2MB
       .block_size = 4096  // 4KB blocks
   };
   ```

3. **Dynamic Allocation**
   - Best for unpredictable memory needs
   - More flexible but higher overhead
   ```c
   HyperionMemoryConfig config = {
       .strategy = HYPERION_MEMORY_STRATEGY_DYNAMIC,
       .track_allocations = true
   };
   ```

4. **Hybrid Allocation**
   - Combines multiple strategies
   - Optimizes for different use cases
   ```c
   HyperionMemoryConfig config = {
       .strategy = HYPERION_MEMORY_STRATEGY_HYBRID,
       .initial_pool_size = 1024 * 1024,
       .enable_optimization = true
   };
   ```

### Memory Pools

1. **Creating and Using Pools**
   ```c
   // Create a memory pool
   HyperionMemoryPool* pool = hyperion_create_pool(
       1024 * 1024,  // 1MB total size
       4096          // 4KB block size
   );

   // Allocate from pool
   void* data = hyperion_pool_alloc(pool, size);

   // Free pool memory
   hyperion_pool_free(pool, data);
   ```

2. **Pool Configuration**
   - Choose appropriate block sizes
   - Monitor pool utilization
   - Consider fragmentation

## Memory Optimization Techniques

### Memory-Mapped Model Loading

1. **Basic Usage**
   ```c
   #include <hyperion/utils/mmap_loader.h>

   // Create a default configuration
   HyperionMmapConfig config = hyperionCreateDefaultMmapConfig();

   // Open the model with memory mapping
   HyperionMappedModel* model = hyperionOpenMappedModel("path/to/model.tmai", &config);
   ```

2. **Configuration Options**
   ```c
   HyperionMmapConfig config = hyperionCreateDefaultMmapConfig();
   config.maxCacheSize = 100 * 1024 * 1024;  // 100MB
   config.prefetchEnabled = true;
   config.prefetchThreads = 2;
   config.adaptiveCaching = true;
   config.minLayerCacheSize = 8 * 1024;      // 8KB
   ```

### Forward Pass Scheduling

1. **Basic Setup**
   ```c
   // Create a scheduler with memory limit
   HyperionForwardScheduler* scheduler = hyperionCreateForwardScheduler(
       model,                  // Mapped model
       HYPERION_EXEC_MEMORY_OPT, // Memory optimization mode
       100 * 1024 * 1024      // 100MB memory limit
   );
   ```

2. **Layer Dependencies**
   ```c
   // Sequential dependency
   hyperionAddLayerToSchedule(scheduler, layerIndex, prevLayer, 
                           HYPERION_DEP_SEQUENTIAL, outputSize);

   // Residual connection
   hyperionAddLayerToSchedule(scheduler, layerIndex, residualLayer,
                           HYPERION_DEP_RESIDUAL, outputSize);
   ```

### Progressive Loading

1. **Configuration**
   ```c
   HyperionProgressiveConfig config = {
       .enable_prefetch = true,
       .prefetch_window = 2,
       .enable_unload = true,
       .memory_threshold = 768 * 1024 * 1024  // 768MB
   };
   ```

2. **Implementation**
   ```c
   HyperionProgressiveLoader* loader = hyperion_create_progressive_loader(&config);

   for (int i = 0; i < num_layers; i++) {
       if (!hyperion_load_layer(loader, i)) {
           break;
       }
       process_layer(i);
       if (memory_pressure) {
           hyperion_unload_layer(loader, i - 2);
       }
   }
   ```

### Mixed Precision Quantization

```c
HyperionMixedPrecisionConfig mpConfig;
hyperionInitMixedPrecisionConfig(&mpConfig);

mpConfig.embeddingPrecision = HYPERION_PREC_INT8;
mpConfig.attentionPrecision = HYPERION_PREC_INT4;
mpConfig.ffnPrecision = HYPERION_PREC_INT4;
mpConfig.outputPrecision = HYPERION_PREC_FP16;

hyperionApplyMixedPrecision(model, &mpConfig);
```

### Model Pruning

```c
HyperionPruneConfig pruneConfig;
hyperionInitPruneConfig(&pruneConfig);

pruneConfig.method = HYPERION_PRUNE_MAGNITUDE;
pruneConfig.sparsity = 0.7;
pruneConfig.blockSize = 4;

hyperionPruneModel(model, &pruneConfig);
```

## Memory Monitoring

### Usage Tracking

1. **Basic Monitoring**
   ```c
   // Enable memory tracking
   hyperion_enable_memory_tracking(true);

   // Get memory statistics
   HyperionMemoryStats stats = hyperion_get_memory_stats();
   printf("Current usage: %zu bytes\n", stats.current_usage);
   printf("Peak usage: %zu bytes\n", stats.peak_usage);
   ```

2. **Detailed Analysis**
   ```c
   // Generate memory report
   hyperion_generate_memory_report("memory_report.txt");
   ```

### Performance Analysis

1. **Memory Usage Metrics**
   - Total allocated memory
   - Current memory usage
   - Peak memory usage
   - Allocation count
   - Fragmentation ratio

2. **Optimization Impact**
   ```c
   // Get current memory usage
   size_t memUsage = hyperionGetSchedulerMemoryUsage(scheduler);

   // Get peak memory usage
   size_t peakMemUsage = hyperionGetSchedulerPeakMemoryUsage(scheduler);
   ```

## Best Practices

### Configuration Guidelines

1. **Memory Strategy Selection**
   - Use static allocation for fixed-size data
   - Use pooled allocation for frequent operations
   - Use dynamic allocation for unpredictable needs
   - Consider hybrid strategies for complex cases

2. **Resource Management**
   - Implement proper cleanup
   - Use RAII patterns when possible
   - Monitor resource usage
   - Handle out-of-memory conditions

### Common Patterns

1. **Memory Pool Usage**
   ```c
   HyperionMemoryPool* pool = hyperion_create_pool(pool_size, object_size);
   for (int i = 0; i < num_objects; i++) {
       void* obj = hyperion_pool_alloc(pool, object_size);
       // Use object
       hyperion_pool_free(pool, obj);
   }
   ```

2. **Progressive Loading**
   ```c
   while (processing) {
       if (memory_pressure) {
           unload_unused_layers();
       }
       load_next_layer();
       process_layer();
   }
   ```

### Error Handling

1. **Common Issues**
   - Memory leaks
   - Fragmentation
   - Out-of-memory errors
   - Poor performance

2. **Solutions**
   - Enable detailed tracking
   - Monitor allocation patterns
   - Implement proper cleanup
   - Use appropriate strategies

## Memory Management Checklist

- [ ] Choose appropriate memory strategy
- [ ] Configure memory pools if needed
- [ ] Implement progressive loading for large models
- [ ] Enable memory tracking
- [ ] Monitor memory usage
- [ ] Handle out-of-memory conditions
- [ ] Implement cleanup procedures
- [ ] Consider mixed precision quantization
- [ ] Evaluate model pruning options
- [ ] Set up performance monitoring