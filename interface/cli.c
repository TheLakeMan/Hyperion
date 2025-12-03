#include "cli.h"
#include <stdio.h>
#include <string.h>

int hyperionCLIInit(HyperionCLIContext *context) {
    if (!context) {
        return 1;
    }
    context->interactive = false;
    context->verbose = false;
    return 0;
}

int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv) {
    if (!context) {
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            context->interactive = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            context->verbose = true;
        }
    }
    return 0;
}

int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (!context) {
        return 1;
    }

    if (context->verbose) {
        printf("[hyperion] Starting session with max_tokens=%d, temperature=%.2f\n",
               context->params.maxTokens, context->params.temperature);
    }

    if (context->interactive) {
        printf("Interactive mode activated. Press Ctrl+C to exit.\n");
    } else {
        printf("Running in batch mode.\n");
    }
    return 0;
}

void hyperionCLICleanup(HyperionCLIContext *context) {
    (void)context;
}
