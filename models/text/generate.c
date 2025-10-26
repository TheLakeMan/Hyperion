/**
 * Hyperion Text Generation Implementation
 *
 * This file implements the text generation model for Hyperion,
 * using 4-bit quantization for extreme memory efficiency.
 */

#include "generate.h"
#include "sampling.h"
#include "../../core/config.h"
#include "../../core/memory.h"
#include "../../utils/quantize.h"
#include "../model_format.h"
#include "tokenizer.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ----------------- Internal Constants and Definitions ----------------- */

/* Block size for matrix multiplication */
#define BLOCK_SIZE 32

/* KV cache entry */
typedef struct {
    float *key;   /* Key vector */
    float *value; /* Value vector */
} KVCacheEntry;

typedef struct {
    uint32_t modelType;
    uint32_t layerCount;
    uint32_t hiddenSize;
    uint32_t contextSize;
} HyperionPackedModelHeader;

typedef struct {
    uint32_t layerType;
    uint32_t inputSize;
    uint32_t outputSize;
    uint32_t activation;
    float    scale;
    float    zeroPoint;
    uint32_t weightBytes;
    uint32_t biasBytes;
} HyperionPackedLayer;

/* ----------------- Static Variables ----------------- */

/* ----------------- Helper Functions ----------------- */

/**
 * Apply generation style adjustments to parameters
 */
static void applyGenerationStyle(HyperionGenerationParams *params, HyperionGenerationStyle style)
{
    switch (style) {
    case HYPERION_STYLE_FORMAL:
        params->temperature = 0.5f; // Less creative
        params->topK        = 1;    // More deterministic
        params->topP        = 0.0f; // Greedy
        break;
    case HYPERION_STYLE_CREATIVE:
        params->temperature = 1.2f; // More creative
        params->topK        = 50;   // Wider range
        params->topP        = 0.95f; // More diverse
        break;
    case HYPERION_STYLE_CONCISE:
        params->maxTokens = params->maxTokens > 20 ? 20 : params->maxTokens; // Shorter output
        params->temperature = 0.6f;
        params->topK = 5;
        params->topP = 0.8f;
        break;
    case HYPERION_STYLE_DESCRIPTIVE:
        params->maxTokens = params->maxTokens < 100 ? 100 : params->maxTokens; // Longer output
        params->temperature = 0.9f;
        params->topK = 0; // Use topP
        params->topP = 0.9f;
        break;
    case HYPERION_STYLE_NEUTRAL:
    default:
        // Default values (already set or remain as is)
        break;
    }
}


static int read_u32(const uint8_t **cursor, size_t *remaining, uint32_t *out)
{
    if (!cursor || !remaining || !out || *remaining < sizeof(uint32_t)) {
        return -1;
    }

    memcpy(out, *cursor, sizeof(uint32_t));
    *cursor += sizeof(uint32_t);
    *remaining -= sizeof(uint32_t);
    return 0;
}

static int read_float(const uint8_t **cursor, size_t *remaining, float *out)
{
    if (!cursor || !remaining || !out || *remaining < sizeof(float)) {
        return -1;
    }

    memcpy(out, *cursor, sizeof(float));
    *cursor += sizeof(float);
    *remaining -= sizeof(float);
    return 0;
}

static int read_bytes(const uint8_t **cursor, size_t *remaining, void *dest, size_t length)
{
    if (!cursor || !remaining || !dest || *remaining < length) {
        return -1;
    }

    memcpy(dest, *cursor, length);
    *cursor += length;
    *remaining -= length;
    return 0;
}

/**
 * Perform top-K sampling
 */

/**
 * Perform top-P (nucleus) sampling
 */

/**
 * Copy a layer's weights from FP32 to 4-bit
 */
static int copyLayerWeights(HyperionLayer *layer, const float *weights, const float *biases)
{
    if (!layer || !weights) {
        return -1;
    }

    /* Create temporary FP32 matrix */
    HyperionMatrixFP32 weightsFP32;
    weightsFP32.rows = layer->inputSize;
    weightsFP32.cols = layer->outputSize;
    weightsFP32.data = (float *)weights;

    /* Quantize to 4-bit */
    HyperionMatrix4bit *weights4bit = hyperionQuantizeFP32To4bit(&weightsFP32);
    if (!weights4bit) {
        return -1;
    }

    /* Copy to layer */
    memcpy(&layer->weights, weights4bit, sizeof(HyperionMatrix4bit));

    /* Add biases if provided */
    if (biases) {
        layer->biases = (float *)HYPERION_MALLOC(layer->outputSize * sizeof(float));
        if (!layer->biases) {
            hyperionDestroyMatrix4bit(weights4bit);
            return -1;
        }

        memcpy(layer->biases, biases, layer->outputSize * sizeof(float));
    }

    /* Clean up */
    HYPERION_FREE(
        weights4bit); /* Just free the struct, not the data which is now owned by the layer */

    return 0;
}

/* ----------------- Model Implementation ----------------- */

/**
 * Create a new text generation model
 */
HyperionModel *hyperionCreateModel(uint32_t type, uint32_t hiddenSize, uint32_t contextSize,
                               HyperionTokenizer *tokenizer)
{
    if (!tokenizer) {
        return NULL;
    }

    HyperionModel *model = (HyperionModel *)HYPERION_MALLOC(sizeof(HyperionModel));
    if (!model) {
        return NULL;
    }

    /* Initialize the model */
    model->type        = type;
    model->layerCount  = 0;
    model->layers      = NULL;
    model->tokenizer   = tokenizer;
    model->hiddenSize  = hiddenSize;
    model->contextSize = contextSize;

    /* Allocate activation buffers */
    model->activations[0] = (float *)HYPERION_MALLOC(contextSize * hiddenSize * sizeof(float));
    model->activations[1] = (float *)HYPERION_MALLOC(contextSize * hiddenSize * sizeof(float));

    if (!model->activations[0] || !model->activations[1]) {
        if (model->activations[0])
            HYPERION_FREE(model->activations[0]);
        if (model->activations[1])
            HYPERION_FREE(model->activations[1]);
        HYPERION_FREE(model);
        return NULL;
    }

    model->activeBuffer = 0;

    return model;
}

/**
 * Destroy a text generation model
 */
void hyperionDestroyModel(HyperionModel *model)
{
    if (!model) {
        return;
    }

    /* Free layers */
    if (model->layers) {
        for (uint32_t i = 0; i < model->layerCount; i++) {
            if (model->layers[i].weights.data) {
                HYPERION_FREE(model->layers[i].weights.data);
            }
            if (model->layers[i].biases) {
                HYPERION_FREE(model->layers[i].biases);
            }
        }
        HYPERION_FREE(model->layers);
    }

    /* Free activation buffers */
    if (model->activations[0]) {
        HYPERION_FREE(model->activations[0]);
    }
    if (model->activations[1]) {
        HYPERION_FREE(model->activations[1]);
    }

    /* Note: We don't free the tokenizer as it might be used elsewhere */

    /* Free the model */
    HYPERION_FREE(model);
}

/**
 * Add a layer to a model
 */
int hyperionAddLayer(HyperionModel *model, HyperionLayerType type, uint32_t inputSize,
                   uint32_t outputSize, HyperionActivation activation)
{
    if (!model) {
        return -1;
    }

    /* Allocate or reallocate the layers array */
    HyperionLayer *newLayers =
        (HyperionLayer *)HYPERION_MALLOC((model->layerCount + 1) * sizeof(HyperionLayer));
    if (!newLayers) {
        return -1;
    }

    /* Copy existing layers */
    if (model->layers) {
        memcpy(newLayers, model->layers, model->layerCount * sizeof(HyperionLayer));
        HYPERION_FREE(model->layers);
    }

    model->layers = newLayers;

    /* Initialize the new layer */
    HyperionLayer *layer  = &model->layers[model->layerCount];
    layer->type         = type;
    layer->activation   = activation;
    layer->inputSize    = inputSize;
    layer->outputSize   = outputSize;
    layer->weights.data = NULL;
    layer->biases       = NULL;

    model->layerCount++;

    return 0;
}

/**
 * Load model weights from a file
 */
int hyperionLoadModelWeights(HyperionModel *model, const char *path)
{
    if (!model || !path) {
        return -1;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return -1;
    }

    /* Read header */
    uint32_t magic, version, layerCount;
    if (fread(&magic, sizeof(magic), 1, file) != 1 || magic != 0x4D494E54 || /* "HYPERION" */
        fread(&version, sizeof(version), 1, file) != 1 ||
        fread(&layerCount, sizeof(layerCount), 1, file) != 1 || layerCount != model->layerCount) {
        fclose(file);
        return -1;
    }

    /* Read layer weights */
    for (uint32_t i = 0; i < model->layerCount; i++) {
        HyperionLayer *layer = &model->layers[i];

        /* Read layer type and sizes */
        uint32_t layerType, inputSize, outputSize, activation;
        if (fread(&layerType, sizeof(layerType), 1, file) != 1 ||
            fread(&inputSize, sizeof(inputSize), 1, file) != 1 ||
            fread(&outputSize, sizeof(outputSize), 1, file) != 1) {
            fclose(file);
            return -1;
        }

        /* Verify layer details */
        if (layerType != layer->type || inputSize != layer->inputSize ||
            outputSize != layer->outputSize) {
            fclose(file);
            return -1;
        }

        /* Allocate weight matrix */
        size_t dataSize =
            (layer->inputSize * layer->outputSize + 1) / 2; /* 4-bit, 2 values per byte */
        layer->weights.data = (uint8_t *)HYPERION_MALLOC(dataSize);
        if (!layer->weights.data) {
            fclose(file);
            return -1;
        }

        /* Read weights */
        if (fread(&layer->weights.scale, sizeof(layer->weights.scale), 1, file) != 1 ||
            fread(&layer->weights.zeroPoint, sizeof(layer->weights.zeroPoint), 1, file) != 1 ||
            fread(layer->weights.data, 1, dataSize, file) != dataSize) {
            fclose(file);
            return -1;
        }

        layer->weights.rows = layer->inputSize;
        layer->weights.cols = layer->outputSize;

        /* Allocate and read biases */
        layer->biases = (float *)HYPERION_MALLOC(layer->outputSize * sizeof(float));
        if (!layer->biases) {
            fclose(file);
            return -1;
        }

        if (fread(layer->biases, sizeof(float), layer->outputSize, file) != layer->outputSize) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);
    return 0;
}

static HyperionModel *hyperionLoadModelPackage(const char *packagePath, const char *tokenizerPath)
{
    HyperionModelInfo info;
    void             *weightsBuffer = NULL;
    size_t            weightsLength = 0;
    HyperionTokenizer *tokenizer    = NULL;
    HyperionModel     *model        = NULL;
    int                success      = 0;

    if (hyperionModelRead(packagePath, &info, &weightsBuffer, &weightsLength) != 0) {
        goto cleanup;
    }

    if (info.header.domain != HYPERION_MODEL_DOMAIN_TEXT) {
        goto cleanup;
    }

    tokenizer = hyperionCreateTokenizer();
    if (!tokenizer) {
        goto cleanup;
    }

    if (tokenizerPath && hyperionLoadVocabulary(tokenizer, tokenizerPath) != 0) {
        goto cleanup;
    }

    const uint8_t *cursor    = (const uint8_t *)weightsBuffer;
    size_t         remaining = weightsLength;

    HyperionPackedModelHeader packedHeader;
    if (read_u32(&cursor, &remaining, &packedHeader.modelType) != 0 ||
        read_u32(&cursor, &remaining, &packedHeader.layerCount) != 0 ||
        read_u32(&cursor, &remaining, &packedHeader.hiddenSize) != 0 ||
        read_u32(&cursor, &remaining, &packedHeader.contextSize) != 0) {
        goto cleanup;
    }

    if (packedHeader.layerCount == 0) {
        goto cleanup;
    }

    if (info.header.quantization != HYPERION_MODEL_QUANT_INT4 &&
        info.header.quantization != HYPERION_MODEL_QUANT_INT8 &&
        info.header.quantization != HYPERION_MODEL_QUANT_UNKNOWN) {
        goto cleanup;
    }

    model = hyperionCreateModel(packedHeader.modelType, packedHeader.hiddenSize,
                                packedHeader.contextSize, tokenizer);
    if (!model) {
        goto cleanup;
    }

    for (uint32_t i = 0; i < packedHeader.layerCount; ++i) {
        HyperionPackedLayer entry;

        if (read_u32(&cursor, &remaining, &entry.layerType) != 0 ||
            read_u32(&cursor, &remaining, &entry.inputSize) != 0 ||
            read_u32(&cursor, &remaining, &entry.outputSize) != 0 ||
            read_u32(&cursor, &remaining, &entry.activation) != 0 ||
            read_float(&cursor, &remaining, &entry.scale) != 0 ||
            read_float(&cursor, &remaining, &entry.zeroPoint) != 0 ||
            read_u32(&cursor, &remaining, &entry.weightBytes) != 0 ||
            read_u32(&cursor, &remaining, &entry.biasBytes) != 0) {
            goto cleanup;
        }

        if (hyperionAddLayer(model, entry.layerType, entry.inputSize, entry.outputSize,
                             entry.activation) != 0) {
            goto cleanup;
        }

        HyperionLayer *layer = &model->layers[i];
        layer->weights.scale     = entry.scale;
        layer->weights.zeroPoint = entry.zeroPoint;
        layer->weights.rows      = layer->inputSize;
        layer->weights.cols      = layer->outputSize;

        if (entry.weightBytes == 0) {
            goto cleanup;
        }

        layer->weights.data = (uint8_t *)HYPERION_MALLOC(entry.weightBytes);
        if (!layer->weights.data) {
            goto cleanup;
        }

        if (read_bytes(&cursor, &remaining, layer->weights.data, entry.weightBytes) != 0) {
            goto cleanup;
        }

        size_t expectedBiasBytes = (size_t)layer->outputSize * sizeof(float);
        if (entry.biasBytes != expectedBiasBytes) {
            goto cleanup;
        }

        layer->biases = (float *)HYPERION_MALLOC(expectedBiasBytes);
        if (!layer->biases) {
            goto cleanup;
        }

        if (read_bytes(&cursor, &remaining, layer->biases, expectedBiasBytes) != 0) {
            goto cleanup;
        }
    }

    success   = 1;
    tokenizer = NULL; /* Ownership remains with caller via model pointer */

cleanup:
    if (!success) {
        if (model) {
            hyperionDestroyModel(model);
            model = NULL;
        }
        if (tokenizer) {
            hyperionDestroyTokenizer(tokenizer);
        }
    }

    if (weightsBuffer) {
        HYPERION_FREE(weightsBuffer);
    }

    return model;
}

/**
 * Load a complete model from files
 */
HyperionModel *hyperionLoadModel(const char *modelPath, const char *weightsPath,
                             const char *tokenizerPath)
{
    if (modelPath) {
        FILE *maybePackage = fopen(modelPath, "rb");
        if (maybePackage) {
            uint32_t magic = 0;
            if (fread(&magic, sizeof(magic), 1, maybePackage) == 1) {
                fclose(maybePackage);
                if (magic == HYPERION_MODEL_FORMAT_MAGIC) {
                    return hyperionLoadModelPackage(modelPath, tokenizerPath);
                }
            }
            else {
                fclose(maybePackage);
            }
        }
    }

    if (weightsPath) {
        FILE *maybePackage = fopen(weightsPath, "rb");
        if (maybePackage) {
            uint32_t magic = 0;
            if (fread(&magic, sizeof(magic), 1, maybePackage) == 1) {
                fclose(maybePackage);
                if (magic == HYPERION_MODEL_FORMAT_MAGIC) {
                    return hyperionLoadModelPackage(weightsPath, tokenizerPath);
                }
            }
            else {
                fclose(maybePackage);
            }
        }
    }

    /* Load model structure */
    FILE *file = fopen(modelPath, "rb");
    if (!file) {
        return NULL;
    }

    /* Read header */
    uint32_t magic, version, type, hiddenSize, contextSize, layerCount;
    if (fread(&magic, sizeof(magic), 1, file) != 1 || magic != 0x4D494E54 || /* "HYPERION" */
        fread(&version, sizeof(version), 1, file) != 1 ||
        fread(&type, sizeof(type), 1, file) != 1 ||
        fread(&hiddenSize, sizeof(hiddenSize), 1, file) != 1 ||
        fread(&contextSize, sizeof(contextSize), 1, file) != 1 ||
        fread(&layerCount, sizeof(layerCount), 1, file) != 1) {
        fclose(file);
        return NULL;
    }

    /* Load tokenizer */
    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    if (!tokenizer) {
        fclose(file);
        return NULL;
    }

    if (hyperionLoadVocabulary(tokenizer, tokenizerPath) != 0) {
        hyperionDestroyTokenizer(tokenizer);
        fclose(file);
        return NULL;
    }

    /* Create model */
    HyperionModel *model = hyperionCreateModel(type, hiddenSize, contextSize, tokenizer);
    if (!model) {
        hyperionDestroyTokenizer(tokenizer);
        fclose(file);
        return NULL;
    }

    /* Read layer definitions */
    for (uint32_t i = 0; i < layerCount; i++) {
        uint32_t layerType, inputSize, outputSize, activation;
        if (fread(&layerType, sizeof(layerType), 1, file) != 1 ||
            fread(&inputSize, sizeof(inputSize), 1, file) != 1 ||
            fread(&outputSize, sizeof(outputSize), 1, file) != 1 ||
            fread(&activation, sizeof(activation), 1, file) != 1) {
            hyperionDestroyModel(model);
            fclose(file);
            return NULL;
        }

        hyperionAddLayer(model, (HyperionLayerType)layerType, inputSize, outputSize,
                       (HyperionActivation)activation);
    }

    fclose(file);

    /* Load weights */
    if (hyperionLoadModelWeights(model, weightsPath) != 0) {
        hyperionDestroyModel(model);
        return NULL;
    }

    return model;
}

/* Include SIMD operations and cache optimizations */
#include "../../utils/cache_opt.h"
#include "../../utils/simd_ops.h"

/**
 * Cache-optimized matrix-vector multiplication using SIMD if available
 */
static void matrixVectorMultiplySIMD(const float *matrix, const float *vector, float *output,
                                     int rows, int cols, bool useSIMD)
{
    if (useSIMD) {
        /* Use SIMD implementation from simd_ops.h */
        hyperion_simd_matmul_f32(matrix, vector, output, rows, cols, 1);
    }
    else {
        /* Get cache optimization parameters */
        HyperionCacheOptConfig config = hyperion_cache_opt_init_default();
        hyperion_cache_opt_matrix_multiply(rows, 1, cols, &config);

        /* Use cache-friendly blocking */
        HYPERION_LOOP_TILING_2D(i, 0, rows, k, 0, cols, config.blockSizeX, config.blockSizeY, {
            /* Inside the innermost loops, accumulate partial sums */
            output[i] += matrix[i] * vector[k];
        });
    }
}

/**
 * Perform a single forward pass through the model
 */
int hyperionModelForward(HyperionModel *model, const int *input, int inputLength, float *output)
{
    if (!model || !input || !output || inputLength <= 0) {
        return -1;
    }

    /* Context limitation */
    if (inputLength > model->contextSize) {
        inputLength = model->contextSize;
    }

    /* Check if SIMD is available */
    bool useSIMD =
        hyperion_simd_detect_capabilities() > 0; /* Non-zero means some SIMD is available */

    /* Process based on model type */
    switch (model->type) {
    case HYPERION_MODEL_TYPE_RNN:
        /* Simple RNN implementation */
        {
            /* Embedding layer */
            float *embeddings = model->activations[model->activeBuffer];
            memset(embeddings, 0, model->hiddenSize * sizeof(float));

            /* Process only the last token for output */
            int lastToken = input[inputLength - 1];

            /* Check token range */
            if (lastToken < 0 || lastToken >= model->tokenizer->tokenCount) {
                lastToken = HYPERION_TOKEN_UNKNOWN;
            }

            /* Apply the model layers */
            for (uint32_t i = 0; i < model->layerCount; i++) {
                HyperionLayer *layer = &model->layers[i];

                /* Flip activation buffers */
                model->activeBuffer = 1 - model->activeBuffer;
                float *input        = model->activations[1 - model->activeBuffer];
                float *output       = model->activations[model->activeBuffer];

                /* Apply layer based on type */
                switch (layer->type) {
                case HYPERION_LAYER_EMBEDDING:
                    /* Copy embedding vector for the token */
                    {
                        HyperionMatrixFP32 *matrix = hyperionDequantize4bitToFP32(&layer->weights);

                        if (!matrix) {
                            return -1;
                        }

                        /* Copy embedding for the token */
                        memcpy(output + lastToken * layer->outputSize,
                               matrix->data + lastToken * layer->outputSize,
                               layer->outputSize * sizeof(float));

                        hyperionDestroyMatrixFP32(matrix);
                    }
                    break;

                case HYPERION_LAYER_DENSE:
                    /* Dense layer implementation */
                    {
                        HyperionMatrixFP32 *matrix = hyperionDequantize4bitToFP32(&layer->weights);

                        if (!matrix) {
                            return -1;
                        }

                        /* Matrix multiplication */
                        for (uint32_t j = 0; j < layer->outputSize; j++) {
                            float sum = 0.0f;
                            for (uint32_t k = 0; k < layer->inputSize; k++) {
                                sum += input[k] * matrix->data[k * layer->outputSize + j];
                            }

                            /* Add bias */
                            if (layer->biases) {
                                sum += layer->biases[j];
                            }

                            /* Store result */
                            output[j] = sum;
                        }

                        hyperionDestroyMatrixFP32(matrix);

                        /* Apply activation function */
                        switch (layer->activation) {
                        case HYPERION_ACTIVATION_RELU:
                            for (uint32_t j = 0; j < layer->outputSize; j++) {
                                if (output[j] < 0.0f) {
                                    output[j] = 0.0f;
                                }
                            }
                            break;

                        case HYPERION_ACTIVATION_SIGMOID:
                            for (uint32_t j = 0; j < layer->outputSize; j++) {
                                output[j] = 1.0f / (1.0f + expf(-output[j]));
                            }
                            break;

                        case HYPERION_ACTIVATION_TANH:
                            for (uint32_t j = 0; j < layer->outputSize; j++) {
                                output[j] = tanhf(output[j]);
                            }
                            break;

                        case HYPERION_ACTIVATION_GELU:
                            for (uint32_t j = 0; j < layer->outputSize; j++) {
                                float x   = output[j];
                                output[j] = 0.5f * x *
                                            (1.0f + tanhf(sqrtf(2.0f / 3.14159f) *
                                                          (x + 0.044715f * x * x * x)));
                            }
                            break;

                        default:
                            /* No activation (linear) */
                            break;
                        }
                    }
                    break;

                case HYPERION_LAYER_OUTPUT:
                    /* Output layer */
                    {
                        HyperionMatrixFP32 *matrix = hyperionDequantize4bitToFP32(&layer->weights);

                        if (!matrix) {
                            return -1;
                        }

                        /* Matrix multiplication */
                        for (uint32_t j = 0; j < layer->outputSize; j++) {
                            float sum = 0.0f;
                            for (uint32_t k = 0; k < layer->inputSize; k++) {
                                sum += input[k] * matrix->data[k * layer->outputSize + j];
                            }

                            /* Add bias */
                            if (layer->biases) {
                                sum += layer->biases[j];
                            }

                            /* Store result directly to output */
                            output[j] = sum;
                        }

                        hyperionDestroyMatrixFP32(matrix);
                    }
                    break;

                default:
                    /* Unsupported layer type */
                    return -1;
                }
            }

            /* Copy final activations to output */
            memcpy(output, model->activations[model->activeBuffer],
                   model->tokenizer->tokenCount * sizeof(float));
        }
        break;

    case HYPERION_MODEL_TYPE_TRANSFORMER:
        /* Transformer implementation */
        /* In a real implementation, we would need much more code for transformers */
        /* This is a very simplified version for demonstration purposes */
        {
            /* Embedding layer */
            float *embeddings = model->activations[model->activeBuffer];
            memset(embeddings, 0, inputLength * model->hiddenSize * sizeof(float));

            /* Apply the model layers */
            for (uint32_t i = 0; i < model->layerCount; i++) {
                HyperionLayer *layer = &model->layers[i];

                /* Flip activation buffers */
                model->activeBuffer = 1 - model->activeBuffer;
                float *input        = model->activations[1 - model->activeBuffer];
                float *output       = model->activations[model->activeBuffer];

                /* Apply layer based on type */
                switch (layer->type) {
                case HYPERION_LAYER_EMBEDDING:
                    /* Apply token embeddings */
                    {
                        HyperionMatrixFP32 *matrix = hyperionDequantize4bitToFP32(&layer->weights);

                        if (!matrix) {
                            return -1;
                        }

                        /* Embed each token */
                        for (int j = 0; j < inputLength; j++) {
                            int token = input[j];

                            /* Check token range */
                            if (token < 0 || token >= model->tokenizer->tokenCount) {
                                token = HYPERION_TOKEN_UNKNOWN;
                            }

                            /* Copy embedding for this token */
                            memcpy(output + j * layer->outputSize,
                                   matrix->data + token * layer->outputSize,
                                   layer->outputSize * sizeof(float));
                        }

                        hyperionDestroyMatrixFP32(matrix);
                    }
                    break;

                case HYPERION_LAYER_ATTENTION:
                    /* Simplified self-attention layer */
                    /* A real implementation would be much more complex */
                    {
                        /* For this simplified version, just pass through */
                        memcpy(output, input, inputLength * layer->outputSize * sizeof(float));
                    }
                    break;

                case HYPERION_LAYER_DENSE:
                    /* Dense layer for each position */
                    {
                        for (int j = 0; j < inputLength; j++) {
                            HyperionMatrixFP32 *matrix = hyperionDequantize4bitToFP32(&layer->weights);

                            if (!matrix) {
                                return -1;
                            }

                            /* Matrix multiplication for this position */
                            for (uint32_t k = 0; k < layer->inputSize; k++) {
                                float sum = 0.0f;
                                for (uint32_t l = 0; l < layer->inputSize; l++) {
                                    sum += input[j * layer->inputSize + l] *
                                           matrix->data[l * layer->outputSize + k];
                                }

                                /* Add bias */
                                if (layer->biases) {
                                    sum += layer->biases[k];
                                }

                                /* Store result */
                                output[j * layer->outputSize + k] = sum;
                            }

                            hyperionDestroyMatrixFP32(matrix);

                            /* Apply activation function */
                            switch (layer->activation) {
                            case HYPERION_ACTIVATION_RELU:
                                for (uint32_t j = 0; j < layer->outputSize; j++) {
                                    float *val = &output[j * layer->outputSize + k];
                                    if (*val < 0.0f)
                                        *val = 0.0f;
                                }
                                break;

                            case HYPERION_ACTIVATION_SIGMOID:
                                for (uint32_t j = 0; j < layer->outputSize; j++) {
                                    float *val = &output[j * layer->outputSize + k];
                                    *val       = 1.0f / (1.0f + expf(-*val));
                                }
                                break;

                            case HYPERION_ACTIVATION_TANH:
                                for (uint32_t j = 0; j < layer->outputSize; j++) {
                                    float *val = &output[j * layer->outputSize + k];
                                    *val       = tanhf(*val);
                                }
                                break;

                            case HYPERION_ACTIVATION_GELU:
                                for (uint32_t j = 0; j < layer->outputSize; j++) {
                                    float x   = output[j];
                                    *val       = 0.5f * x *
                                           (1.0f + tanhf(sqrtf(2.0f / 3.14159f) *
                                                         (x + 0.044715f * x * x * x)));
                                }
                                break;

                            default:
                                /* No activation (linear) */
                                break;
                            }
                        }
                    }
                    break;

                case HYPERION_LAYER_LAYERNORM:
                    /* Layer normalization */
                    /* Apply to each position separately */
                    for (int j = 0; j < inputLength; j++) {
                        /* Calculate mean */
                        float mean = 0.0f;
                        for (uint32_t k = 0; k < layer->inputSize; k++) {
                            mean += input[j * layer->inputSize + k];
                        }
                        mean /= layer->inputSize;

                        /* Calculate variance */
                        float variance = 0.0f;
                        for (uint32_t k = 0; k < layer->inputSize; k++) {
                            float diff = input[j * layer->inputSize + k] - mean;
                            variance += diff * diff;
                        }
                        variance /= layer->inputSize;

                        /* Normalize */
                        for (uint32_t k = 0; k < layer->inputSize; k++) {
                            float val =
                                (input[j * layer->inputSize + k] - mean) / sqrtf(variance + 1e-5f);

                            /* Scale and shift (using weights and biases) */
                            if (layer->biases) {
                                val = val * layer->biases[k] + layer->biases[layer->inputSize + k];
                            }

                            output[j * layer->inputSize + k] = val;
                        }
                    }
                    break;

                case HYPERION_LAYER_OUTPUT:
                    /* Output layer */
                    /* For transformers, use only the last position */
                    {
                        HyperionMatrixFP32 *matrix = hyperionDequantize4bitToFP32(&layer->weights);

                        if (!matrix) {
                            return -1;
                        }

                        /* For the output layer, we use only the last token's representation */
                        float *lastTokenRep = input + (inputLength - 1) * layer->inputSize;

                        /* Matrix multiplication */
                        for (uint32_t j = 0; j < layer->outputSize; j++) {
                            float sum = 0.0f;
                            for (uint32_t k = 0; k < layer->inputSize; k++) {
                                sum += lastTokenRep[k] * matrix->data[k * layer->outputSize + j];
                            }

                            /* Add bias */
                            if (layer->biases) {
                                sum += layer->biases[j];
                            }

                            /* Store result */
                            output[j] = sum;
                        }

                        hyperionDestroyMatrixFP32(matrix);
                    }
                    break;

                default:
                    /* Unsupported layer type */
                    return -1;
                }
            }

            /* Copy final activations to output */
            if (model->tokenizer->tokenCount <= model->layerCount) {
                /* Vocabulary too small, there's a problem */
                return -1;
            }

            /* Get the final output layer's values */
            float *finalOutput = model->activations[model->activeBuffer];
            memcpy(output, finalOutput, model->tokenizer->tokenCount * sizeof(float));
        }
        break;

    default:
        /* Unsupported model type */
        return -1;
    }

    return 0;
}

int hyperionGenerateText(HyperionModel *model, const HyperionGenerationParams *params,
                       int *outputTokens, int maxOutputTokens)
{
    if (!model || !params || !outputTokens || maxOutputTokens <= 0) {
        return 0;
    }

    /* Initialize random number generator */
    hyperionSamplingSeedRandom(params->seed);

    /* Apply generation style adjustments */
    HyperionGenerationParams actualParams = *params; // Create a mutable copy
    applyGenerationStyle(&actualParams, params->style);

    /* Check if prompt is provided */
    if (!actualParams.promptTokens || actualParams.promptLength == 0) {
        /* Start with BOS token */
        outputTokens[0] = HYPERION_TOKEN_BOS;

        int numTokens = 1;

        /* Generate tokens */
        while (numTokens < maxOutputTokens && numTokens < actualParams.maxTokens) {
            /* Forward pass */
            float *logits = (float *)HYPERION_MALLOC(model->tokenizer->tokenCount * sizeof(float));
            if (!logits) {
                /* Memory allocation failed */
                break;
            }

            /* Get logits for next token */
            int result = hyperionModelForward(model, outputTokens, numTokens, logits);
            if (result != 0) {
                HYPERION_FREE(logits);
                break;
            }

            /* Sample next token */
            int nextToken = hyperionSampleToken(logits, model->tokenizer->tokenCount, &actualParams);
            HYPERION_FREE(logits);

            /* Check for EOS token */
            if (nextToken == HYPERION_TOKEN_EOS) {
                break;
            }

            /* Add token to output */
            outputTokens[numTokens++] = nextToken;
        }

        return numTokens;
    }
    else {
        /* Start with prompt */
        if (actualParams.promptLength > maxOutputTokens) {
            /* Prompt too long */
            return 0;
        }

        /* Copy prompt */
        memcpy(outputTokens, actualParams.promptTokens, actualParams.promptLength * sizeof(int));
        int numTokens = actualParams.promptLength;

        /* Generate tokens */
        while (numTokens < maxOutputTokens && numTokens < actualParams.maxTokens) {
            /* Forward pass */
            float *logits = (float *)HYPERION_MALLOC(model->tokenizer->tokenCount * sizeof(float));
            if (!logits) {
                /* Memory allocation failed */
                break;
            }

            /* Get logits for next token */
            int contextSize = numTokens;
            if (contextSize > model->contextSize) {
                contextSize = model->contextSize;
            }

            int result = hyperionModelForward(model, outputTokens + numTokens - contextSize,
                                            contextSize, logits);
            if (result != 0) {
                HYPERION_FREE(logits);
                break;
            }

            /* Sample next token */
            int nextToken = hyperionSampleToken(logits, model->tokenizer->tokenCount, &actualParams);
            HYPERION_FREE(logits);

            /* Check for EOS token */
            if (nextToken == HYPERION_TOKEN_EOS) {
                break;
            }

            /* Add token to output */
            outputTokens[numTokens++] = nextToken;
        }

        return numTokens;
    }
}

/**
 * Convert a model to 4-bit quantization
 */
int hyperionQuantizeModel(HyperionModel *model)
{
    if (!model) {
        return -1;
    }

    /* Each layer is already 4-bit quantized during loading */
    /* This function is a placeholder for higher-precision models that need conversion */

    return 0;
}