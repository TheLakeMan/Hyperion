# Hyperion API Reference

## Overview

This document provides a comprehensive reference for the Hyperion API, covering all core components, their functions, and usage examples.

## Core Components

### Memory Management

```c
/**
 * Memory management configuration
 */
typedef struct {
    size_t initial_pool_size;    // Initial memory pool size
    size_t max_pool_size;        // Maximum memory pool size
    bool track_allocations;      // Whether to track allocations
    bool enable_optimization;    // Whether to enable memory optimization
} HyperionMemoryConfig;

/**
 * Initialize memory management
 */
bool hyperionInitMemory(const HyperionMemoryConfig* config);

/**
 * Allocate memory
 */
void* hyperionAlloc(size_t size);

/**
 * Free allocated memory
 */
void hyperionFree(void* ptr);

/**
 * Get memory statistics
 */
HyperionMemoryStats hyperionGetMemoryStats();
```

### I/O System

```c
/**
 * File operations
 */
bool hyperionFileExists(const char* path);
bool hyperionReadFile(const char* path, void* buffer, size_t size);
bool hyperionWriteFile(const char* path, const void* buffer, size_t size);

/**
 * Directory operations
 */
bool hyperionCreateDir(const char* path);
bool hyperionListDir(const char* path, char*** files, size_t* count);

/**
 * Path manipulation
 */
char* hyperionJoinPath(const char* path1, const char* path2);
char* hyperionGetDirName(const char* path);
char* hyperionGetBaseName(const char* path);
```

### Configuration System

```c
/**
 * Configuration structure
 */
typedef struct {
    const char* key;
    HyperionConfigType type;
    union {
        int int_value;
        float float_value;
        bool bool_value;
        char* string_value;
    } value;
} HyperionConfig;

/**
 * Load configuration
 */
bool hyperionLoadConfig(const char* path, HyperionConfig** config, size_t* count);

/**
 * Get configuration value
 */
bool hyperionGetConfigValue(const HyperionConfig* config, size_t count, 
                         const char* key, void* value);

/**
 * Set configuration value
 */
bool hyperionSetConfigValue(HyperionConfig* config, size_t count, 
                         const char* key, const void* value);
```

## Model Components

### Tokenizer

```c
/**
 * Tokenizer configuration
 */
typedef struct {
    const char* vocabulary_path;  // Path to vocabulary file
    size_t max_tokens;           // Maximum number of tokens
    bool case_sensitive;         // Whether to be case sensitive
} HyperionTokenizerConfig;

/**
 * Create tokenizer
 */
HyperionTokenizer* hyperionCreateTokenizer(const HyperionTokenizerConfig* config);

/**
 * Tokenize text
 */
bool hyperionTokenize(HyperionTokenizer* tokenizer, const char* text, 
                   int* tokens, size_t* count);

/**
 * Detokenize tokens
 */
bool hyperionDetokenize(HyperionTokenizer* tokenizer, const int* tokens, 
                     size_t count, char** text);
```

### Text Generation

```c
/**
 * Generation configuration
 */
typedef struct {
    int max_length;              // Maximum generation length
    float temperature;           // Sampling temperature
    int top_k;                   // Top-k sampling parameter
    float top_p;                 // Top-p sampling parameter
} HyperionGenerationConfig;

/**
 * Generate text
 */
bool hyperionGenerateText(HyperionModel* model, const char* prompt,
                       const HyperionGenerationConfig* config);
                       char** output);
```

## Optimization Components

### Memory Optimizer

```c
/**
 * Memory optimizer configuration
 */
typedef struct {
    bool enable_tensor_reuse;     // Whether to reuse tensor memory
    bool enable_in_place_ops;     // Whether to use in-place operations
    float memory_speed_tradeoff;  // Memory/speed tradeoff (0.0 to 1.0)
    size_t max_memory_budget;     // Maximum memory budget
} HyperionMemoryOptimizerConfig;

/**
 * Create memory optimizer
 */
HyperionMemoryOptimizer* hyperionCreateMemoryOptimizer(
    const HyperionMemoryOptimizerConfig* config);

/**
 * Optimize memory usage
 */
bool hyperionOptimizeMemoryUsage(HyperionMemoryOptimizer* optimizer,
                              HyperionModel* model,
                              size_t memory_budget);
```

### Progressive Loader

```c
/**
 * Progressive loader configuration
 */
typedef struct {
    bool enable_prefetch;         // Whether to prefetch next layers
    int prefetch_window;          // Number of layers to prefetch
    bool enable_adaptive_window;  // Whether to adapt prefetch window
    size_t max_memory_usage;      // Maximum memory usage
} HyperionProgressiveLoaderConfig;

/**
 * Create progressive loader
 */
HyperionProgressiveLoader* hyperionCreateProgressiveLoader(
    const HyperionProgressiveLoaderConfig* config);

/**
 * Load layer
 */
bool hyperionLoadLayer(HyperionProgressiveLoader* loader, int layer_index);
```

### Layer Scheduler

```c
/**
 * Layer scheduler configuration
 */
typedef struct {
    bool enable_checkpointing;    // Whether to use checkpointing
    float memory_speed_tradeoff;  // Memory/speed tradeoff
    bool recompute_activations;   // Whether to recompute activations
    size_t max_activation_memory; // Maximum activation memory
} HyperionLayerSchedulerConfig;

/**
 * Create layer scheduler
 */
HyperionLayerScheduler* hyperionCreateLayerScheduler(
    HyperionModel* model,
    const HyperionLayerSchedulerConfig* config);

/**
 * Execute layer with optimization
 */
bool hyperionExecuteLayerWithMemoryOptimization(
    HyperionLayerScheduler* scheduler,
    int layer_index,
    HyperionTensor* input,
    HyperionTensor* output);
```

## Utility Components

### Quantization

```c
/**
 * Quantization configuration
 */
typedef struct {
    int bits;                    // Number of bits (4 or 8)
    bool symmetric;              // Whether to use symmetric quantization
    float scale;                 // Quantization scale
    int zero_point;              // Quantization zero point
} HyperionQuantizationConfig;

/**
 * Quantize tensor
 */
bool hyperionQuantizeTensor(const HyperionTensor* input,
                         HyperionTensor* output,
                         const HyperionQuantizationConfig* config);

/**
 * Dequantize tensor
 */
bool hyperionDequantizeTensor(const HyperionTensor* input,
                           HyperionTensor* output);
```

### Sparse Operations

```c
/**
 * Sparse matrix configuration
 */
typedef struct {
    HyperionSparseFormat format;   // Sparse format (CSR, CSC, etc.)
    int block_size;              // Block size for blocked formats
    bool enable_simd;            // Whether to enable SIMD
} HyperionSparseConfig;

/**
 * Convert to sparse format
 */
bool hyperionConvertToSparse(const HyperionTensor* dense,
                          HyperionTensor* sparse,
                          const HyperionSparseConfig* config);

/**
 * Sparse matrix multiplication
 */
bool hyperionSparseMatMul(const HyperionTensor* a,
                       const HyperionTensor* b,
                       HyperionTensor* c);
```

## Error Handling

```c
/**
 * Error codes
 */
typedef enum {
    HYPERION_SUCCESS = 0,
    HYPERION_ERROR_MEMORY,
    HYPERION_ERROR_IO,
    HYPERION_ERROR_INVALID_ARGUMENT,
    HYPERION_ERROR_NOT_IMPLEMENTED,
    HYPERION_ERROR_RUNTIME
} HyperionErrorCode;

/**
 * Get error message
 */
const char* hyperionGetErrorMessage(HyperionErrorCode code);

/**
 * Set error handler
 */
void hyperionSetErrorHandler(HyperionErrorHandler handler);
```

## Usage Examples

### Basic Model Usage

```c
// Initialize memory management
HyperionMemoryConfig mem_config = {
    .initial_pool_size = 1024 * 1024 * 1024,  // 1GB
    .max_pool_size = 2 * 1024 * 1024 * 1024,  // 2GB
    .track_allocations = true,
    .enable_optimization = true
};
hyperionInitMemory(&mem_config);

// Load model
HyperionModel* model = hyperionLoadModel("model.hyperion");

// Create tokenizer
HyperionTokenizerConfig tokenizer_config = {
    .vocabulary_path = "vocabulary.txt",
    .max_tokens = 50000,
    .case_sensitive = false
};
HyperionTokenizer* tokenizer = hyperionCreateTokenizer(&tokenizer_config);

// Generate text
HyperionGenerationConfig gen_config = {
    .max_length = 100,
    .temperature = 0.7f,
    .top_k = 50,
    .top_p = 0.9f
};
char* output;
hyperionGenerateText(model, "Hello, ", &gen_config, &output);
```

### Memory Optimization

```c
// Create memory optimizer
HyperionMemoryOptimizerConfig opt_config = {
    .enable_tensor_reuse = true,
    .enable_in_place_ops = true,
    .memory_speed_tradeoff = 0.5f,
    .max_memory_budget = 512 * 1024 * 1024  // 512MB
};
HyperionMemoryOptimizer* optimizer = hyperionCreateMemoryOptimizer(&opt_config);

// Optimize model
hyperionOptimizeMemoryUsage(optimizer, model, 256 * 1024 * 1024);  // 256MB budget
```

### Progressive Loading

```c
// Create progressive loader
HyperionProgressiveLoaderConfig loader_config = {
    .enable_prefetch = true,
    .prefetch_window = 3,
    .enable_adaptive_window = true,
    .max_memory_usage = 256 * 1024 * 1024  // 256MB
};
HyperionProgressiveLoader* loader = hyperionCreateProgressiveLoader(&loader_config);

// Load layers as needed
hyperionLoadLayer(loader, 0);
hyperionLoadLayer(loader, 1);
```

## Best Practices

1. **Memory Management**
   - Initialize memory management first
   - Set appropriate memory budgets
   - Monitor memory usage
   - Use memory optimization features

2. **Model Usage**
   - Load models once and reuse
   - Use appropriate batch sizes
   - Enable memory optimization
   - Monitor performance

3. **Error Handling**
   - Check return values
   - Handle errors appropriately
   - Clean up resources
   - Log errors for debugging

4. **Performance**
   - Use appropriate configurations
   - Enable optimizations
   - Monitor performance
   - Profile memory usage

### Memory-Efficient Tensor Operations

```c
/**
 * Tensor memory allocation strategy
 */
typedef enum {
    HYPERION_TENSOR_STATIC,    // Static allocation
    HYPERION_TENSOR_POOLED,    // Pooled allocation
    HYPERION_TENSOR_STREAMING  // Streaming allocation
} HyperionTensorAllocStrategy;

/**
 * Tensor memory configuration
 */
typedef struct {
    HyperionTensorAllocStrategy strategy;  // Allocation strategy
    size_t pool_size;                    // Pool size for pooled strategy
    size_t stream_buffer_size;           // Buffer size for streaming
    bool enable_in_place;                // Whether to enable in-place ops
} HyperionTensorMemoryConfig;

/**
 * Create memory-efficient tensor
 */
HyperionTensor* hyperionCreateTensorWithMemoryConfig(
    const HyperionTensorMemoryConfig* config,
    int ndims,
    const int* dims,
    HyperionDataType dtype);

/**
 * Perform in-place tensor operation
 */
bool hyperionTensorInPlaceOp(
    HyperionTensor* tensor,
    HyperionTensorOp op,
    const void* params);

/**
 * Stream tensor data
 */
bool hyperionStreamTensorData(
    HyperionTensor* tensor,
    const void* data,
    size_t offset,
    size_t size);

/**
 * Get tensor memory statistics
 */
HyperionTensorMemoryStats hyperionGetTensorMemoryStats(
    const HyperionTensor* tensor);
```

### Performance Impact Assessment

```c
/**
 * Performance tracking configuration
 */
typedef struct {
    bool track_execution_time;   // Whether to track execution time
    bool track_memory_usage;     // Whether to track memory usage
    bool track_cpu_usage;        // Whether to track CPU usage
    bool track_cache_usage;      // Whether to track cache usage
    bool enable_optimization;    // Whether to enable optimization analysis
} HyperionPerformanceConfig;

/**
 * Performance metrics
 */
typedef struct {
    double execution_time_ms;    // Execution time in milliseconds
    size_t memory_usage_bytes;   // Memory usage in bytes
    float cpu_usage_percent;     // CPU usage percentage
    size_t cache_misses;         // Number of cache misses
    size_t cache_hits;          // Number of cache hits
} HyperionPerformanceMetrics;

/**
 * Optimization impact
 */
typedef struct {
    float speedup_factor;        // Speedup factor compared to baseline
    float memory_reduction;      // Memory reduction percentage
    float cpu_efficiency;        // CPU efficiency improvement
    char* recommendations;       // Optimization recommendations
} HyperionOptimizationImpact;

/**
 * Create performance analysis context
 */
HyperionPerformanceAnalysis* hyperionCreatePerformanceAnalysis(
    const HyperionPerformanceConfig* config);

/**
 * Record performance metrics
 */
bool hyperionRecordMetrics(
    HyperionPerformanceAnalysis* analysis,
    const HyperionPerformanceMetrics* metrics);

/**
 * Analyze optimization impact
 */
bool hyperionAnalyzeOptimizationImpact(
    HyperionPerformanceAnalysis* analysis,
    const HyperionPerformanceMetrics* baseline,
    const HyperionPerformanceMetrics* current);

/**
 * Generate performance report
 */
bool hyperionGeneratePerformanceReport(
    const HyperionPerformanceAnalysis* analysis,
    const char* output_path);

/**
 * Get performance trend
 */
float hyperionGetPerformanceTrend(
    const HyperionPerformanceAnalysis* analysis);
```