/**
 * Hyperion CLI Auto-Completion System
 * 
 * Provides auto-completion for commands, parameters, and file paths
 * to improve developer experience and reduce command-line errors.
 */

#ifndef HYPERION_CLI_COMPLETION_H
#define HYPERION_CLI_COMPLETION_H

#include "cli.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of completion suggestions */
#define HYPERION_CLI_MAX_COMPLETIONS 50
#define HYPERION_CLI_MAX_COMPLETION_LENGTH 256

/* Completion types */
typedef enum {
    HYPERION_COMPLETION_COMMAND = 0,     /* Command names */
    HYPERION_COMPLETION_PARAMETER,       /* Command parameters */
    HYPERION_COMPLETION_FILE,            /* File paths */
    HYPERION_COMPLETION_DIRECTORY,       /* Directory paths */
    HYPERION_COMPLETION_MODEL,           /* Model files */
    HYPERION_COMPLETION_CONFIG_KEY,      /* Configuration keys */
    HYPERION_COMPLETION_CONFIG_VALUE,    /* Configuration values */
    HYPERION_COMPLETION_MCP_URL,         /* MCP server URLs */
    HYPERION_COMPLETION_STYLE,           /* Generation styles */
    HYPERION_COMPLETION_SAMPLING        /* Sampling methods */
} HyperionCompletionType;

/* Completion result structure */
typedef struct {
    char text[HYPERION_CLI_MAX_COMPLETION_LENGTH];  /* Completion text */
    HyperionCompletionType type;                    /* Completion type */
    const char* description;                        /* Optional description */
    int is_directory;                               /* For file completions */
} HyperionCompletion;

/* Completion context */
typedef struct {
    const char* line;                               /* Current input line */
    int cursor_pos;                                 /* Cursor position */
    int word_start;                                 /* Start of current word */
    int word_end;                                   /* End of current word */
    char current_word[HYPERION_CLI_MAX_COMPLETION_LENGTH];  /* Current word being completed */
    char** tokens;                                  /* Parsed tokens */
    int token_count;                                /* Number of tokens */
    int current_token;                              /* Index of token being completed */
} HyperionCompletionContext;

/* Completion functions */

/**
 * Initialize the CLI completion system
 * 
 * @param cli_context CLI context
 * @return 0 on success, non-zero on error
 */
int hyperion_cli_completion_init(HyperionCLIContext* cli_context);

/**
 * Cleanup the CLI completion system
 */
void hyperion_cli_completion_cleanup(void);

/**
 * Get completions for the current input
 * 
 * @param line Current input line
 * @param cursor_pos Cursor position in line
 * @param completions Array to store completions
 * @param max_completions Maximum number of completions
 * @return Number of completions found
 */
int hyperion_cli_get_completions(const char* line, int cursor_pos,
                                HyperionCompletion* completions, int max_completions);

/**
 * Get command completions
 * 
 * @param prefix Command prefix to match
 * @param completions Array to store completions
 * @param max_completions Maximum number of completions
 * @return Number of completions found
 */
int hyperion_cli_complete_commands(const char* prefix, HyperionCompletion* completions, 
                                  int max_completions);

/**
 * Get parameter completions for a specific command
 * 
 * @param command Command name
 * @param prefix Parameter prefix to match
 * @param completions Array to store completions
 * @param max_completions Maximum number of completions
 * @return Number of completions found
 */
int hyperion_cli_complete_parameters(const char* command, const char* prefix,
                                    HyperionCompletion* completions, int max_completions);

/**
 * Get file path completions
 * 
 * @param prefix Path prefix to match
 * @param filter File filter (e.g., "*.bin", "*.json")
 * @param completions Array to store completions
 * @param max_completions Maximum number of completions
 * @return Number of completions found
 */
int hyperion_cli_complete_files(const char* prefix, const char* filter,
                               HyperionCompletion* completions, int max_completions);

/**
 * Get configuration key completions
 * 
 * @param prefix Key prefix to match
 * @param completions Array to store completions
 * @param max_completions Maximum number of completions
 * @return Number of completions found
 */
int hyperion_cli_complete_config_keys(const char* prefix, HyperionCompletion* completions,
                                     int max_completions);

/**
 * Get configuration value completions for a specific key
 * 
 * @param key Configuration key
 * @param prefix Value prefix to match
 * @param completions Array to store completions
 * @param max_completions Maximum number of completions
 * @return Number of completions found
 */
int hyperion_cli_complete_config_values(const char* key, const char* prefix,
                                       HyperionCompletion* completions, int max_completions);

/**
 * Print completions to the console
 * 
 * @param completions Array of completions
 * @param count Number of completions
 * @param show_descriptions Whether to show descriptions
 */
void hyperion_cli_print_completions(const HyperionCompletion* completions, int count,
                                   int show_descriptions);

/**
 * Find common prefix of completions
 * 
 * @param completions Array of completions
 * @param count Number of completions
 * @param common_prefix Buffer to store common prefix
 * @param max_length Maximum length of common prefix
 * @return Length of common prefix
 */
int hyperion_cli_find_common_prefix(const HyperionCompletion* completions, int count,
                                   char* common_prefix, int max_length);

/**
 * Interactive completion handler (for readline-style interfaces)
 * 
 * @param text Text to complete
 * @param start Start position
 * @param end End position
 * @return Array of completion strings (NULL-terminated)
 */
char** hyperion_cli_completion_handler(const char* text, int start, int end);

/* Interactive Configuration Wizard */

/**
 * Run interactive configuration wizard
 * 
 * @param cli_context CLI context
 * @return 0 on success, non-zero on error
 */
int hyperion_cli_run_config_wizard(HyperionCLIContext* cli_context);

/**
 * Configuration wizard step structure
 */
typedef struct {
    const char* name;                               /* Setting name */
    const char* description;                        /* Human-readable description */
    const char* current_value;                      /* Current value */
    const char* default_value;                      /* Default value */
    const char** allowed_values;                    /* NULL-terminated array of allowed values */
    const char* validation_pattern;                 /* Regex pattern for validation */
    int required;                                   /* Whether this setting is required */
    const char* help_text;                          /* Additional help text */
} HyperionConfigStep;

/**
 * Get configuration steps for wizard
 * 
 * @param steps Array to store configuration steps
 * @param max_steps Maximum number of steps
 * @return Number of configuration steps
 */
int hyperion_cli_get_config_steps(HyperionConfigStep* steps, int max_steps);

/**
 * Process a configuration step in the wizard
 * 
 * @param step Configuration step
 * @param cli_context CLI context
 * @return 0 on success, non-zero on error
 */
int hyperion_cli_process_config_step(const HyperionConfigStep* step, 
                                    HyperionCLIContext* cli_context);

/**
 * Validate configuration value
 * 
 * @param step Configuration step
 * @param value Value to validate
 * @return 1 if valid, 0 if invalid
 */
int hyperion_cli_validate_config_value(const HyperionConfigStep* step, const char* value);

/**
 * Show configuration help
 * 
 * @param step Configuration step
 */
void hyperion_cli_show_config_help(const HyperionConfigStep* step);

/* Helper functions for enhanced CLI experience */

/**
 * Enable auto-completion in interactive shell
 * 
 * @param cli_context CLI context
 * @return 0 on success, non-zero on error
 */
int hyperion_cli_enable_completion(HyperionCLIContext* cli_context);

/**
 * Show command suggestions for partial input
 * 
 * @param partial_command Partial command input
 * @param cli_context CLI context
 */
void hyperion_cli_show_suggestions(const char* partial_command, 
                                  HyperionCLIContext* cli_context);

/**
 * Enhanced command prompt with auto-completion
 * 
 * @param prompt Prompt string
 * @param buffer Buffer to store input
 * @param buffer_size Buffer size
 * @param cli_context CLI context
 * @return Length of input, -1 on error
 */
int hyperion_cli_enhanced_prompt(const char* prompt, char* buffer, int buffer_size,
                                HyperionCLIContext* cli_context);

/**
 * Format completion for display
 * 
 * @param completion Completion to format
 * @param buffer Buffer to store formatted text
 * @param buffer_size Buffer size
 * @return Length of formatted text
 */
int hyperion_cli_format_completion(const HyperionCompletion* completion, 
                                  char* buffer, int buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_CLI_COMPLETION_H */