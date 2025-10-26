/**
 * Hyperion Command Line Interface Header
 *
 * This header defines the command-line interface for Hyperion.
 */

#ifndef HYPERION_CLI_H
#define HYPERION_CLI_H

#include "../core/mcp/mcp_client.h"
#include "../models/text/generate.h"
#include "../models/text/hybrid_generate.h"
#include "../utils/deployment_manager.h"
#include "../utils/monitoring_center.h"
#include "../utils/auto_scaler.h"
#include <stdint.h>

/* ----------------- Constants ----------------- */

/* Maximum command length */
#define HYPERION_CLI_MAX_COMMAND_LENGTH 1024

/* Maximum number of arguments */
#define HYPERION_CLI_MAX_ARGS 64

/* Maximum number of commands */
#define HYPERION_CLI_MAX_COMMANDS 32

/* Command exit codes */
#define HYPERION_CLI_EXIT_SUCCESS 0
#define HYPERION_CLI_EXIT_ERROR 1
#define HYPERION_CLI_EXIT_QUIT 2

/* ----------------- Types ----------------- */

/**
 * Command handler function type
 */
typedef int (*HyperionCommandHandler)(int argc, char **argv, void *context);

/**
 * Command structure
 */
typedef struct {
    const char          *name;        /* Command name */
    const char          *description; /* Command description */
    const char          *usage;       /* Command usage */
    HyperionCommandHandler handler;     /* Command handler */
} HyperionCommand;

/**
 * CLI context structure
 */
typedef struct {
    /* Model and tokenizer */
    HyperionModel     *model;         /* Current model */
    HyperionTokenizer *tokenizer;     /* Current tokenizer */
    char            *modelPath;     /* Current model path */
    char            *tokenizerPath; /* Current tokenizer path */

    /* MCP and hybrid generation */
    HyperionMcpClient      *mcpClient;    /* MCP client (if connected) */
    HyperionHybridGenerate *hybridGen;    /* Hybrid generation context */
    char                 *mcpServerUrl; /* MCP server URL */
    int                   useHybrid;    /* Whether to use hybrid generation */
    int                   forceRemote;  /* Force remote execution for next generation */
    int                   forceLocal;   /* Force local execution for next generation */

    /* CLI state */
    int                    interactive; /* Whether in interactive mode */
    int                    verbose;     /* Verbosity level */
    HyperionGenerationParams params;      /* Current generation parameters */
    HyperionGenerationStyle style;       /* Current generation style */
    HyperionDeploymentManager *deploymentManager;
    HyperionMonitoringCenter *monitoringCenter;
    HyperionAutoScaler *autoScaler;
} HyperionCLIContext;

/* ----------------- API Functions ----------------- */

/**
 * Initialize the CLI
 *
 * @param context CLI context
 * @return 0 on success, non-zero on error
 */
int hyperionCLIInit(HyperionCLIContext *context);

/**
 * Clean up the CLI
 *
 * @param context CLI context
 */
void hyperionCLICleanup(HyperionCLIContext *context);

/**
 * Parse command-line arguments
 *
 * @param context CLI context
 * @param argc Argument count
 * @param argv Argument array
 * @return 0 on success, non-zero on error
 */
int hyperionCLIParseArgs(HyperionCLIContext *context, int argc, char **argv);

/**
 * Run the CLI
 *
 * @param context CLI context
 * @param argc Argument count
 * @param argv Argument array
 * @return Exit code
 */
int hyperionCLIRun(HyperionCLIContext *context, int argc, char **argv);

/**
 * Run the interactive shell
 *
 * @param context CLI context
 * @return Exit code
 */
int hyperionCLIRunShell(HyperionCLIContext *context);

/**
 * Process a single command
 *
 * @param context CLI context
 * @param command Command line
 * @return Exit code
 */
int hyperionCLIProcessCommand(HyperionCLIContext *context, const char *command);

/**
 * Register a command
 *
 * @param name Command name
 * @param description Command description
 * @param usage Command usage
 * @param handler Command handler
 * @return 0 on success, non-zero on error
 */
int hyperionCLIRegisterCommand(const char *name, const char *description, const char *usage,
                             HyperionCommandHandler handler);

/**
 * Print help information
 *
 * @param context CLI context
 * @param command Command name (NULL for general help)
 * @return 0 on success, non-zero on error
 */
int hyperionCLIPrintHelp(HyperionCLIContext *context, const char *command);

/* ----------------- Built-in Command Handlers ----------------- */

/**
 * Help command handler
 */
int hyperionCommandHelp(int argc, char **argv, void *context);

/**
 * Version command handler
 */
int hyperionCommandVersion(int argc, char **argv, void *context);

/**
 * Generate command handler
 */
int hyperionCommandGenerate(int argc, char **argv, void *context);

/**
 * Tokenize command handler
 */
int hyperionCommandTokenize(int argc, char **argv, void *context);

/**
 * Model command handler
 */
int hyperionCommandModel(int argc, char **argv, void *context);

/**
 * Config command handler
 */
int hyperionCommandConfig(int argc, char **argv, void *context);

/**
 * Exit command handler
 */
int hyperionCommandExit(int argc, char **argv, void *context);

/**
 * MCP command handler for Model Context Protocol operations
 */
int hyperionCommandMcp(int argc, char **argv, void *context);

/**
 * Hybrid command handler for controlling hybrid local/remote execution
 */
int hyperionCommandHybrid(int argc, char **argv, void *context);

int hyperionCommandDeploy(int argc, char **argv, void *context);

int hyperionCommandMonitor(int argc, char **argv, void *context);

int hyperionCommandAutoscale(int argc, char **argv, void *context);

#endif /* HYPERION_CLI_H */