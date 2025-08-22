# Core API Reference

## Overview

The Core API provides the fundamental functionality for the Hyperion framework, including initialization, memory management, and basic operations.

## Initialization and Cleanup

### `hyperion_init()`
```c
void hyperion_init(void);
```
Initializes the Hyperion framework. Must be called before any other Hyperion functions.

**Example:**
```c
#include <hyperion/core.h>

int main() {
    hyperion_init();
    // Use Hyperion functionality
    hyperion_shutdown();
    return 0;
}
```

### `hyperion_shutdown();`
```c
void hyperion_shutdown();
```
Cleans up resources used by the Hyperion framework. Should be called when Hyperion is no longer needed.

## Memory Management

### `hyperion_configure_memory()`
```c
void hyperion_configure_memory(const HyperionMemoryConfig* config);
```
Configures memory management settings.

**Parameters:**
- `config`: Pointer to memory configuration structure

**Example:**
```c
```c
HyperionMemoryConfig mem_config = {
    .initial_pool_size = 1024 * 1024,  // 1MB
    .enable_optimization = true
};
hyperion_configure_memory(&mem_config);
```
```

### `hyperion_get_memory_usage()`
```c
size_t hyperion_get_memory_usage(void);
```
Returns the current memory usage in bytes.

## Error Handling

### `hyperion_get_error()`
```c
const char* hyperion_get_error(void);
```
Returns the last error message.

**Example:**
```c
HyperionModel* model = hyperion_load_model("model.tmai");
if (!model) {
    printf("Error: %s\n", hyperion_get_error());
    return 1;
}
```

## Logging

### `hyperion_set_log_level()`
```c
void hyperion_set_log_level(HyperionLogLevel level);
```
Sets the logging level.

**Parameters:**
- `level`: Logging level (HYPERION_LOG_DEBUG, HYPERION_LOG_INFO, HYPERION_LOG_WARN, HYPERION_LOG_ERROR)

**Example:**
```c
hyperion_set_log_level(HYPERION_LOG_DEBUG);
```

## Performance Monitoring

### `hyperion_get_performance_metrics()`
```c
void hyperion_get_performance_metrics(HyperionPerformanceMetrics* metrics);
```
Retrieves current performance metrics.

**Parameters:**
- `metrics`: Pointer to metrics structure to be filled

**Example:**
```c
HyperionPerformanceMetrics metrics;
hyperion_get_performance_metrics(&metrics);
printf("Memory usage: %zu bytes\n", metrics.memory_usage);
```

## Data Types

### `HyperionMemoryConfig`
```c
typedef struct {
    size_t initial_pool_size;
    bool enable_optimization;
    bool track_usage;
} HyperionMemoryConfig;
```
Memory configuration structure.

### `HyperionPerformanceMetrics`
```c
typedef struct {
    size_t memory_usage;
    double execution_time;
    size_t cache_hits;
    size_t cache_misses;
} HyperionPerformanceMetrics;
```
Performance metrics structure.

### `HyperionLogLevel`
```c
typedef enum {
    HYPERION_LOG_DEBUG,
    HYPERION_LOG_INFO,
    HYPERION_LOG_WARN,
    HYPERION_LOG_ERROR
} HyperionLogLevel;
```
Logging level enumeration.

## Best Practices

1. Always initialize Hyperion before use
2. Configure memory settings based on your needs
3. Check error messages when operations fail
4. Set appropriate logging level for debugging
5. Monitor performance metrics for optimization
6. Clean up resources when done

## Common Patterns

### Basic Usage
```c
#include <hyperion/core.h>

int main() {
    // Initialize
    hyperion_init();
    
    // Configure memory
    HyperionMemoryConfig mem_config = {
        .initial_pool_size = 1024 * 1024,
        .enable_optimization = true
    };
    hyperion_configure_memory(&mem_config);
    
    // Set logging level
    hyperion_set_log_level(HYPERION_LOG_INFO);
    
    // Use Hyperion functionality
    
    // Clean up
    hyperion_shutdown();
    return 0;
}
```

### Error Handling
```c
HyperionModel* model = hyperion_load_model("model.tmai");
if (!model) {
    printf("Error: %s\n", hyperion_get_error());
    return 1;
}
```

### Performance Monitoring
```c
HyperionPerformanceMetrics metrics;
hyperion_get_performance_metrics(&metrics);
printf("Memory usage: %zu bytes\n", metrics.memory_usage);
printf("Execution time: %f ms\n", metrics.execution_time);
```

## Related Documentation

- [Memory Management API](memory.md)
- [Performance Tools API](performance.md)
- [Models API](models.md)