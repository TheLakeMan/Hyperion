/**
 * Hyperion WebSocket Implementation
 * 
 * Lightweight WebSocket support for real-time streaming responses.
 * Implements the WebSocket protocol (RFC 6455) with minimal dependencies.
 */

#ifndef HYPERION_WEBSOCKET_H
#define HYPERION_WEBSOCKET_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>
#else
typedef int SOCKET;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// WebSocket frame opcodes
typedef enum {
    WS_OPCODE_CONTINUATION = 0x0,
    WS_OPCODE_TEXT = 0x1,
    WS_OPCODE_BINARY = 0x2,
    WS_OPCODE_CLOSE = 0x8,
    WS_OPCODE_PING = 0x9,
    WS_OPCODE_PONG = 0xa
} WebSocketOpcode;

// WebSocket frame structure
typedef struct {
    bool fin;                   // Final fragment flag
    WebSocketOpcode opcode;     // Opcode
    bool masked;                // Masking flag
    uint64_t payload_length;    // Payload length
    uint8_t mask[4];            // Masking key (if masked)
    uint8_t* payload;           // Payload data
} WebSocketFrame;

// WebSocket connection state
typedef enum {
    WS_STATE_CONNECTING,
    WS_STATE_OPEN,
    WS_STATE_CLOSING,
    WS_STATE_CLOSED
} WebSocketState;

// WebSocket connection
typedef struct {
    SOCKET socket;
    WebSocketState state;
    char* buffer;
    size_t buffer_size;
    size_t buffer_used;
    bool is_server;
} WebSocketConnection;

/**
 * Check if a request is a WebSocket upgrade request
 * 
 * @param request HTTP request string
 * @return true if it's a WebSocket upgrade request
 */
bool hyperionIsWebSocketUpgrade(const char* request);

/**
 * Handle WebSocket handshake
 * 
 * @param socket Client socket
 * @param request HTTP request string
 * @return true if handshake successful
 */
bool hyperionWebSocketHandshake(SOCKET socket, const char* request);

/**
 * Create a WebSocket connection
 * 
 * @param socket Connected socket
 * @param is_server Whether this is a server-side connection
 * @return WebSocket connection or NULL on failure
 */
WebSocketConnection* hyperionWebSocketCreate(SOCKET socket, bool is_server);

/**
 * Destroy a WebSocket connection
 * 
 * @param ws WebSocket connection
 */
void hyperionWebSocketDestroy(WebSocketConnection* ws);

/**
 * Send a WebSocket frame
 * 
 * @param ws WebSocket connection
 * @param opcode Frame opcode
 * @param data Payload data
 * @param length Payload length
 * @return Number of bytes sent or -1 on error
 */
int hyperionWebSocketSend(WebSocketConnection* ws, WebSocketOpcode opcode, const void* data, size_t length);

/**
 * Send a text message
 * 
 * @param ws WebSocket connection
 * @param message Text message
 * @return Number of bytes sent or -1 on error
 */
int hyperionWebSocketSendText(WebSocketConnection* ws, const char* message);

/**
 * Send a binary message
 * 
 * @param ws WebSocket connection
 * @param data Binary data
 * @param length Data length
 * @return Number of bytes sent or -1 on error
 */
int hyperionWebSocketSendBinary(WebSocketConnection* ws, const void* data, size_t length);

/**
 * Send a ping frame
 * 
 * @param ws WebSocket connection
 * @param data Optional ping data
 * @param length Ping data length
 * @return Number of bytes sent or -1 on error
 */
int hyperionWebSocketPing(WebSocketConnection* ws, const void* data, size_t length);

/**
 * Send a pong frame (response to ping)
 * 
 * @param ws WebSocket connection
 * @param data Pong data (usually echo of ping data)
 * @param length Pong data length
 * @return Number of bytes sent or -1 on error
 */
int hyperionWebSocketPong(WebSocketConnection* ws, const void* data, size_t length);

/**
 * Receive a WebSocket frame
 * 
 * @param ws WebSocket connection
 * @param frame Output frame structure
 * @return 1 if frame received, 0 if no frame available, -1 on error
 */
int hyperionWebSocketReceive(WebSocketConnection* ws, WebSocketFrame* frame);

/**
 * Close a WebSocket connection
 * 
 * @param ws WebSocket connection
 * @param code Close code (1000 = normal closure)
 * @param reason Close reason (optional)
 * @return true on success
 */
bool hyperionWebSocketClose(WebSocketConnection* ws, uint16_t code, const char* reason);

/**
 * Check if WebSocket connection is open
 * 
 * @param ws WebSocket connection
 * @return true if connection is open
 */
bool hyperionWebSocketIsOpen(WebSocketConnection* ws);

/**
 * Get WebSocket connection state
 * 
 * @param ws WebSocket connection
 * @return Connection state
 */
WebSocketState hyperionWebSocketGetState(WebSocketConnection* ws);

// Streaming generation callback type
typedef void (*StreamingCallback)(const char* token, bool is_final, void* user_data);

/**
 * Generate text with streaming via WebSocket
 * 
 * @param ws WebSocket connection
 * @param prompt Input prompt
 * @param max_tokens Maximum tokens to generate
 * @param temperature Sampling temperature
 * @param callback Streaming callback function
 * @param user_data User data for callback
 * @return Number of tokens generated or -1 on error
 */
int hyperionWebSocketStreamGenerate(WebSocketConnection* ws, const char* prompt, int max_tokens, 
                                   float temperature, StreamingCallback callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_WEBSOCKET_H */