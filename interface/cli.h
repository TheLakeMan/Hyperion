#ifndef HYPERION_CLI_H
#define HYPERION_CLI_H

#include <stdbool.h>

// Sampling strategy placeholder
#define HYPERION_SAMPLING_TOP_P 1

typedef struct {
    int maxTokens;
    int samplingMethod;
    float temperature;
    int topK;
    float topP;
    int seed;
} HyperionGenerationParams;

typedef struct {
    HyperionGenerationParams params;
    bool interactive;
    bool verbose;
    bool memReport;
} HyperionCLIContext;

#define HYPERION_CLI_SUCCESS 0
#define HYPERION_CLI_ERROR 1
#define HYPERION_CLI_EXIT 2

int hyperionCLIInit(HyperionCLIContext *context);
/**
 * Parse command line arguments.
 *
 * Supported flags:
 *  -h, --help          Print usage and exit
 *  -i, --interactive   Run the CLI in interactive mode
 *  -v, --verbose       Enable verbose logging
 *  --mem-report
 *  --debug-mem         Print the memory report on exit
 *
 * Returns HYPERION_CLI_SUCCESS on success, HYPERION_CLI_EXIT when help was
 * requested, and HYPERION_CLI_ERROR for invalid arguments.
 */
int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv);
int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv);
void hyperionCLICleanup(HyperionCLIContext *context);

#endif // HYPERION_CLI_H
