#ifndef HYPERION_TEXT_SAMPLING_H
#define HYPERION_TEXT_SAMPLING_H

#include <stdint.h>
#include "generate.h"

void hyperionSamplingSeedRandom(uint32_t seed);
float hyperionSamplingRandomFloat(void);
void hyperionSamplingApplyTemperature(float *logits, uint32_t size, float temperature);
void hyperionSamplingSoftmax(float *logits, uint32_t size);
int hyperionSamplingSampleTopK(const float *probs, uint32_t size, uint32_t k);
int hyperionSamplingSampleTopP(const float *probs, uint32_t size, float p);
int hyperionSampleToken(const float *output, int vocabSize,
                        const HyperionGenerationParams *params);

#endif /* HYPERION_TEXT_SAMPLING_H */
