#include "models/text/lora.h"
#include "models/text/model.h"
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void write_adapter_file(int fd, size_t rows, size_t cols, size_t rank, float alpha,
                               const float *A, const float *B) {
    FILE *file = fdopen(fd, "w");
    assert(file != NULL);
    fprintf(file, "%zu %zu %zu %f\n", rows, cols, rank, alpha);
    for (size_t i = 0; i < rows * rank; ++i) {
        fprintf(file, "%f ", A[i]);
    }
    fprintf(file, "\n");
    for (size_t i = 0; i < rank * cols; ++i) {
        fprintf(file, "%f ", B[i]);
    }
    fprintf(file, "\n");
    fflush(file);
    fclose(file);
}

static char *create_temp_path(void) {
    char *path = strdup("/tmp/hyperion_adapterXXXXXX");
    assert(path != NULL);
    int fd = mkstemp(path);
    assert(fd != -1);
    close(fd);
    return path;
}

static void test_adapter_load_parses_values(void) {
    size_t rows = 2;
    size_t cols = 3;
    size_t rank = 2;
    float alpha = 2.0f;
    float A[4] = {0.1f, 0.2f, 0.3f, 0.4f};
    float B[6] = {0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f};

    char *path = create_temp_path();
    int fd = open(path, O_WRONLY);
    assert(fd != -1);
    write_adapter_file(fd, rows, cols, rank, alpha, A, B);

    HyperionLoRAAdapter adapter = {0};
    assert(hyperionLoRAAdapterLoad(path, &adapter) == 0);
    assert(adapter.rows == rows);
    assert(adapter.cols == cols);
    assert(adapter.rank == rank);
    assert(fabsf(adapter.alpha - alpha) < 1e-6f);
    for (size_t i = 0; i < rows * rank; ++i) {
        assert(fabsf(adapter.A[i] - A[i]) < 1e-6f);
    }
    for (size_t i = 0; i < rank * cols; ++i) {
        assert(fabsf(adapter.B[i] - B[i]) < 1e-6f);
    }

    hyperionLoRAAdapterFree(&adapter);
    unlink(path);
    free(path);
}

static void test_forward_without_adapter_matches_base(void) {
    float weights[6] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    float input[3] = {1.0f, 2.0f, 3.0f};
    float output[2] = {0.0f, 0.0f};

    HyperionTextModel model;
    assert(hyperionTextModelInit(&model, 3, 2, weights, NULL) == 0);
    hyperionTextModelForward(&model, input, output);

    assert(fabsf(output[0] - 1.0f) < 1e-6f);
    assert(fabsf(output[1] - 2.0f) < 1e-6f);
    hyperionTextModelFree(&model);
}

static void test_forward_with_adapter_applies_delta(void) {
    size_t rows = 2;
    size_t cols = 3;
    size_t rank = 2;
    float alpha = 1.0f;
    float A[4] = {0.5f, 0.6f, 0.7f, 0.8f};
    float B[6] = {0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f};

    char *path = create_temp_path();
    int fd = open(path, O_WRONLY);
    assert(fd != -1);
    write_adapter_file(fd, rows, cols, rank, alpha, A, B);

    float weights[6] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    float input[3] = {1.0f, 2.0f, 3.0f};
    float output[2] = {0.0f, 0.0f};

    HyperionTextModel model;
    assert(hyperionTextModelInit(&model, 3, 2, weights, path) == 0);
    hyperionTextModelForward(&model, input, output);

    /*
     * B * x = [0.14, 0.32]
     * A * (B * x) = [0.262, 0.354]
     * Base output = [1, 2]
     * Final output = [1.262, 2.354]
     */
    assert(fabsf(output[0] - 1.262f) < 1e-3f);
    assert(fabsf(output[1] - 2.354f) < 1e-3f);

    hyperionTextModelFree(&model);
    unlink(path);
    free(path);
}

void run_text_model_tests(void) {
    test_adapter_load_parses_values();
    test_forward_without_adapter_matches_base();
    test_forward_with_adapter_applies_delta();
    printf("All text model tests passed.\n");
}
