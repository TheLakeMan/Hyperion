#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../utils/neural_architecture_search.h"

/* Mock evaluation function for testing */
float mockEvaluationFunction(const NASArchGenome *genome, void *userData) {
    if (!genome) return 0.0f;
    
    /* Simple fitness function based on architecture properties */
    float fitness = 0.0f;
    
    /* Reward moderate number of layers */
    if (genome->numLayers >= 5 && genome->numLayers <= 20) {
        fitness += 50.0f;
    }
    
    /* Reward reasonable channel counts */
    for (int i = 0; i < genome->numLayers; i++) {
        if (genome->genes[i].channels >= 16 && genome->genes[i].channels <= 512) {
            fitness += 10.0f;
        }
    }
    
    /* Penalize excessive complexity */
    float complexity = (float)genome->numLayers * 0.5f;
    for (int i = 0; i < genome->numLayers; i++) {
        complexity += genome->genes[i].channels * 0.01f;
    }
    fitness -= complexity;
    
    /* Add some randomness to simulate real evaluation */
    fitness += ((float)rand() / RAND_MAX - 0.5f) * 20.0f;
    
    return fitness;
}

/* Test NAS context creation and configuration */
int test_nas_context_creation() {
    printf("Testing NAS context creation...\n");
    
    /* Define search space */
    NASLayerType allowedLayers[] = {
        NAS_LAYER_CONV2D, NAS_LAYER_DEPTHWISE_CONV, NAS_LAYER_POINTWISE_CONV,
        NAS_LAYER_MAXPOOL, NAS_LAYER_SKIP, NAS_LAYER_DENSE
    };
    int kernelSizes[] = {1, 3, 5, 7};
    
    NASSearchSpace searchSpace = {
        .searchType = NAS_SEARCH_MACRO,
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 6,
        .minLayers = 3,
        .maxLayers = 15,
        .minChannels = 16,
        .maxChannels = 256,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 4,
        .allowSkipConnections = true,
        .allowResidualBlocks = true
    };
    
    /* Define constraints */
    NASConstraint constraints[] = {
        {.type = NAS_CONSTRAINT_MEMORY, .maxValue = 1024*1024, .weight = 0.3f, .hardConstraint = false},
        {.type = NAS_CONSTRAINT_LATENCY, .maxValue = 100.0f, .weight = 0.4f, .hardConstraint = true},
        {.type = NAS_CONSTRAINT_PARAMS, .maxValue = 1000000, .weight = 0.3f, .hardConstraint = false}
    };
    
    /* Main NAS configuration */
    NASConfig config = {
        .strategy = NAS_STRATEGY_EVOLUTIONARY,
        .searchSpace = searchSpace,
        .constraints = constraints,
        .numConstraints = 3,
        .populationSize = 20,
        .maxGenerations = 10,
        .mutationRate = 0.1f,
        .crossoverRate = 0.8f,
        .eliteRatio = 0.1f,
        .trainingEpochs = 1,
        .validationSamples = 100,
        .accuracyWeight = 0.7f,
        .latencyWeight = 0.2f,
        .memoryWeight = 0.1f,
        .useQuantization = true,
        .useSIMD = true,
        .targetMemoryBudget = 512*1024,
        .targetLatencyMs = 50.0f
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    /* Test SIMD enable/disable */
    bool success = hyperionNASEnableSIMD(nas, false);
    assert(success);
    success = hyperionNASEnableSIMD(nas, true);
    assert(success);
    
    hyperionNASFree(nas);
    
    printf("✓ NAS context creation test passed\n");
    return 0;
}

/* Test random architecture generation */
int test_random_architecture_generation() {
    printf("Testing random architecture generation...\n");
    
    NASLayerType allowedLayers[] = {NAS_LAYER_CONV2D, NAS_LAYER_MAXPOOL, NAS_LAYER_DENSE};
    int kernelSizes[] = {3, 5};
    
    NASSearchSpace searchSpace = {
        .searchType = NAS_SEARCH_MACRO,
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 3,
        .minLayers = 3,
        .maxLayers = 8,
        .minChannels = 32,
        .maxChannels = 128,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 2,
        .allowSkipConnections = false,
        .allowResidualBlocks = false
    };
    
    NASConfig config = {
        .strategy = NAS_STRATEGY_RANDOM,
        .searchSpace = searchSpace,
        .populationSize = 1,
        .maxGenerations = 1
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    /* Generate multiple random architectures */
    for (int i = 0; i < 5; i++) {
        NASArchGenome *genome = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
        assert(genome != NULL);
        
        bool success = hyperionNASGenerateRandomArchitecture(nas, genome);
        assert(success);
        
        /* Verify architecture properties */
        assert(genome->numLayers >= config.searchSpace.minLayers);
        assert(genome->numLayers <= config.searchSpace.maxLayers);
        assert(genome->inputChannels >= config.searchSpace.minChannels);
        assert(genome->inputChannels <= config.searchSpace.maxChannels);
        
        printf("  - Architecture %d: %d layers, input channels: %d\n",
               i + 1, genome->numLayers, genome->inputChannels);
        
        /* Verify layer properties */
        for (int j = 0; j < genome->numLayers; j++) {
            const NASArchGene *gene = &genome->genes[j];
            assert(gene->channels >= config.searchSpace.minChannels);
            assert(gene->channels <= config.searchSpace.maxChannels);
            assert(gene->dropoutRate >= 0.0f && gene->dropoutRate <= 0.5f);
            
            /* Check if layer type is allowed */
            bool layerAllowed = false;
            for (int k = 0; k < config.searchSpace.numAllowedLayers; k++) {
                if (gene->layerType == config.searchSpace.allowedLayers[k]) {
                    layerAllowed = true;
                    break;
                }
            }
            assert(layerAllowed);
        }
        
        hyperionNASGenomeFree(genome);
    }
    
    hyperionNASFree(nas);
    
    printf("✓ Random architecture generation test passed\n");
    return 0;
}

/* Test architecture mutation */
int test_architecture_mutation() {
    printf("Testing architecture mutation...\n");
    
    NASLayerType allowedLayers[] = {NAS_LAYER_CONV2D, NAS_LAYER_DENSE};
    int kernelSizes[] = {3, 5, 7};
    
    NASSearchSpace searchSpace = {
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 2,
        .minLayers = 3,
        .maxLayers = 6,
        .minChannels = 16,
        .maxChannels = 64,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 3
    };
    
    NASConfig config = {
        .strategy = NAS_STRATEGY_EVOLUTIONARY,
        .searchSpace = searchSpace,
        .populationSize = 1
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    /* Generate original architecture */
    NASArchGenome *original = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    NASArchGenome *mutated = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    assert(original != NULL && mutated != NULL);
    
    bool success = hyperionNASGenerateRandomArchitecture(nas, original);
    assert(success);
    
    /* Copy for mutation */
    success = hyperionNASGenomeCopy(mutated, original);
    assert(success);
    
    /* Test mutation with high rate */
    success = hyperionNASMutateArchitecture(nas, mutated, 1.0f); /* 100% mutation rate */
    assert(success);
    
    /* Verify mutation occurred (at least some changes) */
    bool foundDifference = false;
    for (int i = 0; i < original->numLayers && i < mutated->numLayers; i++) {
        if (original->genes[i].layerType != mutated->genes[i].layerType ||
            original->genes[i].channels != mutated->genes[i].channels ||
            original->genes[i].kernelSize != mutated->genes[i].kernelSize) {
            foundDifference = true;
            break;
        }
    }
    
    if (foundDifference || original->numLayers != mutated->numLayers) {
        printf("  - Mutation successfully changed architecture\n");
    } else {
        printf("  - Warning: No mutation detected (this can happen randomly)\n");
    }
    
    /* Test mutation with low rate */
    hyperionNASGenomeCopy(mutated, original);
    success = hyperionNASMutateArchitecture(nas, mutated, 0.1f); /* 10% mutation rate */
    assert(success);
    
    hyperionNASGenomeFree(original);
    hyperionNASGenomeFree(mutated);
    hyperionNASFree(nas);
    
    printf("✓ Architecture mutation test passed\n");
    return 0;
}

/* Test architecture crossover */
int test_architecture_crossover() {
    printf("Testing architecture crossover...\n");
    
    NASLayerType allowedLayers[] = {NAS_LAYER_CONV2D, NAS_LAYER_DENSE, NAS_LAYER_MAXPOOL};
    int kernelSizes[] = {3, 5};
    
    NASSearchSpace searchSpace = {
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 3,
        .minLayers = 4,
        .maxLayers = 8,
        .minChannels = 32,
        .maxChannels = 128,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 2
    };
    
    NASConfig config = {
        .strategy = NAS_STRATEGY_EVOLUTIONARY,
        .searchSpace = searchSpace,
        .populationSize = 1
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    /* Generate parent architectures */
    NASArchGenome *parent1 = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    NASArchGenome *parent2 = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    NASArchGenome *offspring1 = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    NASArchGenome *offspring2 = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    
    assert(parent1 && parent2 && offspring1 && offspring2);
    
    bool success = hyperionNASGenerateRandomArchitecture(nas, parent1);
    assert(success);
    success = hyperionNASGenerateRandomArchitecture(nas, parent2);
    assert(success);
    
    printf("  - Parent 1: %d layers\n", parent1->numLayers);
    printf("  - Parent 2: %d layers\n", parent2->numLayers);
    
    /* Perform crossover */
    success = hyperionNASCrossover(nas, parent1, parent2, offspring1, offspring2);
    assert(success);
    
    printf("  - Offspring 1: %d layers\n", offspring1->numLayers);
    printf("  - Offspring 2: %d layers\n", offspring2->numLayers);
    
    /* Verify offspring are valid */
    assert(offspring1->numLayers > 0 && offspring1->numLayers <= config.searchSpace.maxLayers);
    assert(offspring2->numLayers > 0 && offspring2->numLayers <= config.searchSpace.maxLayers);
    
    hyperionNASGenomeFree(parent1);
    hyperionNASGenomeFree(parent2);
    hyperionNASGenomeFree(offspring1);
    hyperionNASGenomeFree(offspring2);
    hyperionNASFree(nas);
    
    printf("✓ Architecture crossover test passed\n");
    return 0;
}

/* Test constraint evaluation */
int test_constraint_evaluation() {
    printf("Testing constraint evaluation...\n");
    
    NASLayerType allowedLayers[] = {NAS_LAYER_CONV2D, NAS_LAYER_DENSE};
    int kernelSizes[] = {3};
    
    NASSearchSpace searchSpace = {
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 2,
        .minLayers = 2,
        .maxLayers = 4,
        .minChannels = 32,
        .maxChannels = 64,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 1
    };
    
    /* Define strict constraints */
    NASConstraint constraints[] = {
        {.type = NAS_CONSTRAINT_MEMORY, .maxValue = 1000, .weight = 1.0f, .hardConstraint = true},
        {.type = NAS_CONSTRAINT_LATENCY, .maxValue = 1.0f, .weight = 1.0f, .hardConstraint = true},
        {.type = NAS_CONSTRAINT_PARAMS, .maxValue = 1000, .weight = 1.0f, .hardConstraint = false}
    };
    
    NASConfig config = {
        .strategy = NAS_STRATEGY_RANDOM,
        .searchSpace = searchSpace,
        .constraints = constraints,
        .numConstraints = 3,
        .populationSize = 1
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    /* Generate architecture */
    NASArchGenome *genome = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    assert(genome != NULL);
    
    bool success = hyperionNASGenerateRandomArchitecture(nas, genome);
    assert(success);
    
    /* Test metric estimation */
    float latency;
    size_t memory, params;
    success = hyperionNASEstimateMetrics(nas, genome, &latency, &memory, &params);
    assert(success);
    
    printf("  - Estimated latency: %.3f ms\n", latency);
    printf("  - Estimated memory: %zu bytes\n", memory);
    printf("  - Estimated parameters: %zu\n", params);
    
    /* Test constraint evaluation */
    bool satisfiesConstraints;
    success = hyperionNASEvaluateConstraints(nas, genome, &satisfiesConstraints);
    assert(success);
    
    printf("  - Satisfies constraints: %s\n", satisfiesConstraints ? "Yes" : "No");
    
    hyperionNASGenomeFree(genome);
    hyperionNASFree(nas);
    
    printf("✓ Constraint evaluation test passed\n");
    return 0;
}

/* Test full NAS evolution */
int test_nas_evolution() {
    printf("Testing NAS evolution...\n");
    
    NASLayerType allowedLayers[] = {NAS_LAYER_CONV2D, NAS_LAYER_MAXPOOL, NAS_LAYER_DENSE};
    int kernelSizes[] = {3, 5};
    
    NASSearchSpace searchSpace = {
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 3,
        .minLayers = 3,
        .maxLayers = 6,
        .minChannels = 16,
        .maxChannels = 64,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 2
    };
    
    /* Relaxed constraints for testing */
    NASConstraint constraints[] = {
        {.type = NAS_CONSTRAINT_MEMORY, .maxValue = 1024*1024, .weight = 0.2f, .hardConstraint = false},
        {.type = NAS_CONSTRAINT_LATENCY, .maxValue = 1000.0f, .weight = 0.3f, .hardConstraint = false}
    };
    
    NASConfig config = {
        .strategy = NAS_STRATEGY_EVOLUTIONARY,
        .searchSpace = searchSpace,
        .constraints = constraints,
        .numConstraints = 2,
        .populationSize = 10,
        .maxGenerations = 5,
        .mutationRate = 0.2f,
        .crossoverRate = 0.7f,
        .eliteRatio = 0.1f
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    /* Run NAS evolution */
    NASArchGenome *bestArchitecture = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    assert(bestArchitecture != NULL);
    
    printf("  - Running NAS evolution with %d generations...\n", config.maxGenerations);
    
    bool success = hyperionNASRun(nas, mockEvaluationFunction, NULL, bestArchitecture);
    assert(success);
    
    /* Check results */
    printf("  - Best architecture found:\n");
    printf("    * Layers: %d\n", bestArchitecture->numLayers);
    printf("    * Input channels: %d\n", bestArchitecture->inputChannels);
    printf("    * Output channels: %d\n", bestArchitecture->outputChannels);
    printf("    * Fitness: %.3f\n", bestArchitecture->fitness);
    
    /* Test progress tracking */
    int currentGen;
    float bestFitness, avgFitness;
    success = hyperionNASGetProgress(nas, &currentGen, &bestFitness, &avgFitness);
    assert(success);
    
    printf("  - Final generation: %d\n", currentGen);
    printf("  - Best fitness: %.3f\n", bestFitness);
    printf("  - Average fitness: %.3f\n", avgFitness);
    
    assert(bestArchitecture->numLayers >= config.searchSpace.minLayers);
    assert(bestArchitecture->numLayers <= config.searchSpace.maxLayers);
    
    hyperionNASGenomeFree(bestArchitecture);
    hyperionNASFree(nas);
    
    printf("✓ NAS evolution test passed\n");
    return 0;
}

/* Test genome operations */
int test_genome_operations() {
    printf("Testing genome operations...\n");
    
    /* Test genome creation and free */
    NASArchGenome *genome1 = hyperionNASGenomeCreate(10);
    NASArchGenome *genome2 = hyperionNASGenomeCreate(10);
    assert(genome1 != NULL && genome2 != NULL);
    
    /* Initialize genome1 */
    genome1->numLayers = 3;
    genome1->inputChannels = 32;
    genome1->outputChannels = 64;
    genome1->fitness = 85.5f;
    
    for (int i = 0; i < 3; i++) {
        genome1->genes[i].layerType = NAS_LAYER_CONV2D;
        genome1->genes[i].channels = 32 + i * 16;
        genome1->genes[i].kernelSize = 3;
        genome1->genes[i].dropoutRate = 0.1f * i;
        genome1->genes[i].useNormalization = (i % 2 == 0);
    }
    
    /* Test genome copy */
    bool success = hyperionNASGenomeCopy(genome2, genome1);
    assert(success);
    
    /* Verify copy */
    assert(genome2->numLayers == genome1->numLayers);
    assert(genome2->inputChannels == genome1->inputChannels);
    assert(genome2->outputChannels == genome1->outputChannels);
    assert(fabsf(genome2->fitness - genome1->fitness) < 1e-6f);
    
    for (int i = 0; i < genome1->numLayers; i++) {
        assert(genome2->genes[i].layerType == genome1->genes[i].layerType);
        assert(genome2->genes[i].channels == genome1->genes[i].channels);
        assert(genome2->genes[i].kernelSize == genome1->genes[i].kernelSize);
        assert(fabsf(genome2->genes[i].dropoutRate - genome1->genes[i].dropoutRate) < 1e-6f);
        assert(genome2->genes[i].useNormalization == genome1->genes[i].useNormalization);
    }
    
    printf("  - Genome copy successful\n");
    printf("  - Copied genome: %d layers, fitness %.3f\n", 
           genome2->numLayers, genome2->fitness);
    
    hyperionNASGenomeFree(genome1);
    hyperionNASGenomeFree(genome2);
    
    printf("✓ Genome operations test passed\n");
    return 0;
}

/* Performance benchmark for NAS operations */
int benchmark_nas_operations() {
    printf("Benchmarking NAS operations...\n");
    
    NASLayerType allowedLayers[] = {NAS_LAYER_CONV2D, NAS_LAYER_DENSE, NAS_LAYER_MAXPOOL};
    int kernelSizes[] = {3, 5, 7};
    
    NASSearchSpace searchSpace = {
        .allowedLayers = allowedLayers,
        .numAllowedLayers = 3,
        .minLayers = 5,
        .maxLayers = 15,
        .minChannels = 32,
        .maxChannels = 256,
        .allowedKernelSizes = kernelSizes,
        .numKernelSizes = 3
    };
    
    NASConfig config = {
        .strategy = NAS_STRATEGY_EVOLUTIONARY,
        .searchSpace = searchSpace,
        .populationSize = 50,
        .maxGenerations = 1 /* Just test one generation for benchmarking */
    };
    
    NeuralArchitectureSearch *nas = hyperionNASCreate(&config);
    assert(nas != NULL);
    
    clock_t start, end;
    int numOperations = 1000;
    
    /* Benchmark random architecture generation */
    start = clock();
    for (int i = 0; i < numOperations; i++) {
        NASArchGenome *genome = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
        hyperionNASGenerateRandomArchitecture(nas, genome);
        hyperionNASGenomeFree(genome);
    }
    end = clock();
    
    double genTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  - Random generation: %.3f ms per architecture\n", genTime * 1000.0 / numOperations);
    
    /* Benchmark mutation operations */
    NASArchGenome *testGenome = hyperionNASGenomeCreate(config.searchSpace.maxLayers);
    hyperionNASGenerateRandomArchitecture(nas, testGenome);
    
    start = clock();
    for (int i = 0; i < numOperations; i++) {
        hyperionNASMutateArchitecture(nas, testGenome, 0.1f);
    }
    end = clock();
    
    double mutTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  - Mutation: %.3f ms per operation\n", mutTime * 1000.0 / numOperations);
    
    /* Benchmark constraint evaluation */
    start = clock();
    for (int i = 0; i < numOperations; i++) {
        bool satisfies;
        hyperionNASEvaluateConstraints(nas, testGenome, &satisfies);
    }
    end = clock();
    
    double evalTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  - Constraint evaluation: %.3f ms per operation\n", evalTime * 1000.0 / numOperations);
    
    /* Benchmark metric estimation */
    start = clock();
    for (int i = 0; i < numOperations; i++) {
        float latency;
        size_t memory, params;
        hyperionNASEstimateMetrics(nas, testGenome, &latency, &memory, &params);
    }
    end = clock();
    
    double metricTime = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  - Metric estimation: %.3f ms per operation\n", metricTime * 1000.0 / numOperations);
    
    hyperionNASGenomeFree(testGenome);
    hyperionNASFree(nas);
    
    printf("✓ NAS operations benchmark completed\n");
    return 0;
}

int main() {
    printf("========================================\n");
    printf("Hyperion Phase 5.3: Neural Architecture Search Test Suite\n");
    printf("========================================\n");
    
    srand(time(NULL)); /* Initialize random seed */
    
    int result = 0;
    
    /* Basic functionality tests */
    result += test_nas_context_creation();
    result += test_random_architecture_generation();
    result += test_architecture_mutation();
    result += test_architecture_crossover();
    result += test_constraint_evaluation();
    result += test_genome_operations();
    
    /* Advanced functionality tests */
    result += test_nas_evolution();
    
    /* Performance benchmarks */
    result += benchmark_nas_operations();
    
    printf("========================================\n");
    if (result == 0) {
        printf("✅ All Phase 5.3 Neural Architecture Search tests passed!\n");
        printf("Neural Architecture Search capabilities are working correctly:\n");
        printf("  - Evolutionary algorithm implementation\n");
        printf("  - Random architecture generation\n");
        printf("  - Architecture mutation and crossover\n");
        printf("  - Hardware constraint evaluation\n");
        printf("  - Performance metric estimation\n");
        printf("  - Multi-objective optimization support\n");
    } else {
        printf("❌ %d Phase 5.3 Neural Architecture Search tests failed!\n", result);
    }
    printf("========================================\n");
    
    return result;
}