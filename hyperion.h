/**
 * @file hyperion.h
 * @brief Main header file for the Hyperion AI Framework
 *
 * This is the primary include file for applications using Hyperion.
 * It provides a unified interface to all core functionality.
 */

#ifndef HYPERION_H
#define HYPERION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Core includes */
#include "core/memory.h"
#include "core/config.h"

/* Model includes */
#include "models/text/generate.h"
#include "models/text/tokenizer.h"

/* Utility includes */
#include "utils/quantize.h"
#include "utils/tensor.h"

/* Version information */
#define HYPERION_VERSION_MAJOR 1
#define HYPERION_VERSION_MINOR 0
#define HYPERION_VERSION_PATCH 0
#define HYPERION_VERSION_STRING "1.0.0"

/**
 * Initialize the Hyperion framework
 * @param configPath Path to configuration file (NULL for defaults)
 * @return 0 on success, negative on error
 */
int hyperionInit(const char *configPath);

/**
 * Cleanup and shutdown the Hyperion framework
 */
void hyperionCleanup(void);

/**
 * Get version string
 * @return Version string
 */
const char *hyperionGetVersion(void);

/**
 * Check if framework is initialized
 * @return true if initialized, false otherwise
 */
bool hyperionIsInitialized(void);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_H */