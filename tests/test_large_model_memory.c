#include "utils/benchmark.h"
#include "utils/memory_optimizer.h"
#include "utils/memory_pool.h"
#include "utils/progressive_loader.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test configuration
#define TEST_MODEL_PATH "data/large_model.bin"
#define TEST_MEMORY_BUDGET (500 * 1024 * 1024) // 500MB
#define TEST_LAYER_COUNT 24

// Test memory optimizer creation and initialization
static void test_optimizer_creation(void)
{
    HyperionMemoryOptimizerConfig config = {.max_memory_budget     = TEST_MEMORY_BUDGET,
                                          .enable_checkpointing  = true,
                                          .memory_speed_tradeoff = 0.5f,
                                          .recompute_activations = true,
                                          .max_activation_memory = TEST_MEMORY_BUDGET / 2};

    HyperionMemoryOptimizer *optimizer = hyperionCreateMemoryOptimizer(&config);
    assert(optimizer != NULL);
    assert(optimizer->config.max_memory_budget == TEST_MEMORY_BUDGET);
    assert(optimizer->config.enable_checkpointing == true);

    hyperionFreeMemoryOptimizer(optimizer);
    printf("✅ test_optimizer_creation passed\n");
}

// Test memory-aware execution planning
static void test_execution_planning(void)
{
    HyperionMemoryOptimizerConfig config = {.max_memory_budget     = TEST_MEMORY_BUDGET,
                                          .enable_checkpointing  = true,
                                          .memory_speed_tradeoff = 0.5f,
                                          .recompute_activations = true,
                                          .max_activation_memory = TEST_MEMORY_BUDGET / 2};

    HyperionMemoryOptimizer *optimizer = hyperionCreateMemoryOptimizer(&config);
    assert(optimizer != NULL);

    // Create a test model with known memory requirements
    HyperionModel *model = hyperionCreateTestModel(TEST_LAYER_COUNT);
    assert(model != NULL);

    // Generate memory-optimized execution plan
    HyperionExecutionPlan *plan = hyperionCreateMemoryOptimizedExecutionPlan(optimizer, model);
    assert(plan != NULL);
    assert(plan->layer_count == TEST_LAYER_COUNT);

    // Verify memory estimates
    HyperionMemoryEstimate estimate = hyperionEstimateMemoryUsage(optimizer, model);
    assert(estimate.peak_memory <= TEST_MEMORY_BUDGET);
    assert(estimate.average_memory <= TEST_MEMORY_BUDGET * 0.8);

    hyperionFreeExecutionPlan(plan);
    hyperionFreeModel(model);
    hyperionFreeMemoryOptimizer(optimizer);
    printf("✅ test_execution_planning passed\n");
}

// Test activation checkpointing
static void test_checkpointing(void)
{
    HyperionMemoryOptimizerConfig config = {.max_memory_budget     = TEST_MEMORY_BUDGET,
                                          .enable_checkpointing  = true,
                                          .memory_speed_tradeoff = 0.5f,
                                          .recompute_activations = true,
                                          .max_activation_memory = TEST_MEMORY_BUDGET / 2};

    HyperionMemoryOptimizer *optimizer = hyperionCreateMemoryOptimizer(&config);
    assert(optimizer != NULL);

    // Create test activation
    HyperionTensor *activation = hyperionCreateTestTensor(1000, 1000);
    assert(activation != NULL);

    // Create checkpoint
    HyperionCheckpoint *checkpoint = hyperionCreateActivationCheckpoint(optimizer, activation);
    assert(checkpoint != NULL);

    // Restore from checkpoint
    HyperionTensor *restored = hyperionRestoreFromCheckpoint(optimizer, checkpoint);
    assert(restored != NULL);
    assert(hyperionCompareTensors(activation, restored) == true);

    hyperionFreeTensor(restored);
    hyperionFreeCheckpoint(checkpoint);
    hyperionFreeTensor(activation);
    hyperionFreeMemoryOptimizer(optimizer);
    printf("✅ test_checkpointing passed\n");
}

// Test memory/speed tradeoff
static void test_memory_speed_tradeoff(void)
{
    HyperionMemoryOptimizerConfig config = {.max_memory_budget     = TEST_MEMORY_BUDGET,
                                          .enable_checkpointing  = true,
                                          .memory_speed_tradeoff = 0.5f,
                                          .recompute_activations = true,
                                          .max_activation_memory = TEST_MEMORY_BUDGET / 2};

    HyperionMemoryOptimizer *optimizer = hyperionCreateMemoryOptimizer(&config);
    assert(optimizer != NULL);

    // Test different tradeoff values
    for (float tradeoff = 0.0f; tradeoff <= 1.0f; tradeoff += 0.2f) {
        hyperionSetMemorySpeedTradeoff(optimizer, tradeoff);

        HyperionModel *model = hyperionCreateTestModel(TEST_LAYER_COUNT);
        assert(model != NULL);

        HyperionMemoryEstimate estimate = hyperionEstimateMemoryUsage(optimizer, model);
        assert(estimate.peak_memory <= TEST_MEMORY_BUDGET);

        // Higher tradeoff should result in lower memory usage
        if (tradeoff > 0.0f) {
            assert(estimate.peak_memory < TEST_MEMORY_BUDGET);
        }

        hyperionFreeModel(model);
    }

    hyperionFreeMemoryOptimizer(optimizer);
    printf("✅ test_memory_speed_tradeoff passed\n");
}

// Test tensor reuse strategies
static void test_tensor_reuse(void)
{
    HyperionMemoryOptimizerConfig config = {.max_memory_budget     = TEST_MEMORY_BUDGET,
                                          .enable_checkpointing  = true,
                                          .memory_speed_tradeoff = 0.5f,
                                          .recompute_activations = true,
                                          .max_activation_memory = TEST_MEMORY_BUDGET / 2};

    HyperionMemoryOptimizer *optimizer = hyperionCreateMemoryOptimizer(&config);
    assert(optimizer != NULL);

    // Create test tensors
    HyperionTensor *input  = hyperionCreateTestTensor(1000, 1000);
    HyperionTensor *output = hyperionCreateTestTensor(1000, 1000);
    assert(input != NULL && output != NULL);

    // Enable in-place operations
    assert(hyperionEnableInPlaceOperations(optimizer, true) == true);

    // Perform operation with tensor reuse
    assert(hyperionExecuteWithTensorReuse(optimizer, input, output) == true);

    // Verify memory usage is reduced
    HyperionMemoryStats stats = hyperionGetMemoryOptimizerStats(optimizer);
    assert(stats.tensor_reuse_count > 0);
    assert(stats.memory_saved > 0);

    hyperionFreeTensor(output);
    hyperionFreeTensor(input);
    hyperionFreeMemoryOptimizer(optimizer);
    printf("✅ test_tensor_reuse passed\n");
}

// Performance benchmark
static void benchmark_memory_optimization(void)
{
    HyperionMemoryOptimizerConfig config = {.max_memory_budget     = TEST_MEMORY_BUDGET,
                                          .enable_checkpointing  = true,
                                          .memory_speed_tradeoff = 0.5f,
                                          .recompute_activations = true,
                                          .max_activation_memory = TEST_MEMORY_BUDGET / 2};

    HyperionMemoryOptimizer *optimizer = hyperionCreateMemoryOptimizer(&config);
    assert(optimizer != NULL);

    // Benchmark memory optimization
    HyperionBenchmarkResult result = hyperionBenchmarkOperation(
        "Memory Optimization", 100,
        (HyperionBenchmarkOperation){.setup     = NULL,
                                   .operation = (void (*)(void *))hyperionOptimizeMemoryUsage,
                                   .teardown  = NULL,
                                   .context   = optimizer});

    printf("Memory Optimization Performance:\n");
    printf("  Average time: %.2f ms\n", result.average_time_ms);
    printf("  Min time: %.2f ms\n", result.min_time_ms);
    printf("  Max time: %.2f ms\n", result.max_time_ms);
    printf("  Standard deviation: %.2f ms\n", result.std_dev_ms);

    hyperionFreeMemoryOptimizer(optimizer);
    printf("✅ benchmark_memory_optimization completed\n");
}

int main(void)
{
    printf("Starting Large Model Memory Optimization Tests...\n");

    test_optimizer_creation();
    test_execution_planning();
    test_checkpointing();
    test_memory_speed_tradeoff();
    test_tensor_reuse();
    benchmark_memory_optimization();

    printf("All Large Model Memory Optimization Tests Passed!\n");
    return 0;
}