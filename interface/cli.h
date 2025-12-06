#ifndef HYPERION_CLI_H
#define HYPERION_CLI_H

#include <stdbool.h>
#include <stddef.h>

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
    bool saveConfig;
    char configPath[256];
} HyperionCLIContext;

int hyperionCLIInit(HyperionCLIContext *context);
int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv);
int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv);
void hyperionCLICleanup(HyperionCLIContext *context);
int hyperionCLILoadConfig(HyperionCLIContext *context, const char *path);
int hyperionCLISaveConfig(const HyperionCLIContext *context, const char *path);

#endif // HYPERION_CLI_H
