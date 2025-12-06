#include "cli.h"
#include <stdio.h>
#include <string.h>

static void hyperionCLIPrintUsage(const HyperionCLIContext *context, const char *programName) {
    const char *executable = programName != NULL ? programName : "hyperion";

    printf("Usage: %s [options]\n", executable);
    printf("Options:\n");
    printf("  -h, --help                 Show this help message\n");
    printf("  -i, --interactive          Run in interactive mode (default: %s)\n",
           (context && context->interactive) ? "true" : "false");
    printf("  -v, --verbose              Enable verbose output (default: %s)\n",
           (context && context->verbose) ? "true" : "false");
    printf("      --mem-report, --debug-mem   Print memory report on exit (default: %s)\n",
           (context && context->memReport) ? "true" : "false");

    if (context) {
        printf("Generation defaults:\n");
        printf("  max_tokens: %d\n", context->params.maxTokens);
        printf("  temperature: %.2f\n", context->params.temperature);
        printf("  top_k: %d\n", context->params.topK);
        printf("  top_p: %.2f\n", context->params.topP);
        printf("  seed: %d\n", context->params.seed);
    }
}

int hyperionCLIInit(HyperionCLIContext *context) {
    if (!context) {
        return HYPERION_CLI_ERROR;
    }
    context->interactive = false;
    context->verbose = false;
    context->memReport = false;
    return HYPERION_CLI_SUCCESS;
}

int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv) {
    if (!context) {
        return HYPERION_CLI_ERROR;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            hyperionCLIPrintUsage(context, argv[0]);
            return HYPERION_CLI_EXIT;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            context->interactive = true;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            context->verbose = true;
        } else if (strcmp(argv[i], "--mem-report") == 0 || strcmp(argv[i], "--debug-mem") == 0) {
            context->memReport = true;
        } else {
            fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
            hyperionCLIPrintUsage(context, argv[0]);
            return HYPERION_CLI_ERROR;
        }
    }
    return HYPERION_CLI_SUCCESS;
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
