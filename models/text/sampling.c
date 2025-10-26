#include "sampling.h"

#include "../../core/memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static unsigned int g_samplingRandState = 1u;

void hyperionSamplingSeedRandom(uint32_t seed)
{
    g_samplingRandState = (seed == 0) ? (unsigned int)time(NULL) : seed;
}

float hyperionSamplingRandomFloat(void)
{
    g_samplingRandState = g_samplingRandState * 1664525u + 1013904223u;
    return (g_samplingRandState & 0x7FFFFFFFu) / (float)0x7FFFFFFF;
}

void hyperionSamplingApplyTemperature(float *logits, uint32_t size, float temperature)
{
    if (!logits || size == 0) {
        return;
    }

    if (temperature <= 0.0f) {
        temperature = 1.0f;
    }

    for (uint32_t i = 0; i < size; i++) {
        logits[i] /= temperature;
    }
}

void hyperionSamplingSoftmax(float *logits, uint32_t size)
{
    if (!logits || size == 0) {
        return;
    }

    float maxLogit = logits[0];
    for (uint32_t i = 1; i < size; i++) {
        if (logits[i] > maxLogit) {
            maxLogit = logits[i];
        }
    }

    float sum = 0.0f;
    for (uint32_t i = 0; i < size; i++) {
        logits[i] = expf(logits[i] - maxLogit);
        sum += logits[i];
    }

    if (sum > 0.0f) {
        for (uint32_t i = 0; i < size; i++) {
            logits[i] /= sum;
        }
    }
}

int hyperionSamplingSampleTopK(const float *probs, uint32_t size, uint32_t k)
{
    if (!probs || size == 0) {
        return 0;
    }

    if (k == 0) {
        uint32_t bestIdx  = 0;
        float    bestProb = probs[0];
        for (uint32_t i = 1; i < size; ++i) {
            if (probs[i] > bestProb) {
                bestProb = probs[i];
                bestIdx  = i;
            }
        }
        return (int)bestIdx;
    }

    if (k >= size) {
        float sum = 0.0f;
        for (uint32_t i = 0; i < size; i++) {
            sum += probs[i];
        }

        float r      = hyperionSamplingRandomFloat() * sum;
        float cumSum = 0.0f;

        for (uint32_t i = 0; i < size; i++) {
            cumSum += probs[i];
            if (r < cumSum) {
                return (int)i;
            }
        }

        return (int)(size - 1u);
    }

    float *probsCopy = (float *)HYPERION_MALLOC(size * sizeof(float));
    if (!probsCopy) {
        return 0;
    }

    memcpy(probsCopy, probs, size * sizeof(float));

    uint32_t *topIndices = (uint32_t *)HYPERION_MALLOC(k * sizeof(uint32_t));
    if (!topIndices) {
        HYPERION_FREE(probsCopy);
        return 0;
    }

    for (uint32_t i = 0; i < k; i++) {
        float    maxProb = -1.0f;
        uint32_t maxIdx  = 0;

        for (uint32_t j = 0; j < size; j++) {
            if (probsCopy[j] > maxProb) {
                maxProb = probsCopy[j];
                maxIdx  = j;
            }
        }

        topIndices[i]     = maxIdx;
        probsCopy[maxIdx] = -1.0f;
    }

    float sum = 0.0f;
    for (uint32_t i = 0; i < k; i++) {
        sum += probs[topIndices[i]];
    }

    int result = (int)topIndices[0];

    if (sum > 0.0f) {
        float r      = hyperionSamplingRandomFloat() * sum;
        float cumSum = 0.0f;

        for (uint32_t i = 0; i < k; i++) {
            cumSum += probs[topIndices[i]];
            if (r < cumSum) {
                result = (int)topIndices[i];
                break;
            }
        }
    }

    HYPERION_FREE(probsCopy);
    HYPERION_FREE(topIndices);

    return result;
}

int hyperionSamplingSampleTopP(const float *probs, uint32_t size, float p)
{
    if (!probs || size == 0) {
        return 0;
    }

    if (p >= 1.0f) {
        float sum = 0.0f;
        for (uint32_t i = 0; i < size; i++) {
            sum += probs[i];
        }

        float r      = hyperionSamplingRandomFloat() * sum;
        float cumSum = 0.0f;

        for (uint32_t i = 0; i < size; i++) {
            cumSum += probs[i];
            if (r < cumSum) {
                return (int)i;
            }
        }

        return (int)(size - 1u);
    }

    struct ProbIndex {
        float    prob;
        uint32_t index;
    } *probIndices = (struct ProbIndex *)HYPERION_MALLOC(size * sizeof(struct ProbIndex));

    if (!probIndices) {
        return 0;
    }

    for (uint32_t i = 0; i < size; i++) {
        probIndices[i].prob  = probs[i];
        probIndices[i].index = i;
    }

    for (uint32_t i = 0; i < size - 1; i++) {
        for (uint32_t j = i + 1; j < size; j++) {
            if (probIndices[j].prob > probIndices[i].prob) {
                struct ProbIndex temp = probIndices[i];
                probIndices[i]        = probIndices[j];
                probIndices[j]        = temp;
            }
        }
    }

    float    cumSum    = 0.0f;
    uint32_t cutoffIdx = 0;

    for (uint32_t i = 0; i < size; i++) {
        cumSum += probIndices[i].prob;
        if (cumSum >= p) {
            cutoffIdx = i + 1;
            break;
        }
    }

    if (cutoffIdx == 0) {
        cutoffIdx = size;
    }

    float sum = 0.0f;
    for (uint32_t i = 0; i < cutoffIdx; i++) {
        sum += probIndices[i].prob;
    }

    int result = (int)probIndices[0].index;

    if (sum > 0.0f) {
        float r = hyperionSamplingRandomFloat() * sum;
        cumSum  = 0.0f;

        for (uint32_t i = 0; i < cutoffIdx; i++) {
            cumSum += probIndices[i].prob;
            if (r < cumSum) {
                result = (int)probIndices[i].index;
                break;
            }
        }
    }

    HYPERION_FREE(probIndices);

    return result;
}

int hyperionSampleToken(const float *output, int vocabSize,
                        const HyperionGenerationParams *params)
{
    if (!output || !params || vocabSize <= 0) {
        return 0;
    }

    float *probs = (float *)HYPERION_MALLOC((size_t)vocabSize * sizeof(float));
    if (!probs) {
        return 0;
    }

    memcpy(probs, output, (size_t)vocabSize * sizeof(float));
    hyperionSamplingApplyTemperature(probs, (uint32_t)vocabSize, params->temperature);
    hyperionSamplingSoftmax(probs, (uint32_t)vocabSize);

    int token = 0;

    switch (params->samplingMethod) {
    case HYPERION_SAMPLING_GREEDY:
        for (int i = 1; i < vocabSize; i++) {
            if (probs[i] > probs[token]) {
                token = i;
            }
        }
        break;

    case HYPERION_SAMPLING_TOP_K:
        token = hyperionSamplingSampleTopK(probs, (uint32_t)vocabSize, params->topK);
        break;

    case HYPERION_SAMPLING_TOP_P:
        token = hyperionSamplingSampleTopP(probs, (uint32_t)vocabSize, params->topP);
        break;

    case HYPERION_SAMPLING_TEMPERATURE: {
        float r      = hyperionSamplingRandomFloat();
        float cumSum = 0.0f;

        for (int i = 0; i < vocabSize; i++) {
            cumSum += probs[i];
            if (r < cumSum) {
                token = i;
                break;
            }
        }
    } break;

    default:
        for (int i = 1; i < vocabSize; i++) {
            if (probs[i] > probs[token]) {
                token = i;
            }
        }
        break;
    }

    HYPERION_FREE(probs);

    return token;
}
