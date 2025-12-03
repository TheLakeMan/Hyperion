#include "interface/cli.h"
#include <assert.h>
#include <stdio.h>

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

    char *argv[] = {"hyperion", "--interactive", "--verbose", "--mem-report"};
    assert(hyperionCLIParseArgs(&ctx, 4, argv) == 0);
    assert(ctx.interactive == true);
    assert(ctx.verbose == true);
    assert(ctx.memReport == true);
}

void run_cli_tests(void) {
    test_default_context();
    test_argument_parsing();
    printf("All CLI tests passed.\n");
}
