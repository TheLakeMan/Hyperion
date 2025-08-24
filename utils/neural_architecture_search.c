#include "neural_architecture_search.h"
#include "../core/memory.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/**
 * Neural Architecture Search context structure
 */
struct NeuralArchitectureSearch {
    NASConfig config;
    
    /* Population for evolutionary search */
    NASArchGenome *population;
    float *fitnessScores;
    int currentGeneration;
    
    /* Search statistics */
    float bestFitness;
    float avgFitness;
    int evaluationCount;
    
    /* Random number generation state */
    unsigned int randomState;
    
    bool initialized;
};

/* Helper function for random number generation */
static float randomFloat(NeuralArchitectureSearch *nas) {
    nas->randomState = nas->randomState * 1103515245 + 12345;
    return (nas->randomState / 65536 % 32768) / 32767.0f;
}

static int randomInt(NeuralArchitectureSearch *nas, int max) {
    return (int)(randomFloat(nas) * max);
}

/* Helper function to estimate layer computational cost */
static float estimateLayerCost(const NASArchGene *gene, int inputSize) {
    switch (gene->layerType) {
        case NAS_LAYER_CONV2D:
            return (float)(gene->channels * gene->kernelSize * gene->kernelSize * inputSize);
        case NAS_LAYER_DEPTHWISE_CONV:
            return (float)(gene->kernelSize * gene->kernelSize * inputSize + gene->channels * inputSize);
        case NAS_LAYER_POINTWISE_CONV:
            return (float)(gene->channels * inputSize);
        case NAS_LAYER_DENSE:
            return (float)(gene->channels * inputSize);
        case NAS_LAYER_ATTENTION:
            return (float)(inputSize * inputSize + 3 * gene->channels * inputSize);
        case NAS_LAYER_MAXPOOL:
        case NAS_LAYER_AVGPOOL:
            return (float)(gene->kernelSize * gene->kernelSize * inputSize);
        case NAS_LAYER_SKIP:
        case NAS_LAYER_NORMALIZE:
        case NAS_LAYER_ACTIVATION:
            return (float)inputSize;
        default:
            return 0.0f;
    }
}

/* Helper function to estimate layer memory usage */
static size_t estimateLayerMemory(const NASArchGene *gene, int inputSize) {
    size_t paramMemory = 0;
    size_t activationMemory = inputSize * sizeof(float);
    
    switch (gene->layerType) {
        case NAS_LAYER_CONV2D:
            paramMemory = gene->channels * gene->kernelSize * gene->kernelSize * sizeof(float);
            if (gene->useNormalization) {
                paramMemory += gene->channels * 2 * sizeof(float); /* gamma, beta */
            }
            break;
        case NAS_LAYER_DEPTHWISE_CONV:
            paramMemory = gene->kernelSize * gene->kernelSize * sizeof(float);
            break;
        case NAS_LAYER_POINTWISE_CONV:
        case NAS_LAYER_DENSE:
            paramMemory = gene->channels * inputSize * sizeof(float);
            break;
        case NAS_LAYER_ATTENTION:
            paramMemory = 3 * gene->channels * inputSize * sizeof(float); /* Q, K, V matrices */
            break;
        default:
            paramMemory = 0;
    }
    
    return paramMemory + activationMemory;
}

NeuralArchitectureSearch *hyperionNASCreate(const NASConfig *config) {
    if (!config) return NULL;
    
    NeuralArchitectureSearch *nas = (NeuralArchitectureSearch*)hyperionCalloc(1, sizeof(NeuralArchitectureSearch));
    if (!nas) return NULL;
    
    /* Copy configuration */
    nas->config = *config;
    
    /* Initialize population for evolutionary search */
    if (config->strategy == NAS_STRATEGY_EVOLUTIONARY || config->strategy == NAS_STRATEGY_RANDOM) {
        nas->population = (NASArchGenome*)hyperionCalloc(config->populationSize, sizeof(NASArchGenome));
        nas->fitnessScores = (float*)hyperionCalloc(config->populationSize, sizeof(float));
        
        if (!nas->population || !nas->fitnessScores) {
            hyperionNASFree(nas);
            return NULL;
        }
        
        /* Initialize each genome in population */
        for (int i = 0; i < config->populationSize; i++) {
            nas->population[i].genes = (NASArchGene*)hyperionCalloc(config->searchSpace.maxLayers, sizeof(NASArchGene));
            if (!nas->population[i].genes) {
                hyperionNASFree(nas);
                return NULL;
            }
        }
    }
    
    /* Initialize random state */
    nas->randomState = (unsigned int)time(NULL);
    nas->currentGeneration = 0;
    nas->bestFitness = -1e9f;
    nas->avgFitness = 0.0f;
    nas->evaluationCount = 0;
    
    nas->initialized = true;
    return nas;
}

void hyperionNASFree(NeuralArchitectureSearch *nas) {
    if (!nas) return;
    
    if (nas->population) {
        for (int i = 0; i < nas->config.populationSize; i++) {
            hyperionFree(nas->population[i].genes);
        }
        hyperionFree(nas->population);
    }
    
    hyperionFree(nas->fitnessScores);
    hyperionFree(nas);
}

bool hyperionNASGenerateRandomArchitecture(NeuralArchitectureSearch *nas,
                                         NASArchGenome *genome) {
    if (!nas || !genome) return false;
    
    const NASSearchSpace *space = &nas->config.searchSpace;
    
    /* Generate random number of layers */
    int numLayers = space->minLayers + randomInt(nas, space->maxLayers - space->minLayers + 1);
    genome->numLayers = numLayers;
    
    /* Set input/output channels */
    genome->inputChannels = space->minChannels + randomInt(nas, space->maxChannels - space->minChannels + 1);
    genome->outputChannels = space->minChannels + randomInt(nas, space->maxChannels - space->minChannels + 1);
    
    /* Generate random layers */
    for (int i = 0; i < numLayers; i++) {
        NASArchGene *gene = &genome->genes[i];
        
        /* Random layer type */
        int layerIdx = randomInt(nas, space->numAllowedLayers);
        gene->layerType = space->allowedLayers[layerIdx];
        
        /* Random parameters based on layer type */
        gene->channels = space->minChannels + randomInt(nas, space->maxChannels - space->minChannels + 1);
        
        if (gene->layerType == NAS_LAYER_CONV2D || gene->layerType == NAS_LAYER_DEPTHWISE_CONV ||
            gene->layerType == NAS_LAYER_MAXPOOL || gene->layerType == NAS_LAYER_AVGPOOL) {
            int kernelIdx = randomInt(nas, space->numKernelSizes);
            gene->kernelSize = space->allowedKernelSizes[kernelIdx];
            gene->stride = 1 + randomInt(nas, 2); /* 1 or 2 */
            gene->padding = gene->kernelSize / 2; /* Valid padding */
        } else {
            gene->kernelSize = 1;
            gene->stride = 1;
            gene->padding = 0;
        }
        
        gene->dropoutRate = randomFloat(nas) * 0.5f; /* 0 to 0.5 */
        gene->useNormalization = randomFloat(nas) > 0.5f;
        gene->activationFunction = randomInt(nas, 4); /* ReLU, Sigmoid, Tanh, Swish */
    }
    
    /* Initialize fitness metrics */
    genome->fitness = 0.0f;
    genome->accuracy = 0.0f;
    genome->latency = 0.0f;
    genome->memoryUsage = 0;
    genome->parameterCount = 0;
    
    return true;
}

bool hyperionNASMutateArchitecture(NeuralArchitectureSearch *nas,
                                 NASArchGenome *genome, float mutationRate) {
    if (!nas || !genome) return false;
    
    const NASSearchSpace *space = &nas->config.searchSpace;
    
    for (int i = 0; i < genome->numLayers; i++) {
        if (randomFloat(nas) < mutationRate) {
            NASArchGene *gene = &genome->genes[i];
            
            /* Mutate layer type */
            if (randomFloat(nas) < 0.3f) {
                int layerIdx = randomInt(nas, space->numAllowedLayers);
                gene->layerType = space->allowedLayers[layerIdx];
            }
            
            /* Mutate channels */
            if (randomFloat(nas) < 0.5f) {
                gene->channels = space->minChannels + randomInt(nas, space->maxChannels - space->minChannels + 1);
            }
            
            /* Mutate kernel size */
            if (randomFloat(nas) < 0.4f && space->numKernelSizes > 0) {
                int kernelIdx = randomInt(nas, space->numKernelSizes);
                gene->kernelSize = space->allowedKernelSizes[kernelIdx];
            }
            
            /* Mutate other parameters */
            if (randomFloat(nas) < 0.3f) {
                gene->dropoutRate = randomFloat(nas) * 0.5f;
            }
            
            if (randomFloat(nas) < 0.3f) {
                gene->useNormalization = !gene->useNormalization;
            }
        }
    }
    
    return true;
}

bool hyperionNASCrossover(NeuralArchitectureSearch *nas,
                         const NASArchGenome *parent1,
                         const NASArchGenome *parent2,
                         NASArchGenome *offspring1,
                         NASArchGenome *offspring2) {
    if (!nas || !parent1 || !parent2 || !offspring1 || !offspring2) return false;
    
    /* Single-point crossover */
    int crossoverPoint1 = randomInt(nas, parent1->numLayers);
    int crossoverPoint2 = randomInt(nas, parent2->numLayers);
    
    /* Create offspring 1 */
    offspring1->numLayers = crossoverPoint1 + (parent2->numLayers - crossoverPoint2);
    offspring1->inputChannels = parent1->inputChannels;
    offspring1->outputChannels = parent2->outputChannels;
    
    /* Copy first part from parent1 */
    for (int i = 0; i < crossoverPoint1; i++) {
        offspring1->genes[i] = parent1->genes[i];
    }
    
    /* Copy second part from parent2 */
    for (int i = crossoverPoint2; i < parent2->numLayers; i++) {
        if (crossoverPoint1 + (i - crossoverPoint2) < nas->config.searchSpace.maxLayers) {
            offspring1->genes[crossoverPoint1 + (i - crossoverPoint2)] = parent2->genes[i];
        }
    }
    
    /* Create offspring 2 (reverse) */
    offspring2->numLayers = crossoverPoint2 + (parent1->numLayers - crossoverPoint1);
    offspring2->inputChannels = parent2->inputChannels;
    offspring2->outputChannels = parent1->outputChannels;
    
    /* Copy first part from parent2 */
    for (int i = 0; i < crossoverPoint2; i++) {
        offspring2->genes[i] = parent2->genes[i];
    }
    
    /* Copy second part from parent1 */
    for (int i = crossoverPoint1; i < parent1->numLayers; i++) {
        if (crossoverPoint2 + (i - crossoverPoint1) < nas->config.searchSpace.maxLayers) {
            offspring2->genes[crossoverPoint2 + (i - crossoverPoint1)] = parent1->genes[i];
        }
    }
    
    return true;
}

bool hyperionNASEstimateMetrics(NeuralArchitectureSearch *nas,
                               const NASArchGenome *genome,
                               float *estimatedLatency,
                               size_t *estimatedMemory,
                               size_t *estimatedParams) {
    if (!nas || !genome || !estimatedLatency || !estimatedMemory || !estimatedParams) {
        return false;
    }
    
    float totalLatency = 0.0f;
    size_t totalMemory = 0;
    size_t totalParams = 0;
    
    int currentChannels = genome->inputChannels;
    
    for (int i = 0; i < genome->numLayers; i++) {
        const NASArchGene *gene = &genome->genes[i];
        
        /* Estimate computational cost (affects latency) */
        float layerCost = estimateLayerCost(gene, currentChannels);
        totalLatency += layerCost * 1e-6f; /* Convert to approximate milliseconds */
        
        /* Estimate memory usage */
        size_t layerMemory = estimateLayerMemory(gene, currentChannels);
        totalMemory += layerMemory;
        
        /* Estimate parameter count */
        switch (gene->layerType) {
            case NAS_LAYER_CONV2D:
                totalParams += gene->channels * currentChannels * gene->kernelSize * gene->kernelSize;
                currentChannels = gene->channels;
                break;
            case NAS_LAYER_DEPTHWISE_CONV:
                totalParams += currentChannels * gene->kernelSize * gene->kernelSize;
                break;
            case NAS_LAYER_POINTWISE_CONV:
                totalParams += gene->channels * currentChannels;
                currentChannels = gene->channels;
                break;
            case NAS_LAYER_DENSE:
                totalParams += gene->channels * currentChannels;
                currentChannels = gene->channels;
                break;
            case NAS_LAYER_ATTENTION:
                totalParams += 3 * gene->channels * currentChannels; /* Q, K, V */
                currentChannels = gene->channels;
                break;
            default:
                /* No parameters for pooling, skip, etc. */
                break;
        }
        
        /* Add normalization parameters */
        if (gene->useNormalization) {
            totalParams += currentChannels * 2; /* gamma and beta */
        }
    }
    
    *estimatedLatency = totalLatency;
    *estimatedMemory = totalMemory;
    *estimatedParams = totalParams;
    
    return true;
}

bool hyperionNASEvaluateConstraints(NeuralArchitectureSearch *nas,
                                   const NASArchGenome *genome,
                                   bool *satisfiesConstraints) {
    if (!nas || !genome || !satisfiesConstraints) return false;
    
    *satisfiesConstraints = true;
    
    /* Estimate metrics */
    float latency;
    size_t memory, params;
    if (!hyperionNASEstimateMetrics(nas, genome, &latency, &memory, &params)) {
        return false;
    }
    
    /* Check each constraint */
    for (int i = 0; i < nas->config.numConstraints; i++) {
        const NASConstraint *constraint = &nas->config.constraints[i];
        float value = 0.0f;
        
        switch (constraint->type) {
            case NAS_CONSTRAINT_MEMORY:
                value = (float)memory;
                break;
            case NAS_CONSTRAINT_LATENCY:
                value = latency;
                break;
            case NAS_CONSTRAINT_PARAMS:
                value = (float)params;
                break;
            case NAS_CONSTRAINT_FLOPS:
                value = latency * 1e6f; /* Approximate FLOPs from latency */
                break;
            default:
                continue;
        }
        
        if (value > constraint->maxValue) {
            if (constraint->hardConstraint) {
                *satisfiesConstraints = false;
                return true; /* Early exit for hard constraints */
            }
        }
    }
    
    return true;
}

/* Selection for evolutionary algorithm */
static int tournamentSelection(NeuralArchitectureSearch *nas, int tournamentSize) {
    int best = randomInt(nas, nas->config.populationSize);
    float bestFitness = nas->fitnessScores[best];
    
    for (int i = 1; i < tournamentSize; i++) {
        int candidate = randomInt(nas, nas->config.populationSize);
        if (nas->fitnessScores[candidate] > bestFitness) {
            best = candidate;
            bestFitness = nas->fitnessScores[candidate];
        }
    }
    
    return best;
}

bool hyperionNASRun(NeuralArchitectureSearch *nas,
                   NASEvaluationCallback evaluationCallback,
                   void *userData,
                   NASArchGenome *bestArchitecture) {
    if (!nas || !evaluationCallback || !bestArchitecture) return false;
    
    /* Initialize population */
    for (int i = 0; i < nas->config.populationSize; i++) {
        if (!hyperionNASGenerateRandomArchitecture(nas, &nas->population[i])) {
            return false;
        }
    }
    
    /* Evolution loop */
    for (int generation = 0; generation < nas->config.maxGenerations; generation++) {
        nas->currentGeneration = generation;
        
        /* Evaluate population */
        float totalFitness = 0.0f;
        nas->bestFitness = -1e9f;
        int bestIndex = 0;
        
        for (int i = 0; i < nas->config.populationSize; i++) {
            /* Check constraints first */
            bool satisfiesConstraints;
            if (!hyperionNASEvaluateConstraints(nas, &nas->population[i], &satisfiesConstraints)) {
                return false;
            }
            
            if (!satisfiesConstraints) {
                nas->fitnessScores[i] = -1e6f; /* Penalize constraint violations */
            } else {
                /* Evaluate using callback */
                float fitness = evaluationCallback(&nas->population[i], userData);
                nas->fitnessScores[i] = fitness;
                nas->population[i].fitness = fitness;
                nas->evaluationCount++;
            }
            
            totalFitness += nas->fitnessScores[i];
            
            if (nas->fitnessScores[i] > nas->bestFitness) {
                nas->bestFitness = nas->fitnessScores[i];
                bestIndex = i;
            }
        }
        
        nas->avgFitness = totalFitness / nas->config.populationSize;
        
        /* Create new generation */
        NASArchGenome *newPopulation = (NASArchGenome*)hyperionCalloc(nas->config.populationSize, sizeof(NASArchGenome));
        if (!newPopulation) return false;
        
        /* Allocate genes for new population */
        for (int i = 0; i < nas->config.populationSize; i++) {
            newPopulation[i].genes = (NASArchGene*)hyperionCalloc(nas->config.searchSpace.maxLayers, sizeof(NASArchGene));
            if (!newPopulation[i].genes) {
                /* Clean up on failure */
                for (int j = 0; j < i; j++) {
                    hyperionFree(newPopulation[j].genes);
                }
                hyperionFree(newPopulation);
                return false;
            }
        }
        
        /* Elite selection */
        int eliteCount = (int)(nas->config.eliteRatio * nas->config.populationSize);
        if (eliteCount > 0) {
            /* Copy best individual */
            hyperionNASGenomeCopy(&newPopulation[0], &nas->population[bestIndex]);
        }
        
        /* Generate offspring */
        for (int i = eliteCount; i < nas->config.populationSize; i += 2) {
            /* Tournament selection */
            int parent1 = tournamentSelection(nas, 3);
            int parent2 = tournamentSelection(nas, 3);
            
            if (randomFloat(nas) < nas->config.crossoverRate && i + 1 < nas->config.populationSize) {
                /* Crossover */
                hyperionNASCrossover(nas, &nas->population[parent1], &nas->population[parent2],
                                   &newPopulation[i], &newPopulation[i + 1]);
            } else {
                /* Copy parents */
                hyperionNASGenomeCopy(&newPopulation[i], &nas->population[parent1]);
                if (i + 1 < nas->config.populationSize) {
                    hyperionNASGenomeCopy(&newPopulation[i + 1], &nas->population[parent2]);
                }
            }
            
            /* Mutation */
            hyperionNASMutateArchitecture(nas, &newPopulation[i], nas->config.mutationRate);
            if (i + 1 < nas->config.populationSize) {
                hyperionNASMutateArchitecture(nas, &newPopulation[i + 1], nas->config.mutationRate);
            }
        }
        
        /* Replace old population */
        for (int i = 0; i < nas->config.populationSize; i++) {
            hyperionFree(nas->population[i].genes);
        }
        hyperionFree(nas->population);
        nas->population = newPopulation;
    }
    
    /* Return best architecture */
    return hyperionNASGenomeCopy(bestArchitecture, &nas->population[0]);
}

NASArchGenome *hyperionNASGenomeCreate(int maxLayers) {
    NASArchGenome *genome = (NASArchGenome*)hyperionCalloc(1, sizeof(NASArchGenome));
    if (!genome) return NULL;
    
    genome->genes = (NASArchGene*)hyperionCalloc(maxLayers, sizeof(NASArchGene));
    if (!genome->genes) {
        hyperionFree(genome);
        return NULL;
    }
    
    return genome;
}

void hyperionNASGenomeFree(NASArchGenome *genome) {
    if (!genome) return;
    hyperionFree(genome->genes);
    hyperionFree(genome);
}

bool hyperionNASGenomeCopy(NASArchGenome *dest, const NASArchGenome *src) {
    if (!dest || !src) return false;
    
    dest->numLayers = src->numLayers;
    dest->inputChannels = src->inputChannels;
    dest->outputChannels = src->outputChannels;
    dest->fitness = src->fitness;
    dest->accuracy = src->accuracy;
    dest->latency = src->latency;
    dest->memoryUsage = src->memoryUsage;
    dest->parameterCount = src->parameterCount;
    
    for (int i = 0; i < src->numLayers; i++) {
        dest->genes[i] = src->genes[i];
    }
    
    return true;
}

bool hyperionNASGetProgress(NeuralArchitectureSearch *nas,
                           int *currentGeneration,
                           float *bestFitness, float *avgFitness) {
    if (!nas || !currentGeneration || !bestFitness || !avgFitness) return false;
    
    *currentGeneration = nas->currentGeneration;
    *bestFitness = nas->bestFitness;
    *avgFitness = nas->avgFitness;
    
    return true;
}

bool hyperionNASEnableSIMD(NeuralArchitectureSearch *nas, bool enable) {
    if (!nas) return false;
    nas->config.useSIMD = enable;
    return true;
}