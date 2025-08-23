#include "memory_optimizer.h"
#include "memory_pool.h"
#include "tensor.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Memory optimizer structure
struct HyperionMemoryOptimizer {
    HyperionMemoryOptimizerConfig config;
    HyperionMemoryStats  stats;
    struct HyperionMemoryPool    *memory_pool;

    // Tensor reuse tracking
    struct HyperionTensor **reusable_tensors;
    int                   num_reusable_tensors;
    int                   max_reusable_tensors;

    // Memory tracking
    size_t current_memory_usage;
    size_t peak_memory_usage;
};

// Create a memory optimizer
struct HyperionMemoryOptimizer *hyperionCreateMemoryOptimizer(void)
{
    struct HyperionMemoryOptimizer *optimizer =
        (struct HyperionMemoryOptimizer *)malloc(sizeof(struct HyperionMemoryOptimizer));
    if (!optimizer)
        return NULL;

    // Initialize configuration
    optimizer->config.enable_tensor_reuse   = true;
    optimizer->config.memory_speed_tradeoff = 0.5f;
    optimizer->config.max_tensor_reuse      = 16;

    // Initialize statistics
    optimizer->stats.tensors_reused = 0;
    optimizer->stats.memory_saved   = 0;
    optimizer->stats.allocations    = 0;
    optimizer->stats.free_count = 0;

    // Create memory pool
    optimizer->memory_pool = hyperionCreateMemoryPool(1024 * 1024 * 1024); // 1GB default
    if (!optimizer->memory_pool) {
        free(optimizer);
        return NULL;
    }

    // Initialize tensor reuse tracking
    optimizer->max_reusable_tensors = optimizer->config.max_tensor_reuse;
    optimizer->reusable_tensors = (struct HyperionTensor **)calloc(optimizer->max_reusable_tensors,
                                                                 sizeof(struct HyperionTensor *));
    if (!optimizer->reusable_tensors) {
        hyperionFreeMemoryPool(optimizer->memory_pool);
        free(optimizer);
        return NULL;
    }

    optimizer->num_reusable_tensors = 0;
    optimizer->current_memory_usage = 0;
    optimizer->peak_memory_usage    = 0;

    return optimizer;
}

// Free memory optimizer resources
void hyperionFreeMemoryOptimizer(struct HyperionMemoryOptimizer *optimizer)
{
    if (!optimizer)
        return;

    // Free reusable tensors
    for (int i = 0; i < optimizer->num_reusable_tensors; i++) {
        if (optimizer->reusable_tensors[i]) {
            hyperionFreeTensor(optimizer->reusable_tensors[i]);
        }
    }

    free(optimizer->reusable_tensors);
    hyperionFreeMemoryPool(optimizer->memory_pool);
    free(optimizer);
}

// Get current memory optimizer statistics
HyperionMemoryStats hyperionGetMemoryOptimizerStats(struct HyperionMemoryOptimizer *optimizer)
{
    return optimizer ? optimizer->stats : (HyperionMemoryStats){0};
}

// Set memory/speed tradeoff
void hyperionSetMemorySpeedTradeoff(struct HyperionMemoryOptimizer *optimizer, float tradeoff)
{
    if (!optimizer)
        return;

    tradeoff                                = fmaxf(0.0f, fminf(1.0f, tradeoff));
    optimizer->config.memory_speed_tradeoff = tradeoff;
}

// Enable in-place operations for a layer
bool hyperionEnableInPlaceOperations(struct HyperionMemoryOptimizer *optimizer, int layer_index)
{
    if (!optimizer)
        return false;

    // Implementation depends on layer type and operation
    // For now, we'll just return true as a placeholder
    return true;
}

// Execute a function with tensor reuse
bool hyperionExecuteWithTensorReuse(struct HyperionMemoryOptimizer *optimizer,
                                  struct HyperionModel *model, int layer_index,
                                  struct HyperionTensor *input, struct HyperionTensor *output)
{
    if (!optimizer || !model || !input || !output)
        return false;

    // Check if we can reuse a tensor
    struct HyperionTensor *reusable_tensor = NULL;
    for (int i = 0; i < optimizer->num_reusable_tensors; i++) {
        if (optimizer->reusable_tensors[i] &&
            hyperionGetTensorSize(optimizer->reusable_tensors[i]) >= hyperionGetTensorSize(output)) {
            reusable_tensor                = optimizer->reusable_tensors[i];
            optimizer->reusable_tensors[i] = NULL;
            optimizer->stats.tensors_reused++;
            break;
        }
    }

    // If no reusable tensor found, create a new one
    if (!reusable_tensor) {
        reusable_tensor =
            hyperionCreateTensor(hyperionGetTensorShape(output), hyperionGetTensorDataType(output));
        if (!reusable_tensor)
            return false;
        optimizer->stats.allocations++;
    }

    // Execute the layer
    bool success = hyperionExecuteLayer(model, layer_index, input, reusable_tensor);
    if (!success) {
        if (reusable_tensor != output) {
            hyperionFreeTensor(reusable_tensor);
        }
        return false;
    }

    // Copy result to output if needed
    if (reusable_tensor != output) {
        hyperionCopyTensor(reusable_tensor, output);
    }

    // Store tensor for reuse if possible
    if (optimizer->config.enable_tensor_reuse &&
        optimizer->num_reusable_tensors < optimizer->max_reusable_tensors) {
        optimizer->reusable_tensors[optimizer->num_reusable_tensors++] = reusable_tensor;
        optimizer->stats.memory_saved += hyperionGetTensorSize(reusable_tensor);
    }
    else {
        hyperionFreeTensor(reusable_tensor);
        optimizer->stats.free_count++;
    }

    // Update memory usage
    optimizer->current_memory_usage = hyperionGetMemoryPoolUsage(optimizer->memory_pool);
    optimizer->peak_memory_usage =
        fmax(optimizer->peak_memory_usage, optimizer->current_memory_usage);

    return true;
}

// Optimize memory usage
bool hyperionOptimizeMemoryUsage(struct HyperionMemoryOptimizer *optimizer, size_t current_allocation,
                               size_t memory_budget)
{
    if (!optimizer)
        return false;

    // If we're under budget, no optimization needed
    if (current_allocation <= memory_budget)
        return true;

    // Calculate how much memory we need to free
    size_t memory_to_free = current_allocation - memory_budget;

    // Free reusable tensors until we're under budget
    while (memory_to_free > 0 && optimizer->num_reusable_tensors > 0) {
        int last_index = --optimizer->num_reusable_tensors;
        if (optimizer->reusable_tensors[last_index]) {
            size_t tensor_size = hyperionGetTensorSize(optimizer->reusable_tensors[last_index]);
            hyperionFreeTensor(optimizer->reusable_tensors[last_index]);
            optimizer->reusable_tensors[last_index] = NULL;
            memory_to_free = memory_to_free > tensor_size ? memory_to_free - tensor_size : 0;
            optimizer->stats.free_count++;
        }
    }

    return memory_to_free == 0;
}