#ifndef HYPERION_TEXT_MODEL_H
#define HYPERION_TEXT_MODEL_H

#include <stddef.h>
#include "lora.h"

typedef struct {
    size_t inputDim;
    size_t outputDim;
    float *weights; /* outputDim x inputDim */
    HyperionLoRAAdapter adapter;
} HyperionTextModel;

int hyperionTextModelInit(HyperionTextModel *model, size_t inputDim, size_t outputDim,
                          const float *weights, const char *adapterPath);
void hyperionTextModelFree(HyperionTextModel *model);
void hyperionTextModelForward(const HyperionTextModel *model, const float *input, float *output);

#endif /* HYPERION_TEXT_MODEL_H */
