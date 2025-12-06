#if !defined(_WIN32)
#define _GNU_SOURCE
#endif

#include "interface/cli.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(_WIN32)
#include <time.h>
#endif

#if defined(_WIN32)
#define HYPERION_HAS_POSIX_TESTS 0
#else
#define HYPERION_HAS_POSIX_TESTS 1
#endif

static void test_default_context(void) {
    HyperionCLIContext ctx;
    ctx.params.maxTokens = 0;
    ctx.params.temperature = 0.0f;

    assert(hyperionCLIInit(&ctx) == 0);
    assert(ctx.interactive == false);
    assert(ctx.verbose == false);
    assert(ctx.memReport == false);
}

static void test_argument_parsing(void) {
    HyperionCLIContext ctx;
    hyperionCLIInit(&ctx);

    char *argv[] = {"hyperion", "--interactive", "--verbose", "--mem-report", "--seed", "42"};
    assert(hyperionCLIParseArgs(&ctx, 6, argv) == 0);
    assert(ctx.interactive == true);
    assert(ctx.verbose == true);
    assert(ctx.memReport == true);
    assert(ctx.params.seed == 42);
}

#if HYPERION_HAS_POSIX_TESTS
static void test_posix_sleep_available(void) {
    struct timespec ts = {0, 1};
    nanosleep(&ts, NULL);
}

static void test_verbose_output_streaming(void) {
    HyperionCLIContext ctx;
    hyperionCLIInit(&ctx);
    ctx.verbose = true;
    ctx.params.maxTokens = 16;
    ctx.params.temperature = 0.25f;

    char *buffer = NULL;
    size_t size = 0;
    FILE *stream = open_memstream(&buffer, &size);
    assert(stream != NULL);

    FILE *original_stdout = stdout;
    stdout = stream;

    assert(hyperionCLIRun(&ctx, 0, NULL) == 0);

    fflush(stream);
    stdout = original_stdout;
    fclose(stream);

    assert(buffer != NULL);
    assert(size > 0);
    free(buffer);
}

static void test_seed_is_propagated_to_model(void) {
    HyperionGenerationParams params;
    hyperionGenerationSetDefaults(&params);
    params.seed = 1234;

    HyperionModel modelA;
    HyperionModel modelB;

    assert(hyperionModelInit(&modelA, &params) == 0);
    assert(hyperionModelInit(&modelB, &params) == 0);

    for (int i = 0; i < 5; ++i) {
        int tokenA = hyperionModelSampleToken(&modelA, 1000);
        int tokenB = hyperionModelSampleToken(&modelB, 1000);
        assert(tokenA == tokenB);
    }

    hyperionModelCleanup(&modelA);
    hyperionModelCleanup(&modelB);
}
#endif

void run_cli_tests(void) {
    test_default_context();
    test_argument_parsing();
#if HYPERION_HAS_POSIX_TESTS
    test_posix_sleep_available();
    test_verbose_output_streaming();
#else
    printf("Skipping POSIX-specific CLI tests on this platform.\n");
#endif
    test_seed_is_propagated_to_model();
    printf("All CLI tests passed.\n");
}
