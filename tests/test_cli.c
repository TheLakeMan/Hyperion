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
}

static void test_argument_parsing(void) {
    HyperionCLIContext ctx;
    hyperionCLIInit(&ctx);

    char *argv[] = {"hyperion", "--interactive", "--verbose"};
    assert(hyperionCLIParseArgs(&ctx, 3, argv) == 0);
    assert(ctx.interactive == true);
    assert(ctx.verbose == true);
}

int main(void) {
    test_default_context();
    test_argument_parsing();
    printf("All CLI tests passed.\n");
    return 0;
}
