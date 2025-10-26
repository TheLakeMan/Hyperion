/**
 * Hyperion Configuration Header
 * 
 * This header defines the configuration system for Hyperion, allowing
 * for flexible runtime configuration of the framework.
 */

#ifndef HYPERION_CONFIG_H
#define HYPERION_CONFIG_H

/* ----------------- Configuration Value Types ----------------- */

#include "../models/text/generate.h" // For HyperionGenerationStyle

/**
 * Configuration value type enumeration
 */
typedef enum {
    HYPERION_CONFIG_INTEGER,    /* Integer value */
    HYPERION_CONFIG_FLOAT,      /* Float value */
    HYPERION_CONFIG_STRING,     /* String value */
    HYPERION_CONFIG_BOOLEAN,    /* Boolean value */
    HYPERION_CONFIG_STYLE       /* Generation style value */
} HyperionConfigType;

/**
 * Configuration value union
 */
typedef struct {
    HyperionConfigType type;    /* Value type */
    union {
        int intValue;         /* Integer value */
        float floatValue;     /* Float value */
        char *stringValue;    /* String value */
        int boolValue;        /* Boolean value */
        HyperionGenerationStyle styleValue; /* Generation style value */
    } value;
} HyperionConfigValue;

/* ----------------- Configuration API ----------------- */

/**
 * Initialize the configuration system
 * 
 * @return 0 on success, non-zero on error
 */
int hyperionConfigInit();

/**
 * Clean up the configuration system
 */
void hyperionConfigCleanup();

/**
 * Load configuration from a file
 * 
 * @param path File path
 * @return 0 on success, non-zero on error
 */
int hyperionConfigLoad(const char *path);

/**
 * Save configuration to a file
 * 
 * @param path File path
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSave(const char *path);

/**
 * Set an integer configuration value
 * 
 * @param key Configuration key
 * @param value Integer value
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSetInt(const char *key, int value);

/**
 * Get an integer configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value if key not found
 * @return Configuration value or default value
 */
int hyperionConfigGetInt(const char *key, int defaultValue);

/**
 * Set a float configuration value
 * 
 * @param key Configuration key
 * @param value Float value
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSetFloat(const char *key, float value);

/**
 * Get a float configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value if key not found
 * @return Configuration value or default value
 */
float hyperionConfigGetFloat(const char *key, float defaultValue);

/**
 * Set a string configuration value
 * 
 * @param key Configuration key
 * @param value String value
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSetString(const char *key, const char *value);

/**
 * Get a string configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value if key not found
 * @return Configuration value or default value
 */
const char* hyperionConfigGetString(const char *key, const char *defaultValue);

/**
 * Set a boolean configuration value
 * 
 * @param key Configuration key
 * @param value Boolean value (0 = false, non-zero = true)
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSetBool(const char *key, int value);

/**
 * Get a boolean configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value if key not found
 * @return Configuration value or default value
 */
int hyperionConfigGetBool(const char *key, int defaultValue);

/**
 * Check if a configuration key exists
 * 
 * @param key Configuration key
 * @return 1 if key exists, 0 if not
 */
int hyperionConfigHasKey(const char *key);

/**
 * Remove a configuration key
 * 
 * @param key Configuration key
 * @return 1 if key was removed, 0 if key not found
 */
int hyperionConfigRemoveKey(const char *key);

/**
 * Get all configuration keys
 * 
 * @param keys Array to store keys
 * @param maxKeys Maximum number of keys to store
 * @return Number of keys stored
 */
int hyperionConfigGetKeys(char **keys, int maxKeys);

/**
 * Set default configuration values
 * 
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSetDefaults();

/**
 * Override a configuration value from command line
 * 
 * @param key Configuration key
 * @param value Value string
 * @return 0 on success, non-zero on error
 */
int hyperionConfigOverride(const char *key, const char *value);

/**
 * Apply command line overrides
 * 
 * @param argc Argument count
 * @param argv Argument array
 * @return 0 on success, non-zero on error
 */
int hyperionConfigApplyCommandLine(int argc, char **argv);

/**
 * Set a style configuration value
 * 
 * @param key Configuration key
 * @param value Style value
 * @return 0 on success, non-zero on error
 */
int hyperionConfigSetStyle(const char *key, HyperionGenerationStyle value);

/**
 * Get a style configuration value
 * 
 * @param key Configuration key
 * @param defaultValue Default value if key not found
 * @return Configuration value or default value
 */
HyperionGenerationStyle hyperionConfigGetStyle(const char *key, HyperionGenerationStyle defaultValue);

/**
 * Get a configuration value with hierarchy (environment > config > default)
 * This is a convenience function equivalent to hyperionConfigGetString
 * 
 * @param key Configuration key
 * @param defaultValue Default value if key not found
 * @return Configuration value or default value
 */
const char* hyperionConfigGet(const char *key, const char *defaultValue);

#endif /* HYPERION_CONFIG_H */
