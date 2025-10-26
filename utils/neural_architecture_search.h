/**
 * @file neural_architecture_search.h
 * @brief Neural Architecture Search (NAS) for automated model optimization in Hyperion
 *
 * This module provides Neural Architecture Search capabilities to automatically
 * discover optimal network architectures for specific tasks, hardware constraints,
 * and performance requirements. Supports evolutionary algorithms, reinforcement
 * learning-based search, and efficient architecture sampling techniques.
 */

#ifndef HYPERION_NEURAL_ARCHITECTURE_SEARCH_H
#define HYPERION_NEURAL_ARCHITECTURE_SEARCH_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Architecture search space types
 */
typedef enum {
    NAS_SEARCH_MACRO,      /* Macro search space (overall architecture) */
    NAS_SEARCH_MICRO,      /* Micro search space (cell-based) */
    NAS_SEARCH_HYBRID      /* Hybrid macro + micro search */
} NASSearchSpaceType;

/**
 * Search strategy types
 */
typedef enum {
    NAS_STRATEGY_RANDOM,      /* Random sampling */
    NAS_STRATEGY_EVOLUTIONARY, /* Evolutionary algorithm */
    NAS_STRATEGY_RL,          /* Reinforcement learning */
    NAS_STRATEGY_GRADIENT,    /* Gradient-based (DARTS-like) */
    NAS_STRATEGY_BAYESIAN     /* Bayesian optimization */
} NASSearchStrategy;

/**
 * Layer types in search space
 */
typedef enum {
    NAS_LAYER_CONV2D,        /* 2D Convolution */
    NAS_LAYER_DEPTHWISE_CONV, /* Depthwise separable convolution */
    NAS_LAYER_POINTWISE_CONV, /* Pointwise (1x1) convolution */
    NAS_LAYER_MAXPOOL,       /* Max pooling */
    NAS_LAYER_AVGPOOL,       /* Average pooling */
    NAS_LAYER_SKIP,          /* Skip connection */
    NAS_LAYER_DENSE,         /* Dense/Linear layer */
    NAS_LAYER_ATTENTION,     /* Self-attention layer */
    NAS_LAYER_NORMALIZE,     /* Normalization layer */
    NAS_LAYER_ACTIVATION     /* Activation function */
} NASLayerType;

/**
 * Hardware constraint types
 */
typedef enum {
    NAS_CONSTRAINT_MEMORY,    /* Memory usage constraint */
    NAS_CONSTRAINT_LATENCY,   /* Inference latency constraint */
    NAS_CONSTRAINT_ENERGY,    /* Energy consumption constraint */
    NAS_CONSTRAINT_PARAMS,    /* Parameter count constraint */
    NAS_CONSTRAINT_FLOPS      /* FLOPs constraint */
} NASConstraintType;

/**
 * Architecture gene (single layer configuration)
 */
typedef struct {
    NASLayerType layerType;   /* Type of layer */
    int channels;             /* Number of channels/filters */
    int kernelSize;           /* Kernel size (for conv layers) */
    int stride;               /* Stride */
    int padding;              /* Padding */
    float dropoutRate;        /* Dropout rate */
    bool useNormalization;    /* Whether to use normalization */
    int activationFunction;   /* Activation function ID */
} NASArchGene;

/**
 * Complete architecture genome
 */
typedef struct {
    NASArchGene *genes;       /* Array of layer genes */
    int numLayers;           /* Number of layers */
    int inputChannels;       /* Input channels */
    int outputChannels;      /* Output channels */
    float fitness;           /* Fitness score */
    float accuracy;          /* Validation accuracy */
    float latency;           /* Inference latency (ms) */
    size_t memoryUsage;      /* Memory usage (bytes) */
    size_t parameterCount;   /* Number of parameters */
} NASArchGenome;

/**
 * Hardware constraint specification
 */
typedef struct {
    NASConstraintType type;   /* Type of constraint */
    float maxValue;          /* Maximum allowed value */
    float weight;            /* Weight in fitness function */
    bool hardConstraint;     /* Whether it's a hard constraint */
} NASConstraint;

/**
 * Search space configuration
 */
typedef struct {
    NASSearchSpaceType searchType;  /* Type of search space */
    NASLayerType *allowedLayers;    /* Allowed layer types */
    int numAllowedLayers;          /* Number of allowed layer types */
    int minLayers, maxLayers;      /* Layer count range */
    int minChannels, maxChannels;  /* Channel count range */
    int *allowedKernelSizes;       /* Allowed kernel sizes */
    int numKernelSizes;           /* Number of kernel size options */
    bool allowSkipConnections;     /* Whether skip connections are allowed */
    bool allowResidualBlocks;      /* Whether residual blocks are allowed */
} NASSearchSpace;

/**
 * NAS configuration
 */
typedef struct {
    NASSearchStrategy strategy;      /* Search strategy */
    NASSearchSpace searchSpace;      /* Search space definition */
    NASConstraint *constraints;      /* Hardware/performance constraints */
    int numConstraints;             /* Number of constraints */
    
    /* Search parameters */
    int populationSize;             /* Population size for evolutionary/RL */
    int maxGenerations;             /* Maximum generations/iterations */
    float mutationRate;             /* Mutation rate */
    float crossoverRate;            /* Crossover rate */
    float eliteRatio;              /* Elite retention ratio */
    
    /* Evaluation parameters */
    int trainingEpochs;            /* Epochs for architecture evaluation */
    int validationSamples;         /* Number of validation samples */
    float accuracyWeight;          /* Weight for accuracy in fitness */
    float latencyWeight;           /* Weight for latency in fitness */
    float memoryWeight;            /* Weight for memory in fitness */
    
    /* Hardware-specific */
    bool useQuantization;          /* Whether to use quantization */
    bool useSIMD;                  /* Whether to use SIMD optimization */
    size_t targetMemoryBudget;     /* Target memory budget */
    float targetLatencyMs;         /* Target latency in milliseconds */
} NASConfig;

/**
 * NAS context structure
 */
typedef struct NeuralArchitectureSearch NeuralArchitectureSearch;

/**
 * Architecture evaluation callback
 * 
 * User-provided function to evaluate architecture performance
 */
typedef float (*NASEvaluationCallback)(const NASArchGenome *genome, 
                                      void *userData);

/**
 * Create Neural Architecture Search context
 * @param config NAS configuration
 * @return Newly created NAS context, or NULL on failure
 */
NeuralArchitectureSearch *hyperionNASCreate(const NASConfig *config);

/**
 * Free Neural Architecture Search context
 * @param nas NAS context to free
 */
void hyperionNASFree(NeuralArchitectureSearch *nas);

/**
 * Run Neural Architecture Search
 * 
 * @param nas NAS context
 * @param evaluationCallback Function to evaluate architectures
 * @param userData User data passed to evaluation callback
 * @param bestArchitecture Output best discovered architecture
 * @return true on success, false on failure
 */
bool hyperionNASRun(NeuralArchitectureSearch *nas,
                   NASEvaluationCallback evaluationCallback,
                   void *userData,
                   NASArchGenome *bestArchitecture);

/**
 * Generate random architecture from search space
 * 
 * @param nas NAS context
 * @param genome Output architecture genome
 * @return true on success, false on failure
 */
bool hyperionNASGenerateRandomArchitecture(NeuralArchitectureSearch *nas,
                                         NASArchGenome *genome);

/**
 * Mutate architecture genome
 * 
 * @param nas NAS context
 * @param genome Architecture genome to mutate (modified in place)
 * @param mutationRate Mutation rate (0.0 to 1.0)
 * @return true on success, false on failure
 */
bool hyperionNASMutateArchitecture(NeuralArchitectureSearch *nas,
                                 NASArchGenome *genome, float mutationRate);

/**
 * Crossover between two architectures
 * 
 * @param nas NAS context
 * @param parent1 First parent architecture
 * @param parent2 Second parent architecture
 * @param offspring1 First offspring (output)
 * @param offspring2 Second offspring (output)
 * @return true on success, false on failure
 */
bool hyperionNASCrossover(NeuralArchitectureSearch *nas,
                         const NASArchGenome *parent1,
                         const NASArchGenome *parent2,
                         NASArchGenome *offspring1,
                         NASArchGenome *offspring2);

/**
 * Evaluate architecture constraints
 * 
 * @param nas NAS context
 * @param genome Architecture to evaluate
 * @param satisfiesConstraints Output whether constraints are satisfied
 * @return true on success, false on failure
 */
bool hyperionNASEvaluateConstraints(NeuralArchitectureSearch *nas,
                                   const NASArchGenome *genome,
                                   bool *satisfiesConstraints);

/**
 * Estimate architecture performance metrics
 * 
 * @param nas NAS context
 * @param genome Architecture to analyze
 * @param estimatedLatency Output estimated latency (ms)
 * @param estimatedMemory Output estimated memory usage (bytes)
 * @param estimatedParams Output estimated parameter count
 * @return true on success, false on failure
 */
bool hyperionNASEstimateMetrics(NeuralArchitectureSearch *nas,
                               const NASArchGenome *genome,
                               float *estimatedLatency,
                               size_t *estimatedMemory,
                               size_t *estimatedParams);

/**
 * Create architecture genome
 * @param maxLayers Maximum number of layers
 * @return Newly allocated genome, or NULL on failure
 */
NASArchGenome *hyperionNASGenomeCreate(int maxLayers);

/**
 * Free architecture genome
 * @param genome Genome to free
 */
void hyperionNASGenomeFree(NASArchGenome *genome);

/**
 * Copy architecture genome
 * @param dest Destination genome
 * @param src Source genome
 * @return true on success, false on failure
 */
bool hyperionNASGenomeCopy(NASArchGenome *dest, const NASArchGenome *src);

/**
 * Convert genome to human-readable string
 * @param genome Architecture genome
 * @param buffer Output string buffer
 * @param bufferSize Size of output buffer
 * @return true on success, false on failure
 */
bool hyperionNASGenomeToString(const NASArchGenome *genome,
                              char *buffer, size_t bufferSize);

/**
 * Save architecture to file
 * @param genome Architecture genome
 * @param filename Output file name
 * @return true on success, false on failure
 */
bool hyperionNASSaveArchitecture(const NASArchGenome *genome,
                                const char *filename);

/**
 * Load architecture from file
 * @param genome Output architecture genome
 * @param filename Input file name
 * @return true on success, false on failure
 */
bool hyperionNASLoadArchitecture(NASArchGenome *genome,
                                const char *filename);

/**
 * Progressive Neural Architecture Search
 * 
 * Performs progressive search starting with simple architectures
 * 
 * @param nas NAS context
 * @param evaluationCallback Evaluation function
 * @param userData User data for evaluation
 * @param stages Number of progressive stages
 * @param bestArchitecture Output best architecture
 * @return true on success, false on failure
 */
bool hyperionNASProgressiveSearch(NeuralArchitectureSearch *nas,
                                 NASEvaluationCallback evaluationCallback,
                                 void *userData, int stages,
                                 NASArchGenome *bestArchitecture);

/**
 * Multi-objective Neural Architecture Search
 * 
 * Optimizes for multiple objectives simultaneously (accuracy, latency, memory)
 * 
 * @param nas NAS context
 * @param evaluationCallback Evaluation function
 * @param userData User data for evaluation
 * @param paretoFront Output Pareto front of solutions
 * @param maxSolutions Maximum number of Pareto optimal solutions
 * @param numSolutions Output number of solutions found
 * @return true on success, false on failure
 */
bool hyperionNASMultiObjectiveSearch(NeuralArchitectureSearch *nas,
                                    NASEvaluationCallback evaluationCallback,
                                    void *userData,
                                    NASArchGenome *paretoFront,
                                    int maxSolutions, int *numSolutions);

/**
 * Hardware-aware architecture search
 * 
 * Search specifically optimized for target hardware platform
 * 
 * @param nas NAS context
 * @param targetHardware Hardware specification string
 * @param evaluationCallback Evaluation function
 * @param userData User data
 * @param bestArchitecture Output optimized architecture
 * @return true on success, false on failure
 */
bool hyperionNASHardwareAwareSearch(NeuralArchitectureSearch *nas,
                                   const char *targetHardware,
                                   NASEvaluationCallback evaluationCallback,
                                   void *userData,
                                   NASArchGenome *bestArchitecture);

/**
 * Get search progress statistics
 * @param nas NAS context
 * @param currentGeneration Output current generation
 * @param bestFitness Output best fitness found
 * @param avgFitness Output average fitness in population
 * @return true on success, false on failure
 */
bool hyperionNASGetProgress(NeuralArchitectureSearch *nas,
                           int *currentGeneration,
                           float *bestFitness, float *avgFitness);

/**
 * Enable/disable SIMD optimization for NAS
 * @param nas NAS context
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionNASEnableSIMD(NeuralArchitectureSearch *nas, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_NEURAL_ARCHITECTURE_SEARCH_H */