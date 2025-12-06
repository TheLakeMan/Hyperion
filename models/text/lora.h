#ifndef HYPERION_TEXT_LORA_H
#define HYPERION_TEXT_LORA_H

#include <stddef.h>

typedef struct {
    size_t rows;
    size_t cols;
    size_t rank;
    float alpha;
    float *A; /* rows x rank */
    float *B; /* rank x cols */
    int loaded;
} HyperionLoRAAdapter;

int hyperionLoRAAdapterLoad(const char *path, HyperionLoRAAdapter *adapter);
void hyperionLoRAAdapterFree(HyperionLoRAAdapter *adapter);
void hyperionLoRAApply(const HyperionLoRAAdapter *adapter, const float *input, float *output);

#endif /* HYPERION_TEXT_LORA_H */
