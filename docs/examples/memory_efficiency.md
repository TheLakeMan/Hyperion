# Memory Efficiency Examples

This document provides practical examples of using Hyperion's memory-efficient tensor operations and performance impact assessment tools.

## Memory-Efficient Tensor Operations

### Example 1: Creating a Memory-Efficient Tensor

```c
#include "hyperion/tensor.h"
#include "hyperion/memory.h"

void create_memory_efficient_tensor() {
    // Configure tensor memory allocation
    HyperionTensorMemoryConfig config = {
        .strategy = HYPERION_TENSOR_POOLED,
        .pool_size = 1024 * 1024,  // 1MB pool
        .stream_buffer_size = 0,    // Not using streaming
        .enable_in_place = true     // Enable in-place operations
    };

    // Create tensor dimensions
    int dims[] = {32, 32, 3};  // 32x32x3 tensor
    HyperionDataType dtype = HYPERION_FLOAT32;

    // Create the tensor
    HyperionTensor* tensor = hyperionCreateTensorWithMemoryConfig(
        &config, 3, dims, dtype);

    // Use the tensor...
    
    // Get memory statistics
    HyperionTensorMemoryStats stats = hyperionGetTensorMemoryStats(tensor);
    printf("Memory usage: %zu bytes\n", stats.used_memory);
    printf("Pool efficiency: %.2f%%\n", stats.pool_efficiency * 100.0f);

    // Clean up
    hyperionFreeTensor(tensor);
}
```

### Example 2: In-Place Tensor Operations

```c
void perform_in_place_operations() {
    // Create tensor (as shown in Example 1)
    HyperionTensor* tensor = /* ... */;

    // Perform in-place addition
    float add_value = 1.0f;
    hyperionTensorInPlaceOp(tensor, HYPERION_OP_ADD, &add_value);

    // Perform in-place multiplication
    float mul_value = 2.0f;
    hyperionTensorInPlaceOp(tensor, HYPERION_OP_MUL, &mul_value);

    // Note: No additional memory is allocated for these operations
}
```

### Example 3: Streaming Tensor Data

```c
void stream_tensor_data() {
    // Configure for streaming
    HyperionTensorMemoryConfig config = {
        .strategy = HYPERION_TENSOR_STREAMING,
        .stream_buffer_size = 4096,  // 4KB buffer
        .enable_in_place = false
    };

    // Create a large tensor
    int dims[] = {1000, 1000};  // 1M elements
    HyperionTensor* tensor = hyperionCreateTensorWithMemoryConfig(
        &config, 2, dims, HYPERION_FLOAT32);

    // Stream data in chunks
    float* data_chunk = malloc(1024 * sizeof(float));
    for (size_t offset = 0; offset < 1000000; offset += 1024) {
        // Fill data_chunk with values...
        hyperionStreamTensorData(tensor, data_chunk, offset, 1024);
    }
    free(data_chunk);
}
```

## Performance Impact Assessment

### Example 1: Basic Performance Tracking

```c
#include "hyperion/performance.h"

void track_performance() {
    // Configure performance tracking
    HyperionPerformanceConfig config = {
        .track_execution_time = true,
        .track_memory_usage = true,
        .track_cpu_usage = true,
        .track_cache_usage = true,
        .enable_optimization = true
    };

    // Create performance analysis context
    HyperionPerformanceAnalysis* analysis = hyperionCreatePerformanceAnalysis(&config);

    // Record baseline metrics
    HyperionPerformanceMetrics baseline = {
        .execution_time_ms = 100.0,
        .memory_usage_bytes = 1024 * 1024,
        .cpu_usage_percent = 50.0f,
        .cache_misses = 1000,
        .cache_hits = 9000
    };
    hyperionRecordMetrics(analysis, &baseline);

    // Perform some operations...

    // Record current metrics
    HyperionPerformanceMetrics current = {
        .execution_time_ms = 80.0,
        .memory_usage_bytes = 512 * 1024,
        .cpu_usage_percent = 40.0f,
        .cache_misses = 800,
        .cache_hits = 9200
    };
    hyperionRecordMetrics(analysis, &current);

    // Analyze optimization impact
    hyperionAnalyzeOptimizationImpact(analysis, &baseline, &current);

    // Get and print the impact
    HyperionOptimizationImpact impact = hyperionGetOptimizationImpact(analysis);
    printf("Speedup: %.2fx\n", impact.speedup_factor);
    printf("Memory reduction: %.2f%%\n", impact.memory_reduction);
    printf("CPU efficiency: %.2f%%\n", impact.cpu_efficiency);
    printf("Recommendations: %s\n", impact.recommendations);

    // Generate report
    hyperionGeneratePerformanceReport(analysis, "performance_report.txt");

    // Clean up
    hyperionFreePerformanceAnalysis(analysis);
}
```

### Example 2: Continuous Performance Monitoring

```c
void monitor_performance() {
    HyperionPerformanceAnalysis* analysis = /* ... */;
    
    // Take performance samples at regular intervals
    for (int i = 0; i < 10; i++) {
        // Perform operations...
        
        // Take a performance sample
        hyperionTakePerformanceSample(analysis);
        
        // Get current metrics
        HyperionPerformanceMetrics metrics = hyperionGetPerformanceMetrics(analysis);
        printf("Sample %d: %.2f ms, %zu bytes\n", 
               i, metrics.execution_time_ms, metrics.memory_usage_bytes);
        
        // Calculate trend
        float trend = hyperionGetPerformanceTrend(analysis);
        printf("Performance trend: %.2f%%\n", trend * 100.0f);
    }
}
```

### Example 3: Memory Usage Analysis

```c
void analyze_memory_usage() {
    HyperionPerformanceAnalysis* analysis = /* ... */;
    
    // Enable memory tracking
    hyperionEnablePerformanceAnalysis(analysis, true);
    
    // Perform memory-intensive operations...
    
    // Get detailed memory statistics
    HyperionMemoryStats stats = hyperionGetMemoryStats();
    printf("Total allocations: %zu\n", stats.total_allocations);
    printf("Total deallocations: %zu\n", stats.total_deallocations);
    printf("Peak memory usage: %zu bytes\n", stats.peak_memory_usage);
    printf("Current memory usage: %zu bytes\n", stats.current_memory_usage);
    
    // Check for memory leaks
    if (stats.total_allocations != stats.total_deallocations) {
        printf("Warning: Potential memory leak detected!\n");
    }
}
```

## Best Practices

1. **Memory-Efficient Tensor Operations**
   - Use pooled allocation for frequently created/destroyed tensors
   - Enable in-place operations when possible
   - Use streaming for very large tensors
   - Monitor memory statistics regularly

2. **Performance Impact Assessment**
   - Establish baseline metrics before optimization
   - Track both memory and execution time
   - Use continuous monitoring for long-running operations
   - Generate regular performance reports
   - Pay attention to optimization recommendations

3. **General Tips**
   - Start with conservative memory budgets
   - Gradually increase optimization aggressiveness
   - Monitor both performance and memory usage
   - Use the provided tools to identify bottlenecks
   - Document performance improvements