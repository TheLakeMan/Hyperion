/**
 * Hyperion Configuration Implementation
 * 
 * This file implements the configuration management system for Hyperion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"
#include "memory.h"
#include "io.h"

#ifdef _WIN32
#define HYPERION_ENV_PREFIX "HYPERION_"
#else
#define HYPERION_ENV_PREFIX "HYPERION_"
#endif

/* ----------------- Configuration Storage ----------------- */

#define MAX_CONFIG_ENTRIES 256
#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 1024

typedef struct {
    char key[MAX_KEY_LENGTH];
    HyperionConfigValue value;
    int active;  /* 1 if set, 0 if deleted or inactive */
} ConfigEntry;

static ConfigEntry configEntries[MAX_CONFIG_ENTRIES];
static int configEntryCount = 0;
static int configInitialized = 0;

/* ----------------- Helper Functions ----------------- */

/**
 * Find a configuration entry by key
 */
static int findConfigEntry(const char *key) {
    for (int i = 0; i < configEntryCount; i++) {
        if (configEntries[i].active && strcmp(configEntries[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Create a new configuration entry
 */
static int createConfigEntry(const char *key, HyperionConfigType type) {
    /* Check if key already exists */
    int index = findConfigEntry(key);
    if (index >= 0) {
        /* Key exists, update type */
        configEntries[index].value.type = type;
        
        /* Free string value if needed */
        if (type != HYPERION_CONFIG_STRING && 
            configEntries[index].value.type == HYPERION_CONFIG_STRING &&
            configEntries[index].value.value.stringValue) {
            free(configEntries[index].value.value.stringValue);
            configEntries[index].value.value.stringValue = NULL;
        }
        
        return index;
    }
    
    /* Check if we have space */
    if (configEntryCount >= MAX_CONFIG_ENTRIES) {
        /* Look for an inactive entry */
        for (int i = 0; i < configEntryCount; i++) {
            if (!configEntries[i].active) {
                /* Found an inactive entry, reuse it */
                strncpy(configEntries[i].key, key, MAX_KEY_LENGTH - 1);
                configEntries[i].key[MAX_KEY_LENGTH - 1] = '\0';
                configEntries[i].value.type = type;
                configEntries[i].active = 1;
                
                /* Clean up possible string value */
                if (configEntries[i].value.type == HYPERION_CONFIG_STRING &&
                    configEntries[i].value.value.stringValue) {
                    free(configEntries[i].value.value.stringValue);
                    configEntries[i].value.value.stringValue = NULL;
                }
                
                return i;
            }
        }
        
        /* No space available */
        return -1;
    }
    
    /* Create a new entry */
    index = configEntryCount++;
    strncpy(configEntries[index].key, key, MAX_KEY_LENGTH - 1);
    configEntries[index].key[MAX_KEY_LENGTH - 1] = '\0';
    configEntries[index].value.type = type;
    configEntries[index].active = 1;
    
    /* Initialize string value to NULL */
    if (type == HYPERION_CONFIG_STRING) {
        configEntries[index].value.value.stringValue = NULL;
    }
    
    return index;
}

/**
 * Convert a config key to environment variable name
 * Example: "model.path" -> "HYPERION_MODEL_PATH"
 */
static void keyToEnvVar(const char *key, char *envVar, size_t envVarSize) {
    snprintf(envVar, envVarSize, "%s", HYPERION_ENV_PREFIX);
    size_t prefixLen = strlen(HYPERION_ENV_PREFIX);
    
    for (size_t i = 0; i < strlen(key) && (prefixLen + i) < (envVarSize - 1); i++) {
        if (key[i] == '.') {
            envVar[prefixLen + i] = '_';
        } else {
            envVar[prefixLen + i] = toupper(key[i]);
        }
    }
    envVar[prefixLen + strlen(key)] = '\0';
}

/**
 * Get value from environment variable
 */
static const char* getFromEnvironment(const char *key) {
    char envVar[128];
    keyToEnvVar(key, envVar, sizeof(envVar));
    return getenv(envVar);
}

/**
 * Trim whitespace from string
 */
static char* trimWhitespace(char *str) {
    if (!str) return NULL;
    
    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) return str;  /* All spaces */
    
    /* Trim trailing space */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    /* Write new null terminator */
    *(end + 1) = 0;
    
    return str;
}

/**
 * Parse a configuration line
 * 
 * Format: key = value
 */
static int parseConfigLine(char *line) {
    if (!line) return 0;
    
    /* Skip comments and empty lines */
    line = trimWhitespace(line);
    if (!line[0] || line[0] == '#' || line[0] == ';') {
        return 0;
    }
    
    /* Find the equal sign */
    char *equalSign = strchr(line, '=');
    if (!equalSign) {
        /* No equal sign, return an error */
        return -1;
    }
    
    /* Split the line at the equal sign */
    *equalSign = '\0';
    char *key = trimWhitespace(line);
    char *value = trimWhitespace(equalSign + 1);
    
    /* Check for empty key or value */
    if (!key[0] || !value[0]) {
        return -1;
    }
    
    /* Determine the value type */
    if ((value[0] == '"' && value[strlen(value) - 1] == '"') ||
        (value[0] == '\'' && value[strlen(value) - 1] == '\'')) {
        /* String value */
        value[strlen(value) - 1] = '\0';  /* Remove trailing quote */
        value++;  /* Skip leading quote */

        hyperionConfigSetString(key, value);
    } else if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        /* Boolean value */
        hyperionConfigSetBool(key, strcmp(value, "true") == 0);
    } else {
        /* Try to parse as a number */
        char *endPtr;
        float floatValue = strtof(value, &endPtr);

        if (*endPtr == '\0') {
            /* Valid number */
            if (floatValue == (int)floatValue) {
                /* Integer */
                hyperionConfigSetInt(key, (int)floatValue);
            } else {
                /* Float */
                hyperionConfigSetFloat(key, floatValue);
            }
        } else {
            /* Try to parse as a style name */
            if (strcmp(value, "neutral") == 0) {
                hyperionConfigSetStyle(key, HYPERION_STYLE_NEUTRAL);
            } else if (strcmp(value, "formal") == 0) {
                hyperionConfigSetStyle(key, HYPERION_STYLE_FORMAL);
            } else if (strcmp(value, "creative") == 0) {
                hyperionConfigSetStyle(key, HYPERION_STYLE_CREATIVE);
            } else if (strcmp(value, "concise") == 0) {
                hyperionConfigSetStyle(key, HYPERION_STYLE_CONCISE);
            } else if (strcmp(value, "descriptive") == 0) {
                hyperionConfigSetStyle(key, HYPERION_STYLE_DESCRIPTIVE);
            } else {
                /* Treat as string */
                hyperionConfigSetString(key, value);
            }
        }
    }
    
    return 0;
}

/* ----------------- Public API ----------------- */

/**
 * Initialize the configuration system
 */
int hyperionConfigInit() {
    if (configInitialized) {
        return 0;  /* Already initialized */
    }
    
    /* Clear the config entries */
    memset(configEntries, 0, sizeof(configEntries));
    configEntryCount = 0;
    configInitialized = 1;
    
    return 0;
}

/**
 * Clean up the configuration system
 */
void hyperionConfigCleanup() {
    /* Clean up string values */
    for (int i = 0; i < configEntryCount; i++) {
        if (configEntries[i].active && 
            configEntries[i].value.type == HYPERION_CONFIG_STRING &&
            configEntries[i].value.value.stringValue) {
            free(configEntries[i].value.value.stringValue);
            configEntries[i].value.value.stringValue = NULL;
        }
    }
    
    /* Reset state */
    memset(configEntries, 0, sizeof(configEntries));
    configEntryCount = 0;
    configInitialized = 0;
}

/**
 * Load configuration from a file
 */
int hyperionConfigLoad(const char *path) {
    HyperionFile *file = hyperionOpenFile(path, HYPERION_FILE_READ);
    if (!file) {
        return -1;
    }
    
    char line[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 10];
    int lineNum = 0;
    int errors = 0;
    
    while (hyperionReadLine(file, line, sizeof(line)) > 0) {
        lineNum++;
        
        if (parseConfigLine(line) < 0) {
            /* Error parsing line */
            fprintf(stderr, "Error parsing config line %d: %s\n", lineNum, line);
            errors++;
        }
    }
    
    hyperionCloseFile(file);
    
    return errors ? -1 : 0;
}

/**
 * Save configuration to a file
 */
int hyperionConfigSave(const char *path) {
    HyperionFile *file = hyperionOpenFile(path, HYPERION_FILE_WRITE | HYPERION_FILE_CREATE);
    if (!file) {
        return -1;
    }
    
    /* Write a header */
    char header[256];
    sprintf(header, "# Hyperion Configuration File\n# Generated automatically\n\n");
    hyperionWriteFile(file, header, strlen(header));
    
    /* Write each entry */
    for (int i = 0; i < configEntryCount; i++) {
        if (!configEntries[i].active) {
            continue;
        }
        
        char line[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 10];
        
        switch (configEntries[i].value.type) {
            case HYPERION_CONFIG_INTEGER:
                sprintf(line, "%s = %d\n", configEntries[i].key, 
                        configEntries[i].value.value.intValue);
                break;
            
            case HYPERION_CONFIG_FLOAT:
                sprintf(line, "%s = %f\n", configEntries[i].key, 
                        configEntries[i].value.value.floatValue);
                break;
            
            case HYPERION_CONFIG_STRING:
                sprintf(line, "%s = \"%s\"\n", configEntries[i].key, 
                        configEntries[i].value.value.stringValue ? 
                        configEntries[i].value.value.stringValue : "");
                break;
            
            case HYPERION_CONFIG_BOOLEAN:
                sprintf(line, "%s = %s\n", configEntries[i].key, 
                        configEntries[i].value.value.boolValue ? "true" : "false");
                break;
            case HYPERION_CONFIG_STYLE:
                {
                    const char *styleStr;
                    switch (configEntries[i].value.value.styleValue) {
                        case HYPERION_STYLE_NEUTRAL: styleStr = "neutral"; break;
                        case HYPERION_STYLE_FORMAL: styleStr = "formal"; break;
                        case HYPERION_STYLE_CREATIVE: styleStr = "creative"; break;
                        case HYPERION_STYLE_CONCISE: styleStr = "concise"; break;
                        case HYPERION_STYLE_DESCRIPTIVE: styleStr = "descriptive"; break;
                        default: styleStr = "neutral"; break;
                    }
                    sprintf(line, "%s = %s\n", configEntries[i].key, styleStr);
                }
                break;
        }
        
        hyperionWriteFile(file, line, strlen(line));
    }
    
    hyperionCloseFile(file);
    
    return 0;
}

/**
 * Set a configuration integer value
 */
int hyperionConfigSetInt(const char *key, int value) {
    if (!configInitialized) {
        if (hyperionConfigInit() != 0) {
            return -1;
        }
    }
    
    int index = createConfigEntry(key, HYPERION_CONFIG_INTEGER);
    if (index < 0) {
        return -1;
    }
    
    configEntries[index].value.value.intValue = value;
    
    return 0;
}

/**
 * Get a configuration integer value
 */
int hyperionConfigGetInt(const char *key, int defaultValue) {
    // Check environment variable first (highest priority)
    const char *envValue = getFromEnvironment(key);
    if (envValue) {
        return atoi(envValue);
    }
    
    // Check config file value
    if (!configInitialized) {
        return defaultValue;
    }
    
    int index = findConfigEntry(key);
    if (index < 0) {
        return defaultValue;
    }
    
    switch (configEntries[index].value.type) {
        case HYPERION_CONFIG_INTEGER:
            return configEntries[index].value.value.intValue;
        
        case HYPERION_CONFIG_FLOAT:
            return (int)configEntries[index].value.value.floatValue;
        
        case HYPERION_CONFIG_BOOLEAN:
            return configEntries[index].value.value.boolValue ? 1 : 0;
        
        case HYPERION_CONFIG_STRING:
            if (configEntries[index].value.value.stringValue) {
                return atoi(configEntries[index].value.value.stringValue);
            }
            break;
        case HYPERION_CONFIG_STYLE:
            return (int)configEntries[index].value.value.styleValue;
    }
    
    return defaultValue;
}

/**
 * Set a configuration float value
 */
int hyperionConfigSetFloat(const char *key, float value) {
    if (!configInitialized) {
        if (hyperionConfigInit() != 0) {
            return -1;
        }
    }
    
    int index = createConfigEntry(key, HYPERION_CONFIG_FLOAT);
    if (index < 0) {
        return -1;
    }
    
    configEntries[index].value.value.floatValue = value;
    
    return 0;
}

/**
 * Get a configuration float value
 */
float hyperionConfigGetFloat(const char *key, float defaultValue) {
    // Check environment variable first
    const char *envValue = getFromEnvironment(key);
    if (envValue) {
        return (float)atof(envValue);
    }
    
    // Check config file value
    if (!configInitialized) {
        return defaultValue;
    }
    
    int index = findConfigEntry(key);
    if (index < 0) {
        return defaultValue;
    }
    
    switch (configEntries[index].value.type) {
        case HYPERION_CONFIG_INTEGER:
            return (float)configEntries[index].value.value.intValue;
        
        case HYPERION_CONFIG_FLOAT:
            return configEntries[index].value.value.floatValue;
        
        case HYPERION_CONFIG_BOOLEAN:
            return configEntries[index].value.value.boolValue ? 1.0f : 0.0f;
        
        case HYPERION_CONFIG_STRING:
            if (configEntries[index].value.value.stringValue) {
                return (float)atof(configEntries[index].value.value.stringValue);
            }
            break;
        case HYPERION_CONFIG_STYLE:
            return (float)configEntries[index].value.value.styleValue;
    }
    
    return defaultValue;
}

/**
 * Set a configuration string value
 */
int hyperionConfigSetString(const char *key, const char *value) {
    if (!configInitialized) {
        if (hyperionConfigInit() != 0) {
            return -1;
        }
    }
    
    int index = createConfigEntry(key, HYPERION_CONFIG_STRING);
    if (index < 0) {
        return -1;
    }
    
    /* Free existing value if any */
    if (configEntries[index].value.value.stringValue) {
        free(configEntries[index].value.value.stringValue);
    }
    
    /* Copy the new value */
    if (value) {
        configEntries[index].value.value.stringValue = _strdup(value);
        if (!configEntries[index].value.value.stringValue) {
            return -1;
        }
    } else {
        configEntries[index].value.value.stringValue = NULL;
    }
    
    return 0;
}

/**
 * Get a configuration value (convenience function)
 */
const char* hyperionConfigGet(const char *key, const char *defaultValue) {
    return hyperionConfigGetString(key, defaultValue);
}
    // Check environment variable first
    const char *envValue = getFromEnvironment(key);
    if (envValue) {
        return envValue;
    }
    
    // Check config file value
    if (!configInitialized) {
        return defaultValue;
    }
    
    int index = findConfigEntry(key);
    if (index < 0) {
        return defaultValue;
    }
    
    static char buffer[MAX_VALUE_LENGTH];
    
    switch (configEntries[index].value.type) {
        case HYPERION_CONFIG_INTEGER:
            sprintf(buffer, "%d", configEntries[index].value.value.intValue);
            return buffer;
        
        case HYPERION_CONFIG_FLOAT:
            sprintf(buffer, "%f", configEntries[index].value.value.floatValue);
            return buffer;
        
        case HYPERION_CONFIG_BOOLEAN:
            return configEntries[index].value.value.boolValue ? "true" : "false";
        
        case HYPERION_CONFIG_STRING:
            return configEntries[index].value.value.stringValue ? 
                   configEntries[index].value.value.stringValue : defaultValue;
        case HYPERION_CONFIG_STYLE:
            switch (configEntries[index].value.value.styleValue) {
                case HYPERION_STYLE_NEUTRAL: return "neutral";
                case HYPERION_STYLE_FORMAL: return "formal";
                case HYPERION_STYLE_CREATIVE: return "creative";
                case HYPERION_STYLE_CONCISE: return "concise";
                case HYPERION_STYLE_DESCRIPTIVE: return "descriptive";
                default: return "neutral";
            }
    }
    
    return defaultValue;
}

/**
 * Set a configuration boolean value
 */
int hyperionConfigSetBool(const char *key, int value) {
    if (!configInitialized) {
        if (hyperionConfigInit() != 0) {
            return -1;
        }
    }
    
    int index = createConfigEntry(key, HYPERION_CONFIG_BOOLEAN);
    if (index < 0) {
        return -1;
    }
    
    configEntries[index].value.value.boolValue = value ? 1 : 0;
    
    return 0;
}

/**
 * Get a configuration boolean value
 */
int hyperionConfigGetBool(const char *key, int defaultValue) {
    if (!configInitialized) {
        return defaultValue;
    }
    
    int index = findConfigEntry(key);
    if (index < 0) {
        return defaultValue;
    }
    
    switch (configEntries[index].value.type) {
        case HYPERION_CONFIG_INTEGER:
            return configEntries[index].value.value.intValue != 0;
        
        case HYPERION_CONFIG_FLOAT:
            return configEntries[index].value.value.floatValue != 0.0f;
        
        case HYPERION_CONFIG_BOOLEAN:
            return configEntries[index].value.value.boolValue;
        
        case HYPERION_CONFIG_STRING:
            if (configEntries[index].value.value.stringValue) {
                return strcmp(configEntries[index].value.value.stringValue, "true") == 0 ||
                       strcmp(configEntries[index].value.value.stringValue, "1") == 0 ||
                       strcmp(configEntries[index].value.value.stringValue, "yes") == 0 ||
                       strcmp(configEntries[index].value.value.stringValue, "y") == 0 ||
                       strcmp(configEntries[index].value.value.stringValue, "on") == 0;
            }
            break;
    }
    
    return defaultValue;
}

/**
 * Set a configuration generation style value
 */
int hyperionConfigSetStyle(const char *key, HyperionGenerationStyle value) {
    if (!configInitialized) {
        if (hyperionConfigInit() != 0) {
            return -1;
        }
    }
    
    int index = createConfigEntry(key, HYPERION_CONFIG_STYLE);
    if (index < 0) {
        return -1;
    }
    
    configEntries[index].value.value.styleValue = value;
    
    return 0;
}

/**
 * Get a configuration generation style value
 */
HyperionGenerationStyle hyperionConfigGetStyle(const char *key, HyperionGenerationStyle defaultValue) {
    if (!configInitialized) {
        return defaultValue;
    }
    
    int index = findConfigEntry(key);
    if (index < 0) {
        return defaultValue;
    }
    
    switch (configEntries[index].value.type) {
        case HYPERION_CONFIG_STYLE:
            return configEntries[index].value.value.styleValue;
        
        case HYPERION_CONFIG_STRING:
            if (configEntries[index].value.value.stringValue) {
                const char *s = configEntries[index].value.value.stringValue;
                if (strcmp(s, "neutral") == 0) return HYPERION_STYLE_NEUTRAL;
                if (strcmp(s, "formal") == 0) return HYPERION_STYLE_FORMAL;
                if (strcmp(s, "creative") == 0) return HYPERION_STYLE_CREATIVE;
                if (strcmp(s, "concise") == 0) return HYPERION_STYLE_CONCISE;
                if (strcmp(s, "descriptive") == 0) return HYPERION_STYLE_DESCRIPTIVE;
            }
            break;
    }
    
    return defaultValue;
}

/**
 * Check if a configuration key exists
 */
int hyperionConfigHasKey(const char *key) {
    if (!configInitialized) {
        return 0;
    }
    
    return findConfigEntry(key) >= 0;
}

/**
 * Remove a configuration key
 */
int hyperionConfigRemoveKey(const char *key) {
    if (!configInitialized) {
        return 0;
    }
    
    int index = findConfigEntry(key);
    if (index < 0) {
        return 0;
    }
    
    /* Free string value if needed */
    if (configEntries[index].value.type == HYPERION_CONFIG_STRING &&
        configEntries[index].value.value.stringValue) {
        free(configEntries[index].value.value.stringValue);
        configEntries[index].value.value.stringValue = NULL;
    }
    
    configEntries[index].active = 0;
    
    return 1;
}

/**
 * Get all configuration keys
 */
int hyperionConfigGetKeys(char **keys, int maxKeys) {
    if (!configInitialized || !keys || maxKeys <= 0) {
        return 0;
    }
    
    int count = 0;
    
    for (int i = 0; i < configEntryCount && count < maxKeys; i++) {
        if (configEntries[i].active) {
            keys[count++] = configEntries[i].key;
        }
    }
    
    return count;
}

/**
 * Set default configuration values
 */
int hyperionConfigSetDefaults() {
    if (!configInitialized) {
        if (hyperionConfigInit() != 0) {
            return -1;
        }
    }
    
    /* System settings */
    hyperionConfigSetString("system.name", "Hyperion");
    hyperionConfigSetString("system.version", "0.1.0");
    hyperionConfigSetString("system.data_dir", "./data");
    hyperionConfigSetString("system.model_dir", "./models");
    
    /* Memory settings */
    hyperionConfigSetInt("memory.pool_size", 1024 * 1024);  /* 1MB */
    hyperionConfigSetInt("memory.max_allocations", 10000);
    hyperionConfigSetBool("memory.track_leaks", 1);
    
    /* Model settings */
    hyperionConfigSetInt("model.context_size", 512);
    hyperionConfigSetInt("model.hidden_size", 256);
    hyperionConfigSetFloat("model.temperature", 0.7f);
    hyperionConfigSetInt("model.top_k", 40);
    hyperionConfigSetFloat("model.top_p", 0.9f);
    
    /* Text generation settings */
    hyperionConfigSetInt("generate.max_tokens", 100);
    hyperionConfigSetBool("generate.add_bos", 1);
    hyperionConfigSetInt("generate.style", HYPERION_STYLE_NEUTRAL);
    
    /* Tokenizer settings */
    hyperionConfigSetInt("tokenizer.vocab_size", 8192);
    hyperionConfigSetBool("tokenizer.case_sensitive", 0);
    
    return 0;
}

/**
 * Override a configuration value from command line
 */
int hyperionConfigOverride(const char *key, const char *value) {
    if (!key || !value) {
        return -1;
    }
    
    /* Parse the value based on type */
    if ((value[0] == '"' && value[strlen(value) - 1] == '"') ||
        (value[0] == '\'' && value[strlen(value) - 1] == '\'')) {
        /* String value */
        char *stringValue = _strdup(value + 1);
        stringValue[strlen(stringValue) - 1] = '\0';
        
        int result = hyperionConfigSetString(key, stringValue);
        free(stringValue);
        return result;
    } else if (strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
        /* Boolean value */
        return hyperionConfigSetBool(key, strcmp(value, "true") == 0);
    } else if (strcmp(key, "generate.style") == 0) {
        /* Style value */
        if (strcmp(value, "neutral") == 0) return hyperionConfigSetStyle(key, HYPERION_STYLE_NEUTRAL);
        if (strcmp(value, "formal") == 0) return hyperionConfigSetStyle(key, HYPERION_STYLE_FORMAL);
        if (strcmp(value, "creative") == 0) return hyperionConfigSetStyle(key, HYPERION_STYLE_CREATIVE);
        if (strcmp(value, "concise") == 0) return hyperionConfigSetStyle(key, HYPERION_STYLE_CONCISE);
        if (strcmp(value, "descriptive") == 0) return hyperionConfigSetStyle(key, HYPERION_STYLE_DESCRIPTIVE);
        return -1; /* Invalid style */
    } else {
        /* Try to parse as a number */
        char *endPtr;
        float floatValue = strtof(value, &endPtr);
        
        if (*endPtr == '\0') {
            /* Valid number */
            if (floatValue == (int)floatValue) {
                /* Integer */
                return hyperionConfigSetInt(key, (int)floatValue);
            } else {
                /* Float */
                return hyperionConfigSetFloat(key, floatValue);
            }
        } else {
            /* Treat as string */
            return hyperionConfigSetString(key, value);
        }
    }
}

/**
 * Apply command line overrides
 */
int hyperionConfigApplyCommandLine(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        /* Look for --key=value or -key=value format */
        if ((argv[i][0] == '-' && argv[i][1] == '-') || 
            (argv[i][0] == '-')) {
            /* Get the key (skip leading dashes) */
            char *key = argv[i] + (argv[i][1] == '-' ? 2 : 1);
            
            /* Find the equal sign */
            char *equalSign = strchr(key, '=');
            if (equalSign) {
                /* Split the string at the equal sign */
                *equalSign = '\0';
                char *value = equalSign + 1;
                
                /* Override the configuration */
                hyperionConfigOverride(key, value);
                
                /* Restore the equal sign for display purposes */
                *equalSign = '=';
            } else if (i + 1 < argc && argv[i + 1][0] != '-') {
                /* Format: --key value or -key value */
                hyperionConfigOverride(key, argv[i + 1]);
                i++;  /* Skip the value */
            }
        }
    }
    
    return 0;
}

/**
 * Get a configuration value with proper hierarchy (environment > config > default)
 * This is a convenience function equivalent to hyperionConfigGetString
 */
const char* hyperionConfigGet(const char *key, const char *defaultValue) {
    return hyperionConfigGetString(key, defaultValue);
}