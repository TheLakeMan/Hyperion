/**
 * Hyperion Command Line Interface Implementation
 */

#include "cli.h"
#include "../core/config.h"           // Potentially needed for config command
#include "../core/io.h"               // For file operations (loading models etc.)
#include "../core/memory.h"           // For memory allocation
#include "../models/text/generate.h"  // For model/tokenizer types
#include "../models/text/tokenizer.h" // For tokenizer type

#include <ctype.h> // For isspace
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Define Hyperion Version (replace with actual versioning later)
#define HYPERION_VERSION "0.1.0-alpha"

/* ----------------- Internal State ----------------- */

static HyperionCommand g_commands[HYPERION_CLI_MAX_COMMANDS];
static int           g_commandCount = 0;

/* ----------------- Helper Functions ----------------- */

// Simple command line parsing into argc/argv
// Note: Modifies the input string by inserting null terminators!
static int parseCommandLine(char *line, char **argv, int maxArgs)
{
    int   argc       = 0;
    char *current     = line;
    char *tokenStart = NULL;
    int   inToken    = 0;
    int   inQuotes   = 0;

    while (*current != '\0' && argc < maxArgs - 1) {
        if (*current == '"') {
            inQuotes = !inQuotes;
            if (!inToken) {
                tokenStart = current + 1; // Start token after quote
                inToken    = 1;
            }
            else if (!inQuotes) {    // Closing quote
                *current     = '\0'; // Terminate token
                argv[argc++] = tokenStart;
                inToken      = 0;
            }
        }
        else if (isspace((unsigned char)*current)) {
            if (inToken && !inQuotes) {
                *current     = '\0'; // Terminate token
                argv[argc++] = tokenStart;
                inToken      = 0;
            }
        }
        else {
            if (!inToken) {
                tokenStart = current;
                inToken    = 1;
            }
        }
        current++;
    }

    // Handle the last token if the line doesn't end with whitespace
    if (inToken) {
        // If still in quotes at the end, it's an error or unterminated string
        if (inQuotes) {
            fprintf(stderr, "Error: Unterminated quote in command.\n");
            return -1; // Indicate parse error
        }
        // No need to terminate, already points within the original string which is null-terminated
        argv[argc++] = tokenStart;
    }

    argv[argc] = NULL; // Null-terminate the argv array
    return argc;
}

// Find a command by name
static HyperionCommand *findCommand(const char *name)
{
    for (int i = 0; i < g_commandCount; ++i) {
        if (strcmp(g_commands[i].name, name) == 0) {
            return &g_commands[i];
        }
    }
    return NULL;
}

static void formatTimestamp(time_t value, char *buffer, size_t length)
{
    if (!buffer || length == 0) return;
    if (value == 0) {
        snprintf(buffer, length, "n/a");
        return;
    }

    struct tm tm_info;
#ifdef _WIN32
    if (localtime_s(&tm_info, &value) != 0) {
        snprintf(buffer, length, "n/a");
        return;
    }
#else
    if (!localtime_r(&value, &tm_info)) {
        snprintf(buffer, length, "n/a");
        return;
    }
#endif
    strftime(buffer, length, "%Y-%m-%d %H:%M:%S", &tm_info);
}

/* ----------------- API Functions Implementation ----------------- */

int hyperionCLIInit(HyperionCLIContext *context)
{
    if (!context)
        return HYPERION_CLI_EXIT_ERROR;

    // Initialize context defaults
    memset(context, 0, sizeof(HyperionCLIContext));

    // CLI state
    context->interactive = 0; // Default to non-interactive
    context->verbose     = 0;

    // Generation parameters
    context->params.maxTokens   = 50; // Use maxTokens
    context->params.temperature = 0.7f;
    context->params.topK        = 50;
    context->params.topP        = 0.9f;

    // MCP and hybrid initialization
    context->mcpClient   = NULL;
    context->hybridGen   = NULL;
    context->useHybrid   = 0; // Default to local-only generation
    context->forceRemote = 0;
    context->forceLocal  = 0;

    g_commandCount = 0;

    context->deploymentManager = hyperionDeploymentManagerCreate(16);
    if (!context->deploymentManager) {
        return HYPERION_CLI_EXIT_ERROR;
    }

    context->monitoringCenter = hyperionMonitoringInstance();
    if (!context->monitoringCenter) {
        return HYPERION_CLI_EXIT_ERROR;
    }

    context->autoScaler = NULL;
    if (hyperionConfigGetBool("autoscale.enabled", 0)) {
        HyperionAutoScalerPolicy policy = {0};
        const char *metric = hyperionConfigGetString("autoscale.metric", "cpu.utilization");
        if (metric) {
            strncpy(policy.metric_name, metric, sizeof(policy.metric_name) - 1);
            policy.metric_name[sizeof(policy.metric_name) - 1] = '\0';
        }
        policy.scale_up_threshold = (double)hyperionConfigGetFloat("autoscale.scale_up_threshold", 75.0f);
        policy.scale_down_threshold = (double)hyperionConfigGetFloat("autoscale.scale_down_threshold", 25.0f);
        policy.scale_step = (size_t)hyperionConfigGetInt("autoscale.scale_step", 1);
        policy.min_replicas = (size_t)hyperionConfigGetInt("autoscale.min_replicas", 1);
        policy.max_replicas = (size_t)hyperionConfigGetInt("autoscale.max_replicas", 10);
        policy.scale_up_cooldown_seconds = hyperionConfigGetInt("autoscale.scale_up_cooldown", 120);
        policy.scale_down_cooldown_seconds = hyperionConfigGetInt("autoscale.scale_down_cooldown", 300);

        context->autoScaler = hyperionAutoScalerCreate(&policy, context->monitoringCenter);
        if (!context->autoScaler) {
            return HYPERION_CLI_EXIT_ERROR;
        }
    }

    // Register built-in commands
    hyperionCLIRegisterCommand("help", "Show help information.", "help [command]", hyperionCommandHelp);
    hyperionCLIRegisterCommand("version", "Show Hyperion version.", "version", hyperionCommandVersion);
    hyperionCLIRegisterCommand("generate", "Generate text using the loaded model.",
                             "generate <prompt>", hyperionCommandGenerate);
    hyperionCLIRegisterCommand("tokenize", "Tokenize input text.", "tokenize <text>",
                             hyperionCommandTokenize);
    hyperionCLIRegisterCommand("model", "Load or inspect the model.", "model load <path> | info",
                             hyperionCommandModel);
    hyperionCLIRegisterCommand("config", "Set or view configuration parameters.",
                             "config [param] [value]", hyperionCommandConfig);
    hyperionCLIRegisterCommand("mcp", "Connect to or control MCP server.",
                             "mcp connect <url> | disconnect | status", hyperionCommandMcp);
    hyperionCLIRegisterCommand("hybrid", "Control hybrid local/remote execution mode.",
                             "hybrid on | off | status | force-local | force-remote",
                             hyperionCommandHybrid);
    hyperionCLIRegisterCommand("deploy", "Manage deployments.",
                             "deploy plan <cfg> | apply <cfg> [--notes <text>] | rollback [version] | status",
                             hyperionCommandDeploy);
    hyperionCLIRegisterCommand("monitor", "Inspect monitoring & analytics data.",
                             "monitor status | logs [--limit N] | reset | alert add <metric> <gt|lt|eq> <threshold> [hits]",
                             hyperionCommandMonitor);
    hyperionCLIRegisterCommand("autoscale", "Evaluate autoscaling decisions.",
                             "autoscale plan [--replicas N] | autoscale policy",
                             hyperionCommandAutoscale);
    hyperionCLIRegisterCommand("exit", "Exit the interactive shell.", "exit", hyperionCommandExit);
    hyperionCLIRegisterCommand("quit", "Exit the interactive shell.", "quit",
                             hyperionCommandExit); // Alias for exit

    // Initialize subsystems if needed (though likely done elsewhere)
    // hyperionIOInit();
    // hyperionMemTrackInit(); // If tracking enabled

    return HYPERION_CLI_EXIT_SUCCESS;
}

void hyperionCLICleanup(HyperionCLIContext *context)
{
    if (!context)
        return;

    // Free context resources - paths
    if (context->modelPath)
        free(context->modelPath);
    if (context->tokenizerPath)
        free(context->tokenizerPath);
    if (context->mcpServerUrl)
        free(context->mcpServerUrl);

    // Clean up model and tokenizer
    if (context->model)
        hyperionDestroyModel(context->model);
    if (context->tokenizer)
        hyperionDestroyTokenizer(context->tokenizer);

    // Clean up MCP client and hybrid generation
    if (context->hybridGen)
        hyperionDestroyHybridGenerate(context->hybridGen);
    if (context->mcpClient) {
        if (hyperionMcpGetConnectionState(context->mcpClient) == HYPERION_MCP_CONNECTED)
            hyperionMcpDisconnect(context->mcpClient);
        hyperionMcpDestroyClient(context->mcpClient);
    }

    if (context->deploymentManager) {
        hyperionDeploymentManagerDestroy(context->deploymentManager);
        context->deploymentManager = NULL;
    }

    if (context->autoScaler) {
        hyperionAutoScalerDestroy(context->autoScaler);
        context->autoScaler = NULL;
    }

    // Reset command registry (optional, as it's static)
    g_commandCount = 0;

    // Cleanup subsystems if initialized here
    // hyperionIOCleanup();
    // hyperionMemTrackCleanup(); // If tracking enabled
}

int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv)
{
    // Basic argument parsing (replace with getopt or similar later if needed)
    // This function is primarily for parsing arguments passed to the program itself,
    // not for parsing commands entered in the interactive shell.

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            context->interactive = 1;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            context->verbose++;
        }
        else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--model") == 0) && i + 1 < argc) {
            // Use HYPERION_FREE/MALLOC if integrated
            if (context->modelPath)
                free(context->modelPath);
            context->modelPath = _strdup(argv[++i]); // Use _strdup
            if (!context->modelPath)
                return HYPERION_CLI_EXIT_ERROR; // Memory error
        }
        else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tokenizer") == 0) &&
                 i + 1 < argc) {
            if (context->tokenizerPath)
                free(context->tokenizerPath);
            context->tokenizerPath = _strdup(argv[++i]); // Use _strdup
            if (!context->tokenizerPath)
                return HYION_CLI_EXIT_ERROR; // Memory error
        }
        else if (strcmp(argv[i], "--temp") == 0 && i + 1 < argc) {
            context->params.temperature = (float)atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--topk") == 0 && i + 1 < argc) {
            context->params.topK = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--topp") == 0 && i + 1 < argc) {
            context->params.topP = (float)atof(argv[++i]);
        }
        else if (strcmp(argv[i], "--maxlen") == 0 && i + 1 < argc) {
            context->params.maxTokens = atoi(argv[++i]); // Use maxTokens
        }
        else if (strcmp(argv[i], "--style") == 0 && i + 1 < argc) {
            const char *styleStr = argv[++i];
            if (strcmp(styleStr, "neutral") == 0) context->params.style = HYPERION_STYLE_NEUTRAL;
            else if (strcmp(styleStr, "formal") == 0) context->params.style = HYPERION_STYLE_FORMAL;
            else if (strcmp(styleStr, "creative") == 0) context->params.style = HYPERION_STYLE_CREATIVE;
            else if (strcmp(styleStr, "concise") == 0) context->params.style = HYPERION_STYLE_CONCISE;
            else if (strcmp(styleStr, "descriptive") == 0) context->params.style = HYPERION_STYLE_DESCRIPTIVE;
            else {
                fprintf(stderr, "Error: Unknown style '%s'. Using neutral.\n", styleStr);
                context->params.style = HYPERION_STYLE_NEUTRAL;
            }
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            hyperionCLIPrintHelp(context, NULL);
            return HYPERION_CLI_EXIT_QUIT; // Exit after showing help
        }
        else {
            // Assume remaining arguments might be a command and its args for non-interactive mode
            // Or potentially a prompt for direct generation
            break; // Stop parsing options here
        }
    }
    return HYPERION_CLI_EXIT_SUCCESS;
}

int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv)
{
    int parseResult = hyperionCLIParseArgs(context, argc, argv);
    if (parseResult != HYPERION_CLI_EXIT_SUCCESS) {
        return parseResult; // Return error or quit code from parsing
    }

    // Load model/tokenizer if paths were provided
    if (context->modelPath) {
        // Call model loading function (implementation needed)
        printf("Info: Attempting to load model from %s\n", context->modelPath);
        // context->model = hyperionLoadModel(context->modelPath); // Example
        // if (!context->model) { fprintf(stderr, "Error loading model.\n"); return
        // HYPERION_CLI_EXIT_ERROR; }
    }
    if (context->tokenizerPath) {
        printf("Info: Attempting to load tokenizer from %s\n", context->tokenizerPath);
        // context->tokenizer = hyperionLoadTokenizer(context->tokenizerPath); // Example
        // if (!context->tokenizer) { fprintf(stderr, "Error loading tokenizer.\n"); return
        // HYPERION_CLI_EXIT_ERROR; }
    }

    if (context->interactive) {
        return hyperionCLIRunShell(context);
    }
    else {
        // Non-interactive mode: try to execute a command from remaining args
        int firstCmdArgIndex = 1; // Find where options stopped
        for (; firstCmdArgIndex < argc; ++firstCmdArgIndex) {
            // Simple check: stop at first arg not starting with '-'? Needs refinement.
            if (argv[firstCmdArgIndex][0] != '-')
                break;
            // Skip option arguments
            if ((strcmp(argv[firstCmdArgIndex], "-m") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "--model") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "-t") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "--tokenizer") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "--temp") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "--topk") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "--topp") == 0 ||
                 strcmp(argv[firstCmdArgIndex], "--maxlen") == 0) &&
                firstCmdArgIndex + 1 < argc) {
                firstCmdArgIndex++;
            }
        }

        if (firstCmdArgIndex < argc) {
            // Treat remaining args as a command
            HyperionCommand *cmd = findCommand(argv[firstCmdArgIndex]);
            if (cmd) {
                return cmd->handler(argc - firstCmdArgIndex, argv + firstCmdArgIndex, context);
            }
            else {
                // If no command matches, maybe treat as a prompt for generate?
                fprintf(stderr, "Error: Unknown command '%s' in non-interactive mode.\n",
                        argv[firstCmdArgIndex]);
                hyperionCLIPrintHelp(context, NULL);
                return HYPERION_CLI_EXIT_ERROR;
            }
        }
        else {
            // No command provided in non-interactive mode, show help or default action?
            fprintf(stderr, "Error: No command specified for non-interactive mode.\n");
            hyperionCLIPrintHelp(context, NULL);
            return HYPERION_CLI_EXIT_ERROR;
        }
    }
}

int hyperionCLIRunShell(HyperionCLIContext *context)
{
    char line[HYPERION_CLI_MAX_COMMAND_LENGTH];
    int  exitCode = HYPERION_CLI_EXIT_SUCCESS;

    printf("Hyperion Interactive Shell (v%s)\n", HYPERION_VERSION);
    printf("Type 'help' for available commands, 'exit' or 'quit' to leave.\n");

    while (exitCode != HYPERION_CLI_EXIT_QUIT) {
        printf("> ");
        fflush(stdout); // Ensure prompt is displayed

        if (fgets(line, sizeof(line), stdin) == NULL) {
            // End of input (e.g., Ctrl+D)
            printf("\nExiting.\n");
            exitCode = HYPERION_CLI_EXIT_QUIT;
            break;
        }

        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;

        // Skip empty lines
        char *trimmed_line = line;
        while (isspace((unsigned char)*trimmed_line))
            trimmed_line++;
        if (*trimmed_line == '\0') {
            continue;
        }

        exitCode = hyperionCLIProcessCommand(context, trimmed_line);

        if (exitCode != HYPERION_CLI_EXIT_SUCCESS && exitCode != HYPERION_CLI_EXIT_QUIT) {
            // Print error message based on exit code?
            fprintf(stderr, "Command failed with exit code %d\n", exitCode);
            // Reset exit code to continue shell unless it was QUIT
            exitCode = HYPERION_CLI_EXIT_SUCCESS;
        }
    }

    return exitCode == HYPERION_CLI_EXIT_QUIT ? HYPERION_CLI_EXIT_SUCCESS
                                            : exitCode; // Return 0 on normal quit
}

int hyperionCLIProcessCommand(HyperionCLIContext *context, const char *command_const)
{
    char  command[HYPERION_CLI_MAX_COMMAND_LENGTH];
    char *argv[HYPERION_CLI_MAX_ARGS];

    // Make a mutable copy for parsing
    // Use safer strncpy_s with truncation
    strncpy_s(command, sizeof(command), command_const, _TRUNCATE);
    // command[sizeof(command) - 1] = '\0'; // strncpy_s guarantees null termination if space allows

    int argc = parseCommandLine(command, argv, HYPERION_CLI_MAX_ARGS);

    if (argc <= 0) {
        return HYPERION_CLI_EXIT_ERROR; // Parsing error or empty command
    }

    HyperionCommand *cmd = findCommand(argv[0]);
    if (cmd) {
        return cmd->handler(argc, argv, context);
    }
    else {
        fprintf(stderr, "Error: Unknown command '%s'. Type 'help' for available commands.\n",
                argv[0]);
        return HYPERION_CLI_EXIT_ERROR;
    }
}

int hyperionCLIRegisterCommand(const char *name, const char *description, const char *usage,
                             HyperionCommandHandler handler)
{
    if (g_commandCount >= HYPERION_CLI_MAX_COMMANDS) {
        fprintf(stderr, "Error: Maximum number of commands reached (%d).\n",
                HYPERION_CLI_MAX_COMMANDS);
        return 1; // Error
    }
    if (!name || !description || !usage || !handler) {
        fprintf(stderr, "Error: Invalid arguments for registering command '%s'.\n",
                name ? name : "(null)");
        return 1; // Error
    }
    if (findCommand(name)) {
        fprintf(stderr, "Warning: Command '%s' already registered. Overwriting.\n", name);
        // Allow overwriting for now, could return error instead
    }

    g_commands[g_commandCount].name        = name;
    g_commands[g_commandCount].description = description;
    g_commands[g_commandCount].usage       = usage;
    g_commands[g_commandCount].handler     = handler;
    g_commandCount++;

    return 0; // Success
}

int hyperionCLIPrintHelp(HyperionCLIContext *context, const char *commandName)
{
    (void)context; // Context might be used later (e.g., to show context-specific help)

    if (commandName) {
        HyperionCommand *cmd = findCommand(commandName);
        if (cmd) {
            printf("Usage: %s\n\n", cmd->usage);
            printf("%s\n", cmd->description);
        }
        else {
            fprintf(stderr, "Error: Unknown command '%s'.\n", commandName);
            return HYPERION_CLI_EXIT_ERROR;
        }
    }
    else {
        printf("Hyperion CLI v%s\n", HYPERION_VERSION);
        printf("Available commands:\n");
        for (int i = 0; i < g_commandCount; ++i) {
            printf("  %-15s %s\n", g_commands[i].name, g_commands[i].description);
        }
        printf("\nType 'help <command>' for more information on a specific command.\n");
    }
    return HYPERION_CLI_EXIT_SUCCESS;
}

/* ----------------- Built-in Command Handlers (Stubs) ----------------- */

int hyperionCommandHelp(int argc, char **argv, void *context)
{
    if (argc > 2) {
        fprintf(stderr, "Usage: help [command]\n");
        return HYPERION_CLI_EXIT_ERROR;
    }
    return hyperionCLIPrintHelp((HyperionCLIContext *)context, (argc == 2) ? argv[1] : NULL);
}

int hyperionCommandVersion(int argc, char **argv, void *context)
{
    (void)argc;
    (void)argv;
    (void)context; // Unused
    printf("Hyperion version %s\n", HYPERION_VERSION);
    return HYPERION_CLI_EXIT_SUCCESS;
}

int hyperionCommandGenerate(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;
    if (argc < 2) {
        fprintf(stderr, "Usage: generate <prompt> [max_tokens] [temperature] [sampling_method]\n");
        fprintf(stderr, "   Sampling methods: greedy, temp, topk, topp\n");
        return HYPERION_CLI_EXIT_ERROR;
    }
    if (!ctx->model || !ctx->tokenizer) {
        fprintf(stderr,
                "Error: Model and tokenizer must be loaded first (use 'model load ...').\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    // Parse arguments
    const char *prompt = argv[1];

    // Initialize generation parameters from context's defaults
    HyperionGenerationParams params = ctx->params;

    // Override with command-line arguments if provided
    if (argc > 2) {
        params.maxTokens = atoi(argv[2]);
    }

    if (argc > 3) {
        params.temperature = (float)atof(argv[3]);
    }

    if (argc > 4) {
        const char *samplingMethod = argv[4];
        if (strcmp(samplingMethod, "greedy") == 0) {
            params.samplingMethod = HYPERION_SAMPLING_GREEDY;
        }
        else if (strcmp(samplingMethod, "temp") == 0) {
            params.samplingMethod = HYPERION_SAMPLING_TEMPERATURE;
        }
        else if (strcmp(samplingMethod, "topk") == 0) {
            params.samplingMethod = HYPERION_SAMPLING_TOP_K;
        }
        else if (strcmp(samplingMethod, "topp") == 0) {
            params.samplingMethod = HYPERION_SAMPLING_TOP_P;
        }
        else {
            fprintf(stderr, "Unknown sampling method: %s\n", samplingMethod);
            return HYPERION_CLI_EXIT_ERROR;
        }
    }

    printf("Generating text for prompt: \"%s\"\n", prompt);
    printf("Parameters: max_tokens=%d, temp=%.2f, top_k=%d, top_p=%.2f, sampling=%d, style=%d
",
           params.maxTokens, params.temperature, params.topK, params.topP, params.samplingMethod, params.style);

    // Tokenize prompt
    int promptTokens[1024]; // Buffer for prompt tokens
    int promptLength = hyperionEncodeText(ctx->tokenizer, prompt, promptTokens, 1024);

    if (promptLength <= 0) {
        fprintf(stderr, "Error: Failed to tokenize prompt.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    printf("Prompt tokenized into %d tokens\n", promptLength);

    // Set prompt in generation parameters
    params.promptTokens = promptTokens;
    params.promptLength = promptLength;
    params.seed         = (uint32_t)time(NULL); // Random seed based on current time

    // Generate text
    int outputTokens[4096]; // Large buffer for generated tokens
    int outputLength;

    // Use hybrid generation if enabled, otherwise use local generation
    if (ctx->useHybrid && ctx->hybridGen) {
        printf("Using hybrid generation mode...\n");

        // Apply force flags if set
        if (ctx->forceLocal) {
            printf("Forcing local execution for this generation.\n");
            hyperionHybridGenerateForceMode(ctx->hybridGen, false);
        }
        else if (ctx->forceRemote) {
            printf("Forcing remote execution for this generation.\n");
            hyperionHybridGenerateForceMode(ctx->hybridGen, true);
        }

        // Generate text using hybrid context
        outputLength = hyperionHybridGenerateText(ctx->hybridGen, &params, outputTokens, 4096);

        // Reset force flags after generation
        ctx->forceLocal  = 0;
        ctx->forceRemote = 0;

        // Display execution mode used
        if (outputLength > 0) {
            printf("Generation used %s execution.\n",
                   hyperionHybridGenerateUsedRemote(ctx->hybridGen) ? "remote" : "local");

            // Get generation stats
            double localTime, remoteTime, tokensPerSec;
            hyperionHybridGenerateGetStats(ctx->hybridGen, &localTime, &remoteTime, &tokensPerSec);

            if (hyperionHybridGenerateUsedRemote(ctx->hybridGen)) {
                printf("Remote execution time: %.2f ms\n", remoteTime);
            }
            else {
                printf("Local execution time: %.2f ms\n", localTime);
            }
            printf("Tokens per second: %.2f\n", tokensPerSec);
        }
    }
    else {
        // Use local generation with direct model access
        outputLength = hyperionGenerateText(ctx->model, &params, outputTokens, 4096);
    }

    if (outputLength <= 0) {
        fprintf(stderr, "Error: Text generation failed.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    // Decode generated tokens to text
    char outputText[16384] = {0}; // Large buffer for decoded text
    int  textLength = hyperionDecodeTokens(ctx->tokenizer, outputTokens, outputLength, outputText,
                                         sizeof(outputText));

    if (textLength <= 0) {
        fprintf(stderr, "Error: Failed to decode generated tokens.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    // Print the generated text
    printf("\n--- Generated Text ---\n%s\n---------------------\n", outputText);

    return HYPERION_CLI_EXIT_SUCCESS;
}

int hyperionCommandTokenize(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;
    if (argc != 2) {
        fprintf(stderr, "Usage: tokenize <text>\n");
        return HYPERION_CLI_EXIT_ERROR;
    }
    if (!ctx->tokenizer) {
        fprintf(stderr, "Error: Tokenizer must be loaded first (use 'model load ...').\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    const char *text = argv[1];
    printf("Tokenizing text: \"%s\"\n", text);

    // Tokenize the input text
    int tokens[1024] = {0}; // Buffer for tokens
    int tokenCount   = hyperionEncodeText(ctx->tokenizer, text, tokens, 1024);

    if (tokenCount <= 0) {
        fprintf(stderr, "Error during tokenization.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    // Display the tokens
    printf("Tokens (%d): [", tokenCount);
    for (int i = 0; i < tokenCount; ++i) {
        // Print token ID and string representation
        const char *tokenStr = hyperionGetTokenString(ctx->tokenizer, tokens[i]);
        printf("%d=\"%s\"%s", tokens[i], tokenStr ? tokenStr : "<unknown>",
               (i == tokenCount - 1) ? "" : ", ");
    }
    printf("]\n");

    // Test decoding (should match original input, modulo special tokens handling)
    char decoded[4096] = {0};
    int  decodedLength =
        hyperionDecodeTokens(ctx->tokenizer, tokens, tokenCount, decoded, sizeof(decoded));

    if (decodedLength > 0) {
        printf("Decoded back: \"%s\"\n", decoded);
    }
    else {
        fprintf(stderr, "Warning: Could not decode tokens back to text.\n");
    }

    return HYPERION_CLI_EXIT_SUCCESS;
}

int hyperionCommandModel(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;
    if (argc < 2) {
        fprintf(stderr, "Usage: model <load <model_path> [tokenizer_path] | info | create-vocab "
                        "<corpus_file> <vocab_size> <output_path>>\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    if (strcmp(argv[1], "load") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: model load <model_path> [tokenizer_path]\n");
            return HYPERION_CLI_EXIT_ERROR;
        }
        const char *modelPath     = argv[2];
        const char *tokenizerPath = (argc > 3) ? argv[3] : NULL; // Optional tokenizer path

        // Free existing paths if any
        if (ctx->modelPath)
            free(ctx->modelPath);
        if (ctx->tokenizerPath)
            free(ctx->tokenizerPath);
        ctx->modelPath     = NULL;
        ctx->tokenizerPath = NULL;

        // Store new paths (use HYPERION_MALLOC if integrated)
        ctx->modelPath = _strdup(modelPath); // Use _strdup
        if (!ctx->modelPath)
            return HYPERION_CLI_EXIT_ERROR; // Memory error

        if (tokenizerPath) {
            ctx->tokenizerPath = _strdup(tokenizerPath); // Use _strdup
            if (!ctx->tokenizerPath) {
                free(ctx->modelPath);
                ctx->modelPath = NULL;
                return HYPERION_CLI_EXIT_ERROR;
            }
        }
        else {
            // Try to infer tokenizer path from model path (e.g., replace extension)
            // Basic example: replace .tmodel with .tok
            size_t      len = strlen(modelPath);
            const char *ext = hyperionGetFileExt(modelPath);
            if (ext && len > strlen(ext)) {
                size_t basePathLen = len - strlen(ext);
                // Need basePathLen + "tok" + null terminator
                size_t inferredPathSize = basePathLen + 4;
                char  *inferredPath     = (char *)malloc(inferredPathSize);
                if (inferredPath) {
                    // Use safer string functions
                    strncpy_s(inferredPath, inferredPathSize, modelPath, basePathLen);
                    strcpy_s(inferredPath + basePathLen, inferredPathSize - basePathLen, "tok");
                    // Check if inferred path exists?
                    if (hyperionFileExists(inferredPath) == 1) {
                        ctx->tokenizerPath = inferredPath;
                        printf("Info: Inferred tokenizer path: %s\n", ctx->tokenizerPath);
                    }
                    else {
                        printf("Warning: Could not infer tokenizer path from model path. Please "
                               "provide explicitly.\n");
                        free(inferredPath);
                    }
                }
            }
            if (!ctx->tokenizerPath) {
                fprintf(stderr, "Error: Tokenizer path not provided and could not be inferred.\n");
                free(ctx->modelPath);
                ctx->modelPath = NULL;
                return HYPERION_CLI_EXIT_ERROR;
            }
        }

        printf("Loading model from: %s\n", ctx->modelPath);
        printf("Loading tokenizer from: %s\n", ctx->tokenizerPath);

        // Load tokenizer first
        if (ctx->tokenizer) {
            hyperionDestroyTokenizer(ctx->tokenizer); // Free previous tokenizer
            ctx->tokenizer = NULL;
        }

        ctx->tokenizer = hyperionCreateTokenizer();
        if (!ctx->tokenizer) {
            fprintf(stderr, "Error: Failed to create tokenizer.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        int result = hyperionLoadVocabulary(ctx->tokenizer, ctx->tokenizerPath);
        if (result != 0) {
            fprintf(stderr, "Error: Failed to load vocabulary from %s.\n", ctx->tokenizerPath);
            hyperionDestroyTokenizer(tokenizer);
            ctx->tokenizer = NULL;
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Tokenizer loaded successfully with %d tokens.\n", ctx->tokenizer->tokenCount);

        // Try to load model
        if (ctx->model) {
            hyperionDestroyModel(ctx->model); // Free previous model
            ctx->model = NULL;
        }

        ctx->model = hyperionLoadModel(modelPath, modelPath, ctx->tokenizerPath);
        if (!ctx->model) {
            fprintf(stderr, "Warning: Could not load model from %s. Only tokenizer is available.\n",
                    modelPath);
            // Continue with just the tokenizer for now
        }
        else {
            printf("Model loaded successfully.\n");
        }
    }
    else if (strcmp(argv[1], "create-vocab") == 0) {
        // Subcommand to create a vocabulary from a text corpus
        if (argc < 5) {
            fprintf(stderr, "Usage: model create-vocab <corpus_file> <vocab_size> <output_path>\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        const char *corpusPath = argv[2];
        int         vocabSize  = atoi(argv[3]);
        const char *outputPath = argv[4];

        if (vocabSize <= 0 || vocabSize > 65536) {
            fprintf(stderr,
                    "Error: Invalid vocabulary size. Please use a value between 1 and 65536.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Creating vocabulary with up to %d tokens from %s\n", vocabSize, corpusPath);

        // Load corpus file
        FILE *corpusFile = fopen(corpusPath, "r");
        if (!corpusFile) {
            fprintf(stderr, "Error: Could not open corpus file %s\n", corpusPath);
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Determine file size
        fseek(corpusFile, 0, SEEK_END);
        long fileSize = ftell(corpusFile);
        fseek(corpusFile, 0, SEEK_SET);

        if (fileSize <= 0) {
            fprintf(stderr, "Error: Corpus file is empty or invalid.\n");
            fclose(corpusFile);
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Allocate memory for corpus
        char *corpus = (char *)malloc(fileSize + 1);
        if (!corpus) {
            fprintf(stderr, "Error: Failed to allocate memory for corpus.\n");
            fclose(corpusFile);
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Read corpus
        size_t bytesRead = fread(corpus, 1, fileSize, corpusFile);
        fclose(corpusFile);

        if (bytesRead <= 0) {
            fprintf(stderr, "Error: Failed to read corpus file.\n");
            free(corpus);
            return HYPERION_CLI_EXIT_ERROR;
        }

        corpus[bytesRead] = '\0'; // Ensure null termination

        // Create new tokenizer
        HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
        if (!tokenizer) {
            fprintf(stderr, "Error: Failed to create tokenizer.\n");
            free(corpus);
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Build vocabulary
        int result = hyperionCreateMinimalVocabulary(tokenizer, corpus, vocabSize);
        if (result != 0) {
            fprintf(stderr, "Error: Failed to create vocabulary.\n");
            hyperionDestroyTokenizer(tokenizer);
            free(corpus);
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Created vocabulary with %d tokens.\n", tokenizer->tokenCount);

        // Save vocabulary
        result = hyperionSaveVocabulary(tokenizer, outputPath);
        if (result != 0) {
            fprintf(stderr, "Error: Failed to save vocabulary to %s.\n", outputPath);
            hyperionDestroyTokenizer(tokenizer);
            free(corpus);
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Vocabulary saved to %s.\n", outputPath);

        // Cleanup
        hyperionDestroyTokenizer(tokenizer);
        free(corpus);
    }
    else if (strcmp(argv[1], "info") == 0) {
        printf("Current Model Path: %s\n", ctx->modelPath ? ctx->modelPath : "(None)");
        printf("Current Tokenizer Path: %s\n", ctx->tokenizerPath ? ctx->tokenizerPath : "(None)");
        printf("Model Loaded: %s\n", ctx->model ? "Yes" : "No");
        printf("Tokenizer Loaded: %s\n", ctx->tokenizer ? "Yes" : "No");

        if (ctx->tokenizer) {
            printf("Tokenizer Information:\n");
            printf("  Vocabulary Size: %d tokens\n", ctx->tokenizer->tokenCount);
            printf("  Special Tokens:\n");
            printf("    <unk> (ID %d): %s\n", HYPERION_TOKEN_UNKNOWN,
                   hyperionGetTokenString(ctx->tokenizer, HYPERION_TOKEN_UNKNOWN));
            printf("    <bos> (ID %d): %s\n", HYPERION_TOKEN_BOS,
                   hyperionGetTokenString(ctx->tokenizer, HYPERION_TOKEN_BOS));
            printf("    <eos> (ID %d): %s\n", HYPERION_TOKEN_EOS,
                   hyperionGetTokenString(ctx->tokenizer, HYPERION_TOKEN_EOS));
            printf("    <pad> (ID %d): %s\n", HYPERION_TOKEN_PAD,
                   hyperionGetTokenString(ctx->tokenizer, HYPERION_TOKEN_PAD));
        }

        if (ctx->model) {
            printf("Model Information:\n");
            printf("  Type: %s\n",
                   ctx->model->type == HYPERION_MODEL_TYPE_RNN
                       ? "RNN"
                       : (ctx->model->type == HYPERION_MODEL_TYPE_TRANSFORMER ? "Transformer"
                                                                            : "Unknown"));
            printf("  Hidden Size: %d\n", ctx->model->hiddenSize);
            printf("  Context Size: %d\n", ctx->model->contextSize);
            printf("  Layers: %d\n", ctx->model->layerCount);
        }
    }
    else {
        fprintf(stderr,
                "Error: Unknown model subcommand '%s'. Use 'load', 'create-vocab', or 'info'.\n",
                argv[1]);
        return HYPERION_CLI_EXIT_ERROR;
    }

    return HYPERION_CLI_EXIT_SUCCESS;
}

int hyperionCommandConfig(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;

    if (argc == 1) {
        // Print current config
        printf("Current Generation Parameters:\n");
        printf("  maxTokens    = %d\n", ctx->params.maxTokens); // Use maxTokens
        printf("  temperature  = %.2f\n", ctx->params.temperature);
        printf("  topK         = %d\n", ctx->params.topK);
        printf("  topP         = %.2f\n", ctx->params.topP);
        printf("  style        = %d\n", ctx->params.style);
        printf("Verbosity: %d\n", ctx->verbose);
        // Add other relevant config from context
    }
    else if (argc == 3) {
        // Set config parameter
        const char *param = argv[1];
        const char *value = argv[2];
        if (strcmp(param, "maxTokens") == 0) {                     // Use maxTokens
            ctx->params.maxTokens = atoi(value);                   // Use maxTokens
            printf("Set maxTokens = %d\n", ctx->params.maxTokens); // Use maxTokens
        }
        else if (strcmp(param, "temperature") == 0) {
            ctx->params.temperature = (float)atof(value);
            printf("Set temperature = %.2f\n", ctx->params.temperature);
        }
        else if (strcmp(param, "topK") == 0) {
            ctx->params.topK = atoi(value);
            printf("Set topK = %d\n", ctx->params.topK);
        }
        else if (strcmp(param, "topP") == 0) {
            ctx->params.topP = (float)atof(value);
            printf("Set topP = %.2f\n", ctx->params.topP);
        }
        else if (strcmp(param, "style") == 0) {
            if (strcmp(value, "neutral") == 0) ctx->params.style = HYPERION_STYLE_NEUTRAL;
            else if (strcmp(value, "formal") == 0) ctx->params.style = HYPERION_STYLE_FORMAL;
            else if (strcmp(value, "creative") == 0) ctx->params.style = HYPERION_STYLE_CREATIVE;
            else if (strcmp(value, "concise") == 0) ctx->params.style = HYPERION_STYLE_CONCISE;
            else if (strcmp(value, "descriptive") == 0) ctx->params.style = HYPERION_STYLE_DESCRIPTIVE;
            else {
                fprintf(stderr, "Error: Unknown style '%s'. Using neutral.\n", value);
                return HYPERION_CLI_EXIT_ERROR;
            }
            printf("Set style = %s\n", value);
        }
        else if (strcmp(param, "verbose") == 0) {
            ctx->verbose = atoi(value);
            printf("Set verbose = %d\n", ctx->verbose);
        }
        else {
            fprintf(stderr, "Error: Unknown configuration parameter '%s'.\n", param);
            return HYPERION_CLI_EXIT_ERROR;
        }
    }
    else {
        fprintf(stderr, "Usage: config [parameter] [value]\n");
        fprintf(stderr, "       config (to view current settings)\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    return HYPERION_CLI_EXIT_SUCCESS;
}

int hyperionCommandExit(int argc, char **argv, void *context)
{
    (void)argc;
    (void)argv;
    (void)context; // Unused
    printf("Exiting Hyperion shell.\n");
    return HYPERION_CLI_EXIT_QUIT; // Special code to signal shell exit
}

/**
 * MCP command handler implementation
 */
int hyperionCommandMcp(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;

    if (argc < 2) {
        fprintf(stderr, "Usage: mcp <connect <url> | disconnect | status>\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "connect") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: mcp connect <url>\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        const char *url = argv[2];

        // Already connected? Disconnect first
        if (ctx->mcpClient) {
            printf("Disconnecting from existing MCP server...\n");
            hyperionMcpDisconnect(ctx->mcpClient);
            hyperionMcpDestroyClient(ctx->mcpClient);
            ctx->mcpClient = NULL;

            // Also destroy hybrid generation context if it exists
            if (ctx->hybridGen) {
                hyperionDestroyHybridGenerate(ctx->hybridGen);
                ctx->hybridGen = NULL;
            }

            if (ctx->mcpServerUrl) {
                free(ctx->mcpServerUrl);
                ctx->mcpServerUrl = NULL;
            }
        }

        // Create MCP client config
        HyperionMcpConfig config;
        hyperionMcpGetDefaultConfig(&config);

        // Adjust config based on user preferences (could be from ctx)
        config.execPreference = HYPERION_EXEC_PREFER_LOCAL; // Default to local-first

        // Create client
        ctx->mcpClient = hyperionMcpCreateClient(&config);
        if (!ctx->mcpClient) {
            fprintf(stderr, "Error: Failed to create MCP client.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Connect to server
        printf("Connecting to MCP server at %s...\n", url);
        bool connected = hyperionMcpConnect(ctx->mcpClient, url);

        if (!connected) {
            fprintf(stderr, "Error: Failed to connect to MCP server at %s.\n", url);
            hyperionMcpDestroyClient(ctx->mcpClient);
            ctx->mcpClient = NULL;
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Store server URL
        ctx->mcpServerUrl = _strdup(url);
        if (!ctx->mcpServerUrl) {
            fprintf(stderr, "Error: Memory allocation failure for server URL.\n");
            hyperionMcpDisconnect(ctx->mcpClient);
            hyperionMcpDestroyClient(ctx->mcpClient);
            ctx->mcpClient = NULL;
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Create hybrid generation context if both model and MCP client are available
        if (ctx->model) {
            ctx->hybridGen = hyperionCreateHybridGenerate(ctx->model, ctx->mcpClient);
            if (!ctx->hybridGen) {
                fprintf(stderr, "Warning: Failed to create hybrid generation context.\n");
                // Continue without hybrid generation
            }
            else {
                ctx->useHybrid = 1;
                printf("Hybrid generation mode enabled.\n");
            }
        }

        // Get server info
        HyperionMcpServerInfo serverInfo;
        if (hyperionMcpGetServerInfo(ctx->mcpClient, &serverInfo)) {
            printf("Connected to MCP server: %s (version %s)\n", serverInfo.serverName,
                   serverInfo.serverVersion);

            // Print capabilities
            printf("Server capabilities: %s\n", serverInfo.serverCapabilities);
        }
        else {
            printf("Connected to MCP server (no server info available).\n");
        }
    }
    else if (strcmp(subcmd, "disconnect") == 0) {
        if (!ctx->mcpClient) {
            fprintf(stderr, "Error: No active MCP connection.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Disconnecting from MCP server...\n");
        hyperionMcpDisconnect(ctx->mcpClient);
        hyperionMcpDestroyClient(ctx->mcpClient);
        ctx->mcpClient = NULL;

        // Also destroy hybrid generation context if it exists
        if (ctx->hybridGen) {
            hyperionDestroyHybridGenerate(ctx->hybridGen);
            ctx->hybridGen = NULL;
        }

        if (ctx->mcpServerUrl) {
            free(ctx->mcpServerUrl);
            ctx->mcpServerUrl = NULL;
        }

        ctx->useHybrid = 0;

        printf("Disconnected from MCP server.\n");
    }
    else if (strcmp(subcmd, "status") == 0) {
        if (!ctx->mcpClient) {
            printf("MCP status: Not connected\n");
            return HYPERION_CLI_EXIT_SUCCESS;
        }

        HyperionMcpConnectionState state = hyperionMcpGetConnectionState(ctx->mcpClient);
        printf("MCP status: %s\n", state == HYPERION_MCP_CONNECTED      ? "Connected"
                                   : state == HYPERION_MCP_CONNECTING   ? "Connecting"
                                   : state == HYPERION_MCP_DISCONNECTED ? "Disconnected"
                                   : state == HYPERION_MCP_ERROR        ? "Error"
                                                                      : "Unknown");

        if (state == HYPERION_MCP_CONNECTED) {
            printf("Connected to: %s\n", ctx->mcpServerUrl ? ctx->mcpServerUrl : "(unknown)");

            // Get execution preference
            HyperionMcpExecutionPreference pref = hyperionMcpGetExecutionPreference(ctx->mcpClient);
            printf("Execution preference: %s\n", pref == HYPERION_EXEC_ALWAYS_LOCAL   ? "Always local"
                                                 : pref == HYPERION_EXEC_PREFER_LOCAL ? "Prefer local"
                                                 : pref == HYPERION_EXEC_PREFER_MCP   ? "Prefer MCP"
                                                 : pref == HYPERION_EXEC_CUSTOM_POLICY
                                                     ? "Custom policy"
                                                     : "Unknown");

            // Print whether force offline mode is enabled
            printf("Force offline mode: %s\n",
                   hyperionMcpGetForceOffline(ctx->mcpClient) ? "Yes" : "No");

            // Print hybrid status
            printf("Hybrid generation: %s\n", ctx->useHybrid ? "Enabled" : "Disabled");
            if (ctx->useHybrid && ctx->hybridGen) {
                printf("Remote generation available: %s\n",
                       hyperionHybridGenerateHasRemote(ctx->hybridGen) ? "Yes" : "No");
            }
        }
    }
    else {
        fprintf(stderr,
                "Error: Unknown MCP subcommand '%s'. Use 'connect', 'disconnect', or 'status'.\n",
                subcmd);
        return HYPERION_CLI_EXIT_ERROR;
    }

    return HYPERION_CLI_EXIT_SUCCESS;
}

/**
 * Hybrid command handler implementation
 */
int hyperionCommandHybrid(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;

    if (argc < 2) {
        fprintf(stderr, "Usage: hybrid <on | off | status | force-local | force-remote>\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "on") == 0) {
        // Check if we have both model and MCP client
        if (!ctx->model) {
            fprintf(stderr, "Error: No model loaded. Hybrid generation requires a local model.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        if (!ctx->mcpClient) {
            fprintf(stderr,
                    "Error: No MCP connection. Hybrid generation requires an MCP connection.\n");
            fprintf(stderr, "Use 'mcp connect <url>' to connect to an MCP server.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        // Create hybrid generation context if needed
        if (!ctx->hybridGen) {
            ctx->hybridGen = hyperionCreateHybridGenerate(ctx->model, ctx->mcpClient);
            if (!ctx->hybridGen) {
                fprintf(stderr, "Error: Failed to create hybrid generation context.\n");
                return HYPERION_CLI_EXIT_ERROR;
            }
        }

        ctx->useHybrid = 1;
        printf("Hybrid generation mode enabled.\n");

        // Reset force flags
        ctx->forceLocal  = 0;
        ctx->forceRemote = 0;
    }
    else if (strcmp(subcmd, "off") == 0) {
        ctx->useHybrid = 0;

        // Don't destroy the hybrid context, just disable its use
        // This allows quick toggling between modes without recreating the context

        printf("Hybrid generation mode disabled. Using local generation only.\n");
    }
    else if (strcmp(subcmd, "status") == 0) {
        printf("Hybrid generation: %s\n", ctx->useHybrid ? "Enabled" : "Disabled");

        if (ctx->hybridGen) {
            printf("Hybrid context: Available\n");
            printf("Remote generation available: %s\n",
                   hyperionHybridGenerateHasRemote(ctx->hybridGen) ? "Yes" : "No");

            // Get execution stats if available
            double localTime, remoteTime, tokensPerSec;
            hyperionHybridGenerateGetStats(ctx->hybridGen, &localTime, &remoteTime, &tokensPerSec);

            if (localTime > 0.0 || remoteTime > 0.0) {
                printf("Last generation stats:\n");
                printf("  Local time: %.2f ms\n", localTime);
                printf("  Remote time: %.2f ms\n", remoteTime);
                printf("  Tokens per second: %.2f\n", tokensPerSec);
                printf("  Last execution: %s\n",
                       hyperionHybridGenerateUsedRemote(ctx->hybridGen) ? "Remote" : "Local");
            }
        }
        else {
            printf("Hybrid context: Not available\n");
        }

        // Force flags
        printf("Force local for next generation: %s\n", ctx->forceLocal ? "Yes" : "No");
        printf("Force remote for next generation: %s\n", ctx->forceRemote ? "Yes" : "No");
    }
    else if (strcmp(subcmd, "force-local") == 0) {
        if (!ctx->useHybrid || !ctx->hybridGen) {
            fprintf(stderr, "Error: Hybrid mode is not enabled.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        ctx->forceLocal  = 1;
        ctx->forceRemote = 0;

        // Apply to hybrid context
        if (!hyperionHybridGenerateForceMode(ctx->hybridGen, false)) {
            fprintf(stderr, "Warning: Could not force local mode in hybrid context.\n");
        }

        printf("Next generation will use local execution.\n");
    }
    else if (strcmp(subcmd, "force-remote") == 0) {
        if (!ctx->useHybrid || !ctx->hybridGen) {
            fprintf(stderr, "Error: Hybrid mode is not enabled.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        if (!hyperionHybridGenerateHasRemote(ctx->hybridGen)) {
            fprintf(stderr, "Error: Remote generation is not available.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        ctx->forceLocal  = 0;
        ctx->forceRemote = 1;

        // Apply to hybrid context
        if (!hyperionHybridGenerateForceMode(ctx->hybridGen, true)) {
            fprintf(stderr, "Warning: Could not force remote mode in hybrid context.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Next generation will use remote execution.\n");
    }
    else {
        fprintf(stderr,
                "Error: Unknown hybrid subcommand '%s'. Use 'on', 'off', 'status', 'force-local', "
                "or 'force-remote'.\n",
                subcmd);
        return HYPERION_CLI_EXIT_ERROR;
    }

    return HYPERION_CLI_EXIT_SUCCESS;
}

static int load_deployment_config(const char *path, HyperionDeploymentConfig *config)
{
    if (!path) {
        fprintf(stderr, "Error: deployment configuration path is required.\n");
        return 0;
    }
    if (!hyperionDeploymentLoadConfig(config, path)) {
        fprintf(stderr, "Error: failed to load deployment config '%s'.\n", path);
        return 0;
    }
    return 1;
}

static void print_deployment_status(const HyperionDeploymentStatus *status,
                                    const HyperionDeploymentManager *manager)
{
    char timestamp[64];
    formatTimestamp(status->last_timestamp, timestamp, sizeof(timestamp));

    printf("Active deployment: %s\n",
           status->has_active ? status->active_config.version : "none");
    if (status->has_active) {
        printf("  Environment: %s\n", status->active_config.environment);
        printf("  Cluster: %s\n", status->active_config.cluster);
        printf("  Replicas: %d\n", status->active_config.desired_replicas);
    }
    printf("Last state: %s @ %s\n", hyperionDeploymentStateName(status->last_state), timestamp);
    printf("Success rate: %.0f%%\n", status->success_rate * 100.0);
    printf("Rollbacks: %zu\n", status->rollback_count);

    HyperionDeploymentHistoryEntry history[6];
    size_t copied = hyperionDeploymentCopyHistory(manager, history, 6);
    if (copied == 0) {
        printf("No deployment history recorded.\n");
        return;
    }

    printf("Recent history:\n");
    for (size_t i = 0; i < copied; ++i) {
        char entry_time[64];
        formatTimestamp(history[i].timestamp, entry_time, sizeof(entry_time));
        printf("  [%s] %s %s (env=%s, replicas=%d)\n",
               entry_time,
               hyperionDeploymentStateName(history[i].state),
               history[i].config.version,
               history[i].config.environment,
               history[i].config.desired_replicas);
        if (history[i].notes[0]) {
            printf("    notes: %s\n", history[i].notes);
        }
    }
}

static int parse_monitor_comparison(const char *value, int *comparison_out)
{
    if (!value || !comparison_out) return 0;
    if (strcmp(value, "gt") == 0 || strcmp(value, ">") == 0) {
        *comparison_out = HYPERION_MONITOR_COMPARE_GREATER;
        return 1;
    }
    if (strcmp(value, "lt") == 0 || strcmp(value, "<") == 0) {
        *comparison_out = HYPERION_MONITOR_COMPARE_LESS;
        return 1;
    }
    if (strcmp(value, "eq") == 0 || strcmp(value, "=") == 0) {
        *comparison_out = HYPERION_MONITOR_COMPARE_EQUAL;
        return 1;
    }
    return 0;
}

static const char *autoscale_decision_name(HyperionAutoScaleDecision decision)
{
    switch (decision) {
        case HYPERION_AUTOSCALE_DECISION_SCALE_UP: return "scale-up";
        case HYPERION_AUTOSCALE_DECISION_SCALE_DOWN: return "scale-down";
        default: return "no-op";
    }
}

static void cli_monitor_alert_callback(const char *metric_name, double current_value, void *user_data)
{
    (void)user_data;
    HyperionMonitoringCenter *center = hyperionMonitoringInstance();
    if (center) {
        char message[160];
        snprintf(message, sizeof(message), "Alert triggered for %s (value %.3f)",
                 metric_name, current_value);
        hyperionMonitoringRecordLog(center, "WARN", message);
    }
    printf("ALERT: %s threshold reached (%.3f)\n", metric_name, current_value);
}

int hyperionCommandDeploy(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;
    if (!ctx || !ctx->deploymentManager) {
        fprintf(stderr, "Deployment manager not initialized.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    HyperionMonitoringCenter *monitor = ctx->monitoringCenter ? ctx->monitoringCenter
                                                               : hyperionMonitoringInstance();

    if (argc < 2) {
        fprintf(stderr, "Usage: deploy plan|apply|rollback|status [...args]\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "plan") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: deploy plan <config_file>\n");
            return HYPERION_CLI_EXIT_ERROR;
        }
        HyperionDeploymentConfig config;
        if (!load_deployment_config(argv[2], &config)) {
            if (monitor) {
                hyperionMonitoringRecordLog(monitor, "WARN", "Deployment plan load failed");
            }
            return HYPERION_CLI_EXIT_ERROR;
        }
        char plan[1024];
        hyperionDeploymentGeneratePlan(&config, plan, sizeof(plan));
        size_t plan_len = strlen(plan);
        printf("%s", plan);
        if (plan_len == 0 || plan[plan_len - 1] != '\n') {
            printf("\n");
        }
        if (monitor) {
            char message[160];
            snprintf(message, sizeof(message), "Generated deployment plan for %s:%s",
                     config.environment, config.version);
            hyperionMonitoringRecordLog(monitor, "INFO", message);
            hyperionMonitoringIncrementCounter(monitor, "deploy.plans_generated", "count",
                                               "Generated deployment plans", 1.0);
        }
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(subcmd, "apply") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: deploy apply <config_file> [--notes <text>]\n");
            return HYPERION_CLI_EXIT_ERROR;
        }

        HyperionDeploymentConfig config;
        if (!load_deployment_config(argv[2], &config)) {
            if (monitor) {
                hyperionMonitoringRecordLog(monitor, "WARN", "Deployment apply config load failed");
            }
            return HYPERION_CLI_EXIT_ERROR;
        }

        char notes[160] = {0};
        for (int i = 3; i < argc; ++i) {
            if (strcmp(argv[i], "--notes") == 0 && i + 1 < argc) {
                strncpy(notes, argv[i + 1], sizeof(notes) - 1);
                break;
            }
        }

        char error[128];
        if (!hyperionDeploymentApply(ctx->deploymentManager, &config,
                                     notes[0] ? notes : NULL, error, sizeof(error))) {
            fprintf(stderr, "Deployment failed: %s\n", error[0] ? error : "validation error");
            if (monitor) {
                char message[160];
                snprintf(message, sizeof(message), "Deployment %s failed: %s",
                         config.version, error[0] ? error : "validation error");
                hyperionMonitoringRecordLog(monitor, "ERROR", message);
                hyperionMonitoringIncrementCounter(monitor, "deploy.apply_failed", "count",
                                                   "Failed deployment attempts", 1.0);
            }
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Deployment %s applied to %s (%d replicas)\n",
               config.version, config.cluster, config.desired_replicas);
        if (monitor) {
            char message[160];
            snprintf(message, sizeof(message), "Deployment %s applied to %s",
                     config.version, config.cluster);
            hyperionMonitoringRecordLog(monitor, "INFO", message);
            hyperionMonitoringIncrementCounter(monitor, "deploy.apply_success", "count",
                                               "Successful deployments", 1.0);
            hyperionMonitoringSetGauge(monitor, "deploy.active_replicas", "count",
                                       "Active deployment replicas",
                                       (double)config.desired_replicas);
        }
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(subcmd, "rollback") == 0) {
        const char *version = (argc >= 3) ? argv[2] : NULL;
        char message[160];
        if (!hyperionDeploymentRollback(ctx->deploymentManager, version, message, sizeof(message))) {
            fprintf(stderr, "Rollback failed: %s\n", message[0] ? message : "unknown error");
            if (monitor) {
                char logbuf[160];
                snprintf(logbuf, sizeof(logbuf), "Rollback request failed (%s)",
                         message[0] ? message : "unknown error");
                hyperionMonitoringRecordLog(monitor, "WARN", logbuf);
                hyperionMonitoringIncrementCounter(monitor, "deploy.rollback_failed", "count",
                                                   "Failed rollbacks", 1.0);
            }
            return HYPERION_CLI_EXIT_ERROR;
        }
        printf("%s\n", message);
        if (monitor) {
            hyperionMonitoringRecordLog(monitor, "INFO", message);
            hyperionMonitoringIncrementCounter(monitor, "deploy.rollback_success", "count",
                                               "Successful rollbacks", 1.0);
        }
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(subcmd, "status") == 0) {
        HyperionDeploymentStatus status;
        if (!hyperionDeploymentGetStatus(ctx->deploymentManager, &status)) {
            fprintf(stderr, "Unable to compute deployment status.\n");
            return HYPERION_CLI_EXIT_ERROR;
        }
        print_deployment_status(&status, ctx->deploymentManager);
        if (monitor && status.has_active) {
            hyperionMonitoringSetGauge(monitor, "deploy.active_replicas", "count",
                                       "Active deployment replicas",
                                       (double)status.active_config.desired_replicas);
        }
        if (ctx->autoScaler && status.has_active) {
            HyperionAutoScaleDecision decision;
            size_t desired_replicas = (size_t)status.active_config.desired_replicas;
            double metric_value = 0.0;
            char reason[160];
            if (hyperionAutoScalerPlan(ctx->autoScaler,
                                       (size_t)status.active_config.desired_replicas,
                                       &decision,
                                       &desired_replicas,
                                       &metric_value,
                                       reason,
                                       sizeof(reason))) {
                printf("Autoscale metric '%s': %.2f\n",
                       hyperionAutoScalerPolicy(ctx->autoScaler)->metric_name,
                       metric_value);
                if (decision != HYPERION_AUTOSCALE_DECISION_NONE) {
                    printf("Autoscaler recommends %s to %zu replicas (%s)\n",
                           autoscale_decision_name(decision),
                           desired_replicas,
                           reason);
                } else {
                    printf("Autoscaler: %s\n", reason);
                }
            } else if (reason[0]) {
                printf("Autoscaler unavailable: %s\n", reason);
            }
        }
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    fprintf(stderr, "Error: Unknown deploy subcommand '%s'.\n", subcmd);
    return HYPERION_CLI_EXIT_ERROR;
}

int hyperionCommandMonitor(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;
    HyperionMonitoringCenter *center = ctx && ctx->monitoringCenter
                                           ? ctx->monitoringCenter
                                           : hyperionMonitoringInstance();
    if (!center) {
        fprintf(stderr, "Monitoring center unavailable.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: monitor status | logs [--limit N] | reset | alert add <metric> <gt|lt|eq> <threshold> [hits] | alert list\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    const char *subcmd = argv[1];

    if (strcmp(subcmd, "status") == 0 || strcmp(subcmd, "metrics") == 0) {
        char buffer[2048];
        hyperionMonitoringExport(center, buffer, sizeof(buffer));
        printf("%s\n", buffer);
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(subcmd, "logs") == 0) {
        size_t limit = 20;
        for (int i = 2; i < argc; ++i) {
            if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
                limit = (size_t)strtoul(argv[i + 1], NULL, 10);
            }
        }
        char buffer[2048];
        hyperionMonitoringExportLogs(center, buffer, sizeof(buffer), limit);
        printf("%s\n", buffer);
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(subcmd, "reset") == 0) {
        hyperionMonitoringReset(center);
        hyperionMonitoringRecordLog(center, "INFO", "Monitoring data reset via CLI");
        printf("Monitoring state reset.\n");
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(subcmd, "alert") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: monitor alert add <metric> <gt|lt|eq> <threshold> [hits] | monitor alert list\n");
            return HYPERION_CLI_EXIT_ERROR;
        }
        const char *alert_cmd = argv[2];
        if (strcmp(alert_cmd, "list") == 0) {
            char buffer[2048];
            hyperionMonitoringExport(center, buffer, sizeof(buffer));
            printf("%s\n", buffer);
            return HYPERION_CLI_EXIT_SUCCESS;
        }
        if (strcmp(alert_cmd, "add") == 0) {
            if (argc < 6) {
                fprintf(stderr, "Usage: monitor alert add <metric> <gt|lt|eq> <threshold> [hits]\n");
                return HYPERION_CLI_EXIT_ERROR;
            }
            const char *metric = argv[3];
            const char *comparison_str = argv[4];
            double threshold = atof(argv[5]);
            size_t hits = 1;
            if (argc >= 7) {
                hits = (size_t)strtoul(argv[6], NULL, 10);
                if (hits == 0) hits = 1;
            }
            int comparison = 0;
            if (!parse_monitor_comparison(comparison_str, &comparison)) {
                fprintf(stderr, "Unknown comparison operator '%s'. Use gt, lt, or eq.\n", comparison_str);
                return HYPERION_CLI_EXIT_ERROR;
            }
            if (!hyperionMonitoringAddAlert(center, metric, "CLI configured alert",
                                            threshold, comparison, hits,
                                            cli_monitor_alert_callback, NULL)) {
                fprintf(stderr, "Failed to register alert.\n");
                return HYPERION_CLI_EXIT_ERROR;
            }
            hyperionMonitoringRecordLog(center, "INFO", "CLI alert registered");
            printf("Alert registered for metric '%s'.\n", metric);
            return HYPERION_CLI_EXIT_SUCCESS;
        }
        fprintf(stderr, "Unknown alert subcommand '%s'.\n", alert_cmd);
        return HYPERION_CLI_EXIT_ERROR;
    }

    fprintf(stderr, "Unknown monitor subcommand '%s'.\n", subcmd);
    return HYPERION_CLI_EXIT_ERROR;
}

static size_t autoscale_resolve_replicas(HyperionCLIContext *ctx, const HyperionAutoScalerPolicy *policy,
                                         int argc, char **argv)
{
    size_t replicas = policy ? policy->min_replicas : 1;
    if (ctx && ctx->deploymentManager) {
        HyperionDeploymentStatus status;
        if (hyperionDeploymentGetStatus(ctx->deploymentManager, &status) && status.has_active) {
            replicas = (size_t)status.active_config.desired_replicas;
        }
    }
    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--replicas") == 0 && i + 1 < argc) {
            replicas = (size_t)strtoul(argv[i + 1], NULL, 10);
        }
    }
    return replicas;
}

int hyperionCommandAutoscale(int argc, char **argv, void *context)
{
    HyperionCLIContext *ctx = (HyperionCLIContext *)context;
    if (!ctx || !ctx->autoScaler) {
        fprintf(stderr, "Autoscaler is not enabled. Set autoscale.enabled=1 in configuration.\n");
        return HYPERION_CLI_EXIT_ERROR;
    }

    const HyperionAutoScalerPolicy *policy = hyperionAutoScalerPolicy(ctx->autoScaler);
    if (argc < 2 || strcmp(argv[1], "policy") == 0) {
        printf("Autoscale policy:\n");
        printf("  metric: %s\n", policy->metric_name[0] ? policy->metric_name : "<none>");
        printf("  scale-up threshold: %.2f\n", policy->scale_up_threshold);
        printf("  scale-down threshold: %.2f\n", policy->scale_down_threshold);
        printf("  scale step: %zu\n", policy->scale_step);
        printf("  min replicas: %zu\n", policy->min_replicas);
        printf("  max replicas: %zu\n", policy->max_replicas);
        printf("  cooldown (up/down): %d / %d seconds\n",
               policy->scale_up_cooldown_seconds,
               policy->scale_down_cooldown_seconds);
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    if (strcmp(argv[1], "plan") == 0) {
        size_t replicas = autoscale_resolve_replicas(ctx, policy, argc, argv);
        HyperionAutoScaleDecision decision;
        size_t desired = replicas;
        double metric = 0.0;
        char reason[160];
        if (!hyperionAutoScalerPlan(ctx->autoScaler, replicas, &decision, &desired, &metric, reason, sizeof(reason))) {
            fprintf(stderr, "%s\n", reason[0] ? reason : "Autoscale plan unavailable");
            return HYPERION_CLI_EXIT_ERROR;
        }

        printf("Metric '%s' current value: %.2f\n", policy->metric_name, metric);
        printf("Current replicas: %zu\n", replicas);
        printf("Decision: %s\n", autoscale_decision_name(decision));
        if (decision != HYPERION_AUTOSCALE_DECISION_NONE) {
            printf("Recommended replicas: %zu\n", desired);
            printf("Reason: %s\n", reason);
        } else {
            printf("No scaling change recommended (%s).\n", reason);
        }
        return HYPERION_CLI_EXIT_SUCCESS;
    }

    fprintf(stderr, "Unknown autoscale subcommand '%s'.\n", argv[1]);
    return HYPERION_CLI_EXIT_ERROR;
}