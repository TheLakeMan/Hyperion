/**
 * @file mcp_client.h
 * @brief Model Context Protocol (MCP) client implementation for Hyperion
 *
 * This file defines the API for interacting with MCP servers, providing
 * hybrid local/remote execution capabilities for Hyperion.
 */

#ifndef HYPERION_MCP_CLIENT_H
#define HYPERION_MCP_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Execution preference mode for hybrid operations
 */
typedef enum {
    HYPERION_EXEC_ALWAYS_LOCAL, /**< Always use local execution only */
    HYPERION_EXEC_PREFER_LOCAL, /**< Prefer local, use MCP only when necessary */
    HYPERION_EXEC_PREFER_MCP,   /**< Prefer MCP when available, fallback to local */
    HYPERION_EXEC_CUSTOM_POLICY /**< Use custom policy defined per operation */
} HyperionMcpExecutionPreference;

/**
 * @brief MCP Server connection state
 */
typedef enum {
    HYPERION_MCP_DISCONNECTED, /**< Not connected to MCP server */
    HYPERION_MCP_CONNECTING,   /**< Connection in progress */
    HYPERION_MCP_CONNECTED,    /**< Successfully connected to MCP server */
    HYPERION_MCP_ERROR         /**< Error connecting to MCP server */
} HyperionMcpConnectionState;

/**
 * @brief MCP client configuration
 */
typedef struct {
    HyperionMcpExecutionPreference execPreference;      /**< Execution preference */
    bool                         enableAutoDiscovery; /**< Automatically discover MCP servers */
    bool                         enableTelemetry;     /**< Allow sending telemetry data */
    int                          connectionTimeoutMs; /**< Connection timeout in milliseconds */
    int                          maxRetryAttempts;    /**< Maximum connection retry attempts */
    bool                         forceOffline;        /**< Force offline mode */
} HyperionMcpConfig;

/**
 * @brief MCP server information
 */
typedef struct {
    char                     serverName[64];           /**< Server name */
    char                     serverUrl[256];           /**< Server URL */
    char                     serverVersion[32];        /**< Server version */
    HyperionMcpConnectionState connectionState;          /**< Connection state */
    char                     serverCapabilities[1024]; /**< JSON string of capabilities */
} HyperionMcpServerInfo;

/**
 * @brief MCP client context
 */
typedef struct HyperionMcpClient HyperionMcpClient;

/**
 * @brief Create an MCP client instance
 *
 * @param config Client configuration
 * @return HyperionMcpClient* Client instance or NULL on failure
 */
HyperionMcpClient *hyperionMcpCreateClient(const HyperionMcpConfig *config);

/**
 * @brief Destroy an MCP client instance
 *
 * @param client Client instance
 */
void hyperionMcpDestroyClient(HyperionMcpClient *client);

/**
 * @brief Connect to an MCP server
 *
 * @param client Client instance
 * @param serverUrl Server URL
 * @return true if connection was successful or in progress
 * @return false if connection failed
 */
bool hyperionMcpConnect(HyperionMcpClient *client, const char *serverUrl);

/**
 * @brief Disconnect from an MCP server
 *
 * @param client Client instance
 */
void hyperionMcpDisconnect(HyperionMcpClient *client);

/**
 * @brief Get connection state
 *
 * @param client Client instance
 * @return HyperionMcpConnectionState Current connection state
 */
HyperionMcpConnectionState hyperionMcpGetConnectionState(HyperionMcpClient *client);

/**
 * @brief Check if MCP capabilities are available
 *
 * @param client Client instance
 * @return true if MCP is available and connected
 * @return false if MCP is unavailable
 */
bool hyperionMcpIsAvailable(HyperionMcpClient *client);

/**
 * @brief Get server information
 *
 * @param client Client instance
 * @param info Output server information
 * @return true if information was successfully retrieved
 * @return false on failure
 */
bool hyperionMcpGetServerInfo(HyperionMcpClient *client, HyperionMcpServerInfo *info);

/**
 * @brief Check if MCP server supports a specific capability
 *
 * @param client Client instance
 * @param capability Capability name
 * @return true if capability is supported
 * @return false if capability is not supported or client is not connected
 */
bool hyperionMcpHasCapability(HyperionMcpClient *client, const char *capability);

/**
 * @brief Call a remote MCP tool
 *
 * @param client Client instance
 * @param toolName Name of the tool to call
 * @param arguments JSON arguments for the tool
 * @param result Output buffer for result
 * @param resultSize Size of output buffer
 * @return int Number of bytes written to result buffer, or negative value on error
 */
int hyperionMcpCallTool(HyperionMcpClient *client, const char *toolName, const char *arguments,
                      char *result, int resultSize);

/**
 * @brief Access an MCP resource
 *
 * @param client Client instance
 * @param resourceUri URI of the resource to access
 * @param result Output buffer for resource content
 * @param resultSize Size of output buffer
 * @return int Number of bytes written to result buffer, or negative value on error
 */
int hyperionMcpAccessResource(HyperionMcpClient *client, const char *resourceUri, char *result,
                            int resultSize);

/**
 * @brief Set execution preference
 *
 * @param client Client instance
 * @param preference Execution preference
 */
void hyperionMcpSetExecutionPreference(HyperionMcpClient             *client,
                                     HyperionMcpExecutionPreference preference);

/**
 * @brief Get execution preference
 *
 * @param client Client instance
 * @return HyperionMcpExecutionPreference Current execution preference
 */
HyperionMcpExecutionPreference hyperionMcpGetExecutionPreference(HyperionMcpClient *client);

/**
 * @brief Force offline mode (disable MCP capabilities)
 *
 * @param client Client instance
 * @param forceOffline Whether to force offline mode
 */
void hyperionMcpSetForceOffline(HyperionMcpClient *client, bool forceOffline);

/**
 * @brief Check if forced offline mode is enabled
 *
 * @param client Client instance
 * @return true if forced offline mode is enabled
 * @return false if forced offline mode is disabled
 */
bool hyperionMcpGetForceOffline(HyperionMcpClient *client);

/**
 * @brief Get default MCP client configuration
 *
 * @param config Output configuration structure
 */
void hyperionMcpGetDefaultConfig(HyperionMcpConfig *config);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_MCP_CLIENT_H */