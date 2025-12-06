#include "lora.h"
#include <stdio.h>
#include <stdlib.h>

static int read_floats(FILE *file, float *buffer, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        if (fscanf(file, "%f", &buffer[i]) != 1) {
            return 1;
        }
    }
    return 0;
}

int hyperionLoRAAdapterLoad(const char *path, HyperionLoRAAdapter *adapter) {
    if (!path || !adapter) {
        return 1;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return 1;
    }

    size_t rows = 0;
    size_t cols = 0;
    size_t rank = 0;
    float alpha = 1.0f;

    if (fscanf(file, "%zu %zu %zu %f", &rows, &cols, &rank, &alpha) != 4) {
        fclose(file);
        return 1;
    }

    if (rows == 0 || cols == 0 || rank == 0) {
        fclose(file);
        return 1;
    }

    float *A = (float *)calloc(rows * rank, sizeof(float));
    float *B = (float *)calloc(rank * cols, sizeof(float));
    if (!A || !B) {
        free(A);
        free(B);
        fclose(file);
        return 1;
    }

    int error = read_floats(file, A, rows * rank);
    error |= read_floats(file, B, rank * cols);
    fclose(file);

    if (error) {
        free(A);
        free(B);
        return 1;
    }

    hyperionLoRAAdapterFree(adapter);

    adapter->rows = rows;
    adapter->cols = cols;
    adapter->rank = rank;
    adapter->alpha = alpha;
    adapter->A = A;
    adapter->B = B;
    adapter->loaded = 1;
    return 0;
}

void hyperionLoRAAdapterFree(HyperionLoRAAdapter *adapter) {
    if (!adapter || !adapter->loaded) {
        return;
    }
    free(adapter->A);
    free(adapter->B);
    adapter->A = NULL;
    adapter->B = NULL;
    adapter->rows = 0;
    adapter->cols = 0;
    adapter->rank = 0;
    adapter->alpha = 1.0f;
    adapter->loaded = 0;
}

void hyperionLoRAApply(const HyperionLoRAAdapter *adapter, const float *input, float *output) {
    if (!adapter || !adapter->loaded || !input || !output) {
        return;
    }

    float *projection = (float *)calloc(adapter->rank, sizeof(float));
    if (!projection) {
        return;
    }

    for (size_t k = 0; k < adapter->rank; ++k) {
        for (size_t c = 0; c < adapter->cols; ++c) {
            projection[k] += adapter->B[k * adapter->cols + c] * input[c];
        }
    }

    for (size_t r = 0; r < adapter->rows; ++r) {
        float delta = 0.0f;
        for (size_t k = 0; k < adapter->rank; ++k) {
            delta += adapter->A[r * adapter->rank + k] * projection[k];
        }
        output[r] += adapter->alpha * delta;
    }

    free(projection);
}
