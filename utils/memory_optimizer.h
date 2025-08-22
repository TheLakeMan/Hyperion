#ifndef HYPERION_MEMORY_OPTIMIZER_H
#define HYPERION_MEMORY_OPTIMIZER_H

#include <stdbool.h>
#include <stddef.h>

// Memory optimizer configuration
typedef struct {
    size_t max_memory_budget;     // Maximum memory to use in bytes
    bool   enable_checkpointing;  // Whether to use activation checkpointing
    float  memory_speed_tradeoff; // 0.0 (prioritize memory) to 1.0 (prioritize speed)
    bool   recompute_activations; // Whether to recompute rather than store activations
    size_t max_activation_memory; // Maximum memory for activations
} HyperionMemoryOptimizerConfig;

// Memory statistics
typedef struct {
    size_t total_allocated;    // Total memory allocated
    size_t current_allocated;  // Current memory in use
    size_t peak_allocated;     // Peak memory usage
    size_t allocation_count;   // Number of allocations
    size_t free_count;         // Number of frees
    size_t tensor_reuse_count; // Number of tensor reuses
    size_t memory_saved;       // Memory saved through optimizations
} HyperionMemoryStats;

// Memory optimizer handle
typedef struct HyperionMemoryOptimizer HyperionMemoryOptimizer;

// Create a memory optimizer
HyperionMemoryOptimizer *hyperionCreateMemoryOptimizer(const HyperionMemoryOptimizerConfig *config);

// Free memory optimizer
void hyperionFreeMemoryOptimizer(HyperionMemoryOptimizer *optimizer);

// Get memory statistics
HyperionMemoryStats hyperionGetMemoryOptimizerStats(const HyperionMemoryOptimizer *optimizer);

// Set memory/speed tradeoff
void hyperionSetMemorySpeedTradeoff(HyperionMemoryOptimizer *optimizer, float tradeoff);

// Enable in-place operations
bool hyperionEnableInPlaceOperations(HyperionMemoryOptimizer *optimizer, bool enable);

// Execute with tensor reuse
bool hyperionExecuteWithTensorReuse(HyperionMemoryOptimizer *optimizer, void *input, void *output);

// Optimize memory usage
bool hyperionOptimizeMemoryUsage(HyperionMemoryOptimizer *optimizer);

#endif // HYPERION_MEMORY_OPTIMIZER_H