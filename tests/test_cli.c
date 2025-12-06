#if !defined(_WIN32)
#define _GNU_SOURCE
#endif

#include "interface/cli.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32)
#include <time.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#define HYPERION_HAS_POSIX_TESTS 0
#else
#define HYPERION_HAS_POSIX_TESTS 1
#endif

static void test_default_context(void) {
    HyperionCLIContext ctx;
    assert(hyperionCLIInit(&ctx) == 0);
    assert(ctx.interactive == false);
    assert(ctx.verbose == false);
    assert(ctx.memReport == false);
    assert(ctx.params.maxTokens == 100);
    assert(ctx.params.samplingMethod == HYPERION_SAMPLING_TOP_P);
    assert(ctx.params.temperature > 0.6f && ctx.params.temperature < 0.8f);
    assert(ctx.params.topK == 40);
    assert(ctx.params.topP > 0.8f && ctx.params.topP < 1.0f);
    assert(ctx.params.seed == 0);
}

static void test_argument_parsing(void) {
    HyperionCLIContext ctx;
    hyperionCLIInit(&ctx);

    char *argv[] = {"hyperion", "--interactive", "--verbose", "--mem-report"};
    assert(hyperionCLIParseArgs(&ctx, 4, argv) == 0);
    assert(ctx.interactive == true);
    assert(ctx.verbose == true);
    assert(ctx.memReport == true);
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
#endif

static void create_temp_config_path(char *buffer, size_t size) {
#if HYPERION_HAS_POSIX_TESTS
    char pathTemplate[] = "/tmp/hyperion_configXXXXXX";
    int fd = mkstemp(pathTemplate);
    assert(fd != -1);
    close(fd);
    snprintf(buffer, size, "%s", pathTemplate);
#else
    char *name = tmpnam(NULL);
    assert(name != NULL);
    snprintf(buffer, size, "%s", name);
#endif
}

static void test_save_and_load_round_trip(void) {
    char path[128];
    create_temp_config_path(path, sizeof(path));

    HyperionCLIContext ctx;
    assert(hyperionCLIInit(&ctx) == 0);
    snprintf(ctx.configPath, sizeof(ctx.configPath), "%s", path);

    ctx.interactive = true;
    ctx.verbose = true;
    ctx.memReport = true;
    ctx.params.maxTokens = 256;
    ctx.params.samplingMethod = 2;
    ctx.params.temperature = 0.42f;
    ctx.params.topK = 12;
    ctx.params.topP = 0.55f;
    ctx.params.seed = 9876;

    assert(hyperionCLISaveConfig(&ctx, ctx.configPath) == 0);

    HyperionCLIContext loaded;
    assert(hyperionCLIInit(&loaded) == 0);
    assert(hyperionCLILoadConfig(&loaded, path) == 0);

    assert(loaded.interactive == true);
    assert(loaded.verbose == true);
    assert(loaded.memReport == true);
    assert(loaded.params.maxTokens == 256);
    assert(loaded.params.samplingMethod == 2);
    assert(loaded.params.temperature > 0.41f && loaded.params.temperature < 0.43f);
    assert(loaded.params.topK == 12);
    assert(loaded.params.topP > 0.54f && loaded.params.topP < 0.56f);
    assert(loaded.params.seed == 9876);

    remove(path);
}

static void test_missing_config_file_uses_defaults(void) {
    HyperionCLIContext ctx;
    assert(hyperionCLIInit(&ctx) == 0);
    snprintf(ctx.configPath, sizeof(ctx.configPath), "%s", "./does_not_exist.cfg");

    ctx.params.maxTokens = 500;
    ctx.params.temperature = 2.0f;
    ctx.interactive = true;

    assert(hyperionCLILoadConfig(&ctx, ctx.configPath) == 0);

    assert(ctx.params.maxTokens == 500);
    assert(ctx.params.temperature == 2.0f);
    assert(ctx.interactive == true);
}

void run_cli_tests(void) {
    test_default_context();
    test_argument_parsing();
#if HYPERION_HAS_POSIX_TESTS
    test_posix_sleep_available();
    test_verbose_output_streaming();
#else
    printf("Skipping POSIX-specific CLI tests on this platform.\n");
#endif
    test_save_and_load_round_trip();
    test_missing_config_file_uses_defaults();
    printf("All CLI tests passed.\n");
}
