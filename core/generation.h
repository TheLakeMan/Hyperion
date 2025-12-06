#ifndef HYPERION_GENERATION_H
#define HYPERION_GENERATION_H

#include <stdbool.h>

// Sampling strategy placeholder
#define HYPERION_SAMPLING_TOP_P 1

/**
 * Generation parameters used for model initialization and sampling.
 */
typedef struct {
    int maxTokens;
    int samplingMethod;
    float temperature;
    int topK;
    float topP;
    int seed;
} HyperionGenerationParams;

/**
 * Lightweight model context holding sampling state.
 */
typedef struct {
    unsigned int seed;
    unsigned int state;
} HyperionModel;

void hyperionGenerationSetDefaults(HyperionGenerationParams *params);
int hyperionModelInit(HyperionModel *model, const HyperionGenerationParams *params);
int hyperionModelSampleToken(HyperionModel *model, int vocabSize);
unsigned int hyperionModelSeed(const HyperionModel *model);
void hyperionModelCleanup(HyperionModel *model);

#endif // HYPERION_GENERATION_H
