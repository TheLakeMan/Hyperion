# Memory Management API Reference

## Overview

The Memory Management API provides functions for configuring and managing memory usage in Hyperion, including memory pools, allocation strategies, and optimization features.

## Memory Configuration

### `hyperion_configure_memory()`
```c
void hyperion_configure_memory(const HyperionMemoryConfig* config);
```
Configures the memory management system with specified settings.

**Parameters:**
- `config`: Pointer to memory configuration structure

**Example:**
```c
HyperionMemoryConfig mem_config = {
    .initial_pool_size = 1024 * 1024,  // 1MB
    .enable_optimization = true,
    .track_usage = true
};
hyperion_configure_memory(&mem_config);
```

### `hyperion_get_memory_config()`
```c
void hyperion_get_memory_config(HyperionMemoryConfig* config);
```
Retrieves the current memory configuration.

**Parameters:**
- `config`: Pointer to store configuration

## Memory Allocation

### `hyperion_malloc()`
```c
void* hyperion_malloc(size_t size);
```
Allocates memory from the memory pool.

**Parameters:**
- `size`: Size in bytes to allocate

**Returns:**
- Pointer to allocated memory or NULL on failure

**Example:**
```c
void* data = hyperion_malloc(1024);
if (!data) {
    printf("Memory allocation failed\n");
    return 1;
}
```

### `hyperion_free()`
```c
void hyperion_free(void* ptr);
```
Frees previously allocated memory.

**Parameters:**
- `ptr`: Pointer to memory to free

**Example:**
```c
void* data = hyperion_malloc(1024);
// Use memory
hyperion_free(data);
```

## Memory Pool Management

### `hyperion_create_memory_pool()`
```c
HyperionMemoryPool* hyperion_create_memory_pool(size_t size);
```
Creates a new memory pool.

**Parameters:**
- `size`: Initial size of the pool

**Returns:**
- Pointer to memory pool or NULL on failure

### `hyperion_destroy_memory_pool()`
```c
void hyperion_destroy_memory_pool(HyperionMemoryPool* pool);
```
Destroys a memory pool and frees all associated memory.

**Parameters:**
- `pool`: Pointer to memory pool

## Memory Usage Tracking

### `hyperion_get_memory_usage()`
```c
size_t hyperion_get_memory_usage(void);
```
Returns the current memory usage in bytes.

**Returns:**
- Current memory usage in bytes

### `hyperion_get_memory_stats()`
```c
void hyperion_get_memory_stats(HyperionMemoryStats* stats);
```
Retrieves detailed memory statistics.

**Parameters:**
- `stats`: Pointer to store statistics

## Memory Optimization

### `hyperion_optimize_memory()`
```c
void hyperion_optimize_memory(void);
```
Performs memory optimization operations.

### `hyperion_set_memory_strategy()`
```c
void hyperion_set_memory_strategy(HyperionMemoryStrategy strategy);
```
Sets the memory allocation strategy.

**Parameters:**
- `strategy`: Memory strategy to use

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

### `HyperionMemoryStats`
```c
typedef struct {
    size_t total_allocated;
    size_t peak_usage;
    size_t current_usage;
    size_t fragmentation;
} HyperionMemoryStats;
```
Memory statistics structure.

### `HyperionMemoryStrategy`
```c
typedef enum {
    HYPERION_MEMORY_STRATEGY_DEFAULT,
    HYPERION_MEMORY_STRATEGY_POOLED,
    HYPERION_MEMORY_STRATEGY_STREAMING
} HyperionMemoryStrategy;
```
Memory allocation strategy enumeration.

## Best Practices

1. Configure memory settings based on application needs
2. Use memory pools for frequent allocations
3. Monitor memory usage during development
4. Enable optimization for production use
5. Track memory usage for debugging
6. Use appropriate allocation strategies

## Common Patterns

### Memory Pool Usage
```c
// Create memory pool
HyperionMemoryPool* pool = hyperion_create_memory_pool(1024 * 1024);
if (!pool) {
    printf("Failed to create memory pool\n");
    return 1;
}

// Configure memory
HyperionMemoryConfig mem_config = {
    .initial_pool_size = 1024 * 1024,
    .enable_optimization = true
};
hyperion_configure_memory(&mem_config);

// Allocate memory
void* data = hyperion_malloc(1024);
if (!data) {
    printf("Memory allocation failed\n");
    return 1;
}

// Use memory
// ...

// Free memory
hyperion_free(data);

// Clean up
hyperion_destroy_memory_pool(pool);
```

### Memory Usage Monitoring
```c
// Enable memory tracking
HyperionMemoryConfig mem_config = {
    .track_usage = true
};
hyperion_configure_memory(&mem_config);

// Get memory statistics
HyperionMemoryStats stats;
hyperion_get_memory_stats(&stats);
printf("Total allocated: %zu bytes\n", stats.total_allocated);
printf("Peak usage: %zu bytes\n", stats.peak_usage);
printf("Current usage: %zu bytes\n", stats.current_usage);
```

## Related Documentation

- [Core API](core.md)
- [Performance Tools API](performance.md)
- [Models API](models.md)