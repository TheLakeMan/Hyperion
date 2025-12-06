#include "cli.h"
#include "core/config_persist.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HYPERION_DEFAULT_MAX_TOKENS 100
#define HYPERION_DEFAULT_SAMPLING_METHOD HYPERION_SAMPLING_TOP_P
#define HYPERION_DEFAULT_TEMPERATURE 0.7f
#define HYPERION_DEFAULT_TOP_K 40
#define HYPERION_DEFAULT_TOP_P 0.9f
#define HYPERION_DEFAULT_SEED 0
#define HYPERION_DEFAULT_CONFIG_PATH "~/.hyperionrc"

static void hyperionCLISetDefaults(HyperionCLIContext *context) {
    context->interactive = false;
    context->verbose = false;
    context->memReport = false;
    context->saveConfig = false;

    context->params.maxTokens = HYPERION_DEFAULT_MAX_TOKENS;
    context->params.samplingMethod = HYPERION_DEFAULT_SAMPLING_METHOD;
    context->params.temperature = HYPERION_DEFAULT_TEMPERATURE;
    context->params.topK = HYPERION_DEFAULT_TOP_K;
    context->params.topP = HYPERION_DEFAULT_TOP_P;
    context->params.seed = HYPERION_DEFAULT_SEED;
}

static int hyperionCLIEqualsIgnoreCase(const char *a, const char *b) {
    if (!a || !b) {
        return 0;
    }

    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static void hyperionCLIExpandPath(const char *path, char *buffer, size_t buffer_size) {
    if (!path || !buffer || buffer_size == 0) {
        return;
    }

    if (path[0] == '~') {
        const char *home = getenv("HOME");
#ifdef _WIN32
        if (!home) {
            home = getenv("USERPROFILE");
        }
#endif
        if (!home) {
            home = ".";
        }

        snprintf(buffer, buffer_size, "%s%s", home, path + 1);
    } else {
        snprintf(buffer, buffer_size, "%s", path);
    }
}

static int parseBoolean(const char *value, bool *output) {
    if (!value || !output) {
        return 1;
    }

    if (hyperionCLIEqualsIgnoreCase(value, "true") || strcmp(value, "1") == 0) {
        *output = true;
        return 0;
    }
    if (hyperionCLIEqualsIgnoreCase(value, "false") || strcmp(value, "0") == 0) {
        *output = false;
        return 0;
    }
    return 1;
}

static int hyperionCLIConfigCallback(const char *key, const char *value, void *user_data) {
    HyperionCLIContext *context = (HyperionCLIContext *)user_data;
    if (!context || !key || !value) {
        return 1;
    }

    if (strcmp(key, "interactive") == 0) {
        return parseBoolean(value, &context->interactive);
    }
    if (strcmp(key, "verbose") == 0) {
        return parseBoolean(value, &context->verbose);
    }
    if (strcmp(key, "mem_report") == 0) {
        return parseBoolean(value, &context->memReport);
    }
    if (strcmp(key, "max_tokens") == 0) {
        context->params.maxTokens = atoi(value);
        return 0;
    }
    if (strcmp(key, "sampling_method") == 0) {
        context->params.samplingMethod = atoi(value);
        return 0;
    }
    if (strcmp(key, "temperature") == 0) {
        context->params.temperature = (float)atof(value);
        return 0;
    }
    if (strcmp(key, "top_k") == 0) {
        context->params.topK = atoi(value);
        return 0;
    }
    if (strcmp(key, "top_p") == 0) {
        context->params.topP = (float)atof(value);
        return 0;
    }
    if (strcmp(key, "seed") == 0) {
        context->params.seed = atoi(value);
        return 0;
    }

    return 0;
}

int hyperionCLILoadConfig(HyperionCLIContext *context, const char *path) {
    if (!context || !path) {
        return 1;
    }

    char expandedPath[sizeof(context->configPath)];
    hyperionCLIExpandPath(path, expandedPath, sizeof(expandedPath));
    return hyperionConfigParseKeyValues(expandedPath, hyperionCLIConfigCallback, context);
}

int hyperionCLISaveConfig(const HyperionCLIContext *context, const char *path) {
    if (!context || !path) {
        return 1;
    }

    char expandedPath[sizeof(context->configPath)];
    hyperionCLIExpandPath(path, expandedPath, sizeof(expandedPath));

    char maxTokens[32];
    snprintf(maxTokens, sizeof(maxTokens), "%d", context->params.maxTokens);

    char samplingMethod[32];
    snprintf(samplingMethod, sizeof(samplingMethod), "%d", context->params.samplingMethod);

    char temperature[32];
    snprintf(temperature, sizeof(temperature), "%.4f", (double)context->params.temperature);

    char topK[32];
    snprintf(topK, sizeof(topK), "%d", context->params.topK);

    char topP[32];
    snprintf(topP, sizeof(topP), "%.4f", (double)context->params.topP);

    char seed[32];
    snprintf(seed, sizeof(seed), "%d", context->params.seed);

    HyperionKeyValue pairs[] = {
        {"interactive", context->interactive ? "true" : "false"},
        {"verbose", context->verbose ? "true" : "false"},
        {"mem_report", context->memReport ? "true" : "false"},
        {"max_tokens", maxTokens},
        {"sampling_method", samplingMethod},
        {"temperature", temperature},
        {"top_k", topK},
        {"top_p", topP},
        {"seed", seed},
    };

    return hyperionConfigWriteKeyValues(expandedPath, pairs, sizeof(pairs) / sizeof(pairs[0]));
}

int hyperionCLIInit(HyperionCLIContext *context) {
    if (!context) {
        return 1;
    }

    hyperionCLISetDefaults(context);
    hyperionCLIExpandPath(HYPERION_DEFAULT_CONFIG_PATH, context->configPath, sizeof(context->configPath));
    return hyperionCLILoadConfig(context, context->configPath);
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
        } else if (strcmp(argv[i], "--config-file") == 0 || strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for %s\n", argv[i]);
                return 1;
            }
            ++i;
            hyperionCLIExpandPath(argv[i], context->configPath, sizeof(context->configPath));
            if (hyperionCLILoadConfig(context, context->configPath) != 0) {
                fprintf(stderr, "Failed to load config from %s\n", context->configPath);
                return 1;
            }
        } else if (strcmp(argv[i], "--save-config") == 0) {
            context->saveConfig = true;
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

    if (context->saveConfig) {
        if (hyperionCLISaveConfig(context, context->configPath) != 0) {
            fprintf(stderr, "Failed to save configuration to %s\n", context->configPath);
            return 1;
        }
        if (context->verbose) {
            printf("Configuration saved to %s\n", context->configPath);
        }
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
