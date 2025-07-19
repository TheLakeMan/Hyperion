/**
 * Hyperion Text Generation Header
 * 
 * This header defines the text generation API for Hyperion, allowing
 * loading and running 4-bit quantized neural language models.
 */

#ifndef HYPERION_GENERATE_H
#define HYPERION_GENERATE_H

#include <stdint.h>
#include "tokenizer.h"
#include "../../utils/quantize.h"

/* ----------------- Constants ----------------- */

/* Model type constants */
#define HYPERION_MODEL_TYPE_RNN         0
#define HYPERION_MODEL_TYPE_TRANSFORMER 1

/* Layer type constants */
#define HYPERION_LAYER_EMBEDDING        0
#define HYPERION_LAYER_DENSE            1
#define HYPERION_LAYER_RNN              2
#define HYPERION_LAYER_ATTENTION        3
#define HYPERION_LAYER_LAYERNORM        4
#define HYPERION_LAYER_OUTPUT           5

/* Activation function constants */
#define HYPERION_ACTIVATION_NONE        0
#define HYPERION_ACTIVATION_RELU        1
#define HYPERION_ACTIVATION_SIGMOID     2
#define HYPERION_ACTIVATION_TANH        3
#define HYPERION_ACTIVATION_GELU        4

/* Sampling method constants */
#define HYPERION_SAMPLING_GREEDY        0
#define HYPERION_SAMPLING_TEMPERATURE   1
#define HYPERION_SAMPLING_TOP_K         2
#define HYPERION_SAMPLING_TOP_P         3

/* ----------------- Types ----------------- */

/**
 * Layer type enumeration
 */
typedef uint32_t HyperionLayerType;

/**
 * Activation function enumeration
 */
typedef uint32_t HyperionActivation;

/**
 * Model layer structure
 */
typedef struct {
    HyperionLayerType type;          /* Layer type */
    HyperionActivation activation;   /* Activation function */
    uint32_t inputSize;            /* Input size */
    uint32_t outputSize;           /* Output size */
    HyperionMatrix4bit weights;      /* Layer weights (4-bit quantized) */
    float *biases;                 /* Layer biases */
} HyperionLayer;

/**
 * Model structure
 */
typedef struct {
    uint32_t type;                 /* Model type */
    uint32_t layerCount;           /* Number of layers */
    HyperionLayer *layers;           /* Layers array */
    HyperionTokenizer *tokenizer;    /* Tokenizer */
    uint32_t hiddenSize;           /* Hidden size */
    uint32_t contextSize;          /* Maximum context size */
    float *activations[2];         /* Ping-pong activation buffers */
    int activeBuffer;              /* Active buffer index */
} HyperionModel;

/**
 * Generation parameters structure
 */
typedef struct {
    int maxTokens;                 /* Maximum tokens to generate */
    uint32_t samplingMethod;       /* Sampling method */
    float temperature;             /* Temperature for sampling */
    uint32_t topK;                 /* Top-K for sampling */
    float topP;                    /* Top-P (nucleus) for sampling */
    uint32_t seed;                 /* Random seed (0 for random) */
    int *promptTokens;             /* Prompt tokens (can be NULL) */
    int promptLength;              /* Prompt length */
} HyperionGenerationParams;

/* ----------------- API Functions ----------------- */

/**
 * Create a new text generation model
 * 
 * @param type Model type
 * @param hiddenSize Hidden size
 * @param contextSize Context size
 * @param tokenizer Tokenizer (ownership not transferred)
 * @return New model or NULL on error
 */
HyperionModel* hyperionCreateModel(uint32_t type, uint32_t hiddenSize, 
                             uint32_t contextSize, HyperionTokenizer *tokenizer);

/**
 * Destroy a text generation model
 * 
 * @param model Model to destroy
 */
void hyperionDestroyModel(HyperionModel *model);

/**
 * Add a layer to a model
 * 
 * @param model Model to add to
 * @param type Layer type
 * @param inputSize Input size
 * @param outputSize Output size
 * @param activation Activation function
 * @return 0 on success, non-zero on error
 */
int hyperionAddLayer(HyperionModel *model, HyperionLayerType type, 
                 uint32_t inputSize, uint32_t outputSize, 
                 HyperionActivation activation);

/**
 * Load model weights from a file
 * 
 * @param model Model to load into
 * @param path File path
 * @return 0 on success, non-zero on error
 */
int hyperionLoadModelWeights(HyperionModel *model, const char *path);

/**
 * Load a complete model from files
 * 
 * @param modelPath Model structure file path
 * @param weightsPath Model weights file path
 * @param tokenizerPath Tokenizer file path
 * @return Loaded model or NULL on error
 */
HyperionModel* hyperionLoadModel(const char *modelPath, const char *weightsPath, 
                           const char *tokenizerPath);

/**
 * Perform a single forward pass through the model
 * 
 * @param model Model to use
 * @param input Input token IDs
 * @param inputLength Number of input tokens
 * @param output Output logits (must be allocated to at least vocab size)
 * @return 0 on success, non-zero on error
 */
int hyperionModelForward(HyperionModel *model, const int *input, 
                     int inputLength, float *output);

/**
 * Sample the next token from output probabilities
 * 
 * @param output Output logits from model
 * @param vocabSize Vocabulary size
 * @param params Generation parameters
 * @return Sampled token ID
 */
int hyperionSampleToken(const float *output, int vocabSize, 
                    const HyperionGenerationParams *params);

/**
 * Generate text from a model
 * 
 * @param model Model to use
 * @param params Generation parameters
 * @param outputTokens Output token buffer (must be allocated)
 * @param maxOutputTokens Maximum output tokens
 * @return Number of tokens generated
 */
int hyperionGenerateText(HyperionModel *model, const HyperionGenerationParams *params,
                     int *outputTokens, int maxOutputTokens);

/**
 * Convert a model to 4-bit quantization
 * 
 * @param model Model to quantize
 * @return 0 on success, non-zero on error
 */
int hyperionQuantizeModel(HyperionModel *model);

#endif /* HYPERION_GENERATE_H */