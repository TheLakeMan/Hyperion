#include "generation.h"

#include <stdlib.h>
#include <time.h>

static unsigned int hyperionResolveSeed(const HyperionGenerationParams *params) {
    if (params && params->seed != 0) {
        return (unsigned int)params->seed;
    }

    unsigned int seed = (unsigned int)time(NULL);
#if !defined(_WIN32)
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        seed ^= (unsigned int)ts.tv_nsec;
    }
#endif
    return seed == 0 ? 1u : seed;
}

void hyperionGenerationSetDefaults(HyperionGenerationParams *params) {
    if (!params) {
        return;
    }

    params->maxTokens = 100;
    params->samplingMethod = HYPERION_SAMPLING_TOP_P;
    params->temperature = 0.7f;
    params->topK = 40;
    params->topP = 0.9f;
    params->seed = 0; // Use random seed by default
}

int hyperionModelInit(HyperionModel *model, const HyperionGenerationParams *params) {
    if (!model || !params) {
        return 1;
    }

    unsigned int seed = hyperionResolveSeed(params);
    model->seed = seed;
    model->state = seed;
    return 0;
}

int hyperionModelSampleToken(HyperionModel *model, int vocabSize) {
    if (!model || vocabSize <= 0) {
        return -1;
    }

    model->state = model->state * 1664525u + 1013904223u;
    return (int)(model->state % (unsigned int)vocabSize);
}

unsigned int hyperionModelSeed(const HyperionModel *model) {
    if (!model) {
        return 0;
    }
    return model->seed;
}

void hyperionModelCleanup(HyperionModel *model) {
    if (!model) {
        return;
    }
    model->seed = 0;
    model->state = 0;
}
