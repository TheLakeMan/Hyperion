#include "model.h"
#include <stdlib.h>
#include <string.h>

int hyperionTextModelInit(HyperionTextModel *model, size_t inputDim, size_t outputDim,
                          const float *weights, const char *adapterPath) {
    if (!model || !weights || inputDim == 0 || outputDim == 0) {
        return 1;
    }

    size_t weightCount = inputDim * outputDim;
    float *copiedWeights = (float *)calloc(weightCount, sizeof(float));
    if (!copiedWeights) {
        return 1;
    }

    memcpy(copiedWeights, weights, weightCount * sizeof(float));

    model->inputDim = inputDim;
    model->outputDim = outputDim;
    model->weights = copiedWeights;
    model->adapter.loaded = 0;
    model->adapter.A = NULL;
    model->adapter.B = NULL;
    model->adapter.alpha = 1.0f;
    model->adapter.rows = 0;
    model->adapter.cols = 0;
    model->adapter.rank = 0;

    if (adapterPath && adapterPath[0] != '\0') {
        if (hyperionLoRAAdapterLoad(adapterPath, &model->adapter) != 0) {
            hyperionTextModelFree(model);
            return 1;
        }
        if (model->adapter.rows != outputDim || model->adapter.cols != inputDim) {
            hyperionTextModelFree(model);
            return 1;
        }
    }

    return 0;
}

void hyperionTextModelFree(HyperionTextModel *model) {
    if (!model) {
        return;
    }
    hyperionLoRAAdapterFree(&model->adapter);
    free(model->weights);
    model->weights = NULL;
    model->inputDim = 0;
    model->outputDim = 0;
}

void hyperionTextModelForward(const HyperionTextModel *model, const float *input, float *output) {
    if (!model || !input || !output || !model->weights) {
        return;
    }

    for (size_t r = 0; r < model->outputDim; ++r) {
        float value = 0.0f;
        for (size_t c = 0; c < model->inputDim; ++c) {
            value += model->weights[r * model->inputDim + c] * input[c];
        }
        output[r] = value;
    }

    if (model->adapter.loaded) {
        hyperionLoRAApply(&model->adapter, input, output);
    }
}
