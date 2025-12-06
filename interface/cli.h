#ifndef HYPERION_CLI_H
#define HYPERION_CLI_H

#include <stdbool.h>

#include "core/generation.h"

typedef struct {
    HyperionGenerationParams params;
    bool interactive;
    bool verbose;
    bool memReport;
} HyperionCLIContext;

int hyperionCLIInit(HyperionCLIContext *context);
int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv);
int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv);
void hyperionCLICleanup(HyperionCLIContext *context);

#endif // HYPERION_CLI_H
