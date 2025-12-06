#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hyperionCLIInit(HyperionCLIContext *context) {
    if (!context) {
        return 1;
    }
    context->interactive = false;
    context->verbose = false;
    context->memReport = false;
    hyperionGenerationSetDefaults(&context->params);
    return 0;
}

static int hyperionCLIParseSeed(HyperionCLIContext *context, const char *value) {
    char *endptr = NULL;
    long seedValue = strtol(value, &endptr, 10);
    if (endptr == value || *endptr != '\0') {
        return 1;
    }

    context->params.seed = (int)seedValue;
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
        } else if (strcmp(argv[i], "--mem-report") == 0 || strcmp(argv[i], "--debug-mem") == 0) {
            context->memReport = true;
        } else if (strncmp(argv[i], "--seed=", 7) == 0) {
            if (hyperionCLIParseSeed(context, argv[i] + 7) != 0) {
                return 1;
            }
        } else if (strcmp(argv[i], "--seed") == 0) {
            if (i + 1 >= argc) {
                return 1;
            }
            if (hyperionCLIParseSeed(context, argv[i + 1]) != 0) {
                return 1;
            }
            ++i; // consume seed value
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

    HyperionModel model;
    if (hyperionModelInit(&model, &context->params) != 0) {
        return 1;
    }

    if (context->verbose) {
        printf("[hyperion] Starting session with max_tokens=%d, temperature=%.2f, seed=%u\n",\
               context->params.maxTokens, context->params.temperature, hyperionModelSeed(&model));
    }

    if (context->interactive) {
        printf("Interactive mode activated. Press Ctrl+C to exit.\n");
    } else {
        printf("Running in batch mode.\n");
    }

    hyperionModelCleanup(&model);
    return 0;
}

void hyperionCLICleanup(HyperionCLIContext *context) {
    (void)context;
}
