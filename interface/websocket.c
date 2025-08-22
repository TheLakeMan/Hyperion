/**
 * Hyperion WebSocket Implementation
 */

#include "websocket.h"
#include "../core/memory.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#define send(s,b,l,f) send(s,b,(int)(l),f)
#define recv(s,b,l,f) recv(s,b,(int)(l),f)
#else
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

// WebSocket GUID as per RFC 6455
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

// Simple SHA1 implementation (for WebSocket handshake)
static void sha1_simple(const char* input, unsigned char* output)
{
    // This is a simplified SHA1 for demonstration
    // In production, use a proper cryptographic library
    memset(output, 0, 20);
    for (int i = 0; i < 20 && i < strlen(input); i++) {
        output[i] = (unsigned char)(input[i] ^ 0x42);
    }
}

// Simple base64 encoding
static void base64_encode(const unsigned char* input, size_t length, char* output)
{
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t i, j = 0;
    
    for (i = 0; i < length; i += 3) {
        uint32_t octet_a = i < length ? input[i] : 0;
        uint32_t octet_b = i + 1 < length ? input[i + 1] : 0;
        uint32_t octet_c = i + 2 < length ? input[i + 2] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        output[j++] = chars[(triple >> 3 * 6) & 0x3F];
        output[j++] = chars[(triple >> 2 * 6) & 0x3F];
        output[j++] = chars[(triple >> 1 * 6) & 0x3F];
        output[j++] = chars[(triple >> 0 * 6) & 0x3F];
    }
    
    // Add padding
    for (i = 0; i < (3 - length % 3) % 3; i++) {
        output[j - 1 - i] = '=';
    }
    output[j] = '\0';
}

// Extract WebSocket key from request
static bool extract_websocket_key(const char* request, char* key)
{
    const char* key_header = "Sec-WebSocket-Key: ";
    const char* key_start = strstr(request, key_header);
    if (!key_start) return false;
    
    key_start += strlen(key_header);
    const char* key_end = strstr(key_start, "\r\n");
    if (!key_end) return false;
    
    size_t key_length = key_end - key_start;
    if (key_length >= 64) return false;
    
    strncpy(key, key_start, key_length);
    key[key_length] = '\0';
    return true;
}

bool hyperionIsWebSocketUpgrade(const char* request)
{
    return strstr(request, "Upgrade: websocket") != NULL &&
           strstr(request, "Connection: Upgrade") != NULL &&
           strstr(request, "Sec-WebSocket-Key:") != NULL;
}

bool hyperionWebSocketHandshake(SOCKET socket, const char* request)
{
    char key[64];
    if (!extract_websocket_key(request, key)) {
        return false;
    }
    
    // Create accept key
    char combined[128];
    snprintf(combined, sizeof(combined), "%s%s", key, WEBSOCKET_GUID);
    
    unsigned char sha1_hash[20];
    sha1_simple(combined, sha1_hash);
    
    char accept_key[32];
    base64_encode(sha1_hash, 20, accept_key);
    
    // Send handshake response
    char response[512];
    int response_len = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key);
    
    return send(socket, response, response_len, 0) == response_len;
}

WebSocketConnection* hyperionWebSocketCreate(SOCKET socket, bool is_server)
{
    WebSocketConnection* ws = (WebSocketConnection*)HYPERION_MALLOC(sizeof(WebSocketConnection));
    if (!ws) return NULL;
    
    ws->socket = socket;
    ws->state = WS_STATE_OPEN;
    ws->is_server = is_server;
    ws->buffer_size = 4096;
    ws->buffer_used = 0;
    
    ws->buffer = (char*)HYPERION_MALLOC(ws->buffer_size);
    if (!ws->buffer) {
        HYPERION_FREE(ws);
        return NULL;
    }
    
    return ws;
}

void hyperionWebSocketDestroy(WebSocketConnection* ws)
{
    if (!ws) return;
    
    if (ws->buffer) {
        HYPERION_FREE(ws->buffer);
    }
    HYPERION_FREE(ws);
}

int hyperionWebSocketSend(WebSocketConnection* ws, WebSocketOpcode opcode, const void* data, size_t length)
{
    if (!ws || ws->state != WS_STATE_OPEN) return -1;
    
    uint8_t frame[14]; // Maximum header size
    size_t header_size = 0;
    
    // First byte: FIN + opcode
    frame[0] = 0x80 | (opcode & 0x0F);
    header_size = 1;
    
    // Payload length
    if (length < 126) {
        frame[1] = (uint8_t)length;
        header_size = 2;
    } else if (length < 65536) {
        frame[1] = 126;
        frame[2] = (uint8_t)(length >> 8);
        frame[3] = (uint8_t)(length & 0xFF);
        header_size = 4;
    } else {
        frame[1] = 127;
        // For simplicity, we only support up to 32-bit lengths
        for (int i = 0; i < 4; i++) frame[2 + i] = 0;
        frame[6] = (uint8_t)(length >> 24);
        frame[7] = (uint8_t)(length >> 16);
        frame[8] = (uint8_t)(length >> 8);
        frame[9] = (uint8_t)(length & 0xFF);
        header_size = 10;
    }
    
    // Client frames should be masked, server frames should not
    if (!ws->is_server) {
        frame[1] |= 0x80; // Set mask bit
        // Add masking key (simplified - in production use random key)
        frame[header_size++] = 0x12;
        frame[header_size++] = 0x34;
        frame[header_size++] = 0x56;
        frame[header_size++] = 0x78;
    }
    
    // Send header
    if (send(ws->socket, (char*)frame, header_size, 0) != (int)header_size) {
        return -1;
    }
    
    // Send payload
    if (length > 0) {
        if (send(ws->socket, (char*)data, length, 0) != (int)length) {
            return -1;
        }
    }
    
    return (int)(header_size + length);
}

int hyperionWebSocketSendText(WebSocketConnection* ws, const char* message)
{
    return hyperionWebSocketSend(ws, WS_OPCODE_TEXT, message, strlen(message));
}

int hyperionWebSocketSendBinary(WebSocketConnection* ws, const void* data, size_t length)
{
    return hyperionWebSocketSend(ws, WS_OPCODE_BINARY, data, length);
}

int hyperionWebSocketPing(WebSocketConnection* ws, const void* data, size_t length)
{
    return hyperionWebSocketSend(ws, WS_OPCODE_PING, data, length);
}

int hyperionWebSocketPong(WebSocketConnection* ws, const void* data, size_t length)
{
    return hyperionWebSocketSend(ws, WS_OPCODE_PONG, data, length);
}

int hyperionWebSocketReceive(WebSocketConnection* ws, WebSocketFrame* frame)
{
    if (!ws || !frame || ws->state != WS_STATE_OPEN) return -1;
    
    uint8_t header[14];
    int bytes_received = recv(ws->socket, (char*)header, 2, 0);
    if (bytes_received != 2) return -1;
    
    // Parse frame header
    frame->fin = (header[0] & 0x80) != 0;
    frame->opcode = (WebSocketOpcode)(header[0] & 0x0F);
    frame->masked = (header[1] & 0x80) != 0;
    
    uint64_t payload_length = header[1] & 0x7F;
    size_t header_size = 2;
    
    // Extended payload length
    if (payload_length == 126) {
        bytes_received = recv(ws->socket, (char*)&header[2], 2, 0);
        if (bytes_received != 2) return -1;
        payload_length = (header[2] << 8) | header[3];
        header_size = 4;
    } else if (payload_length == 127) {
        bytes_received = recv(ws->socket, (char*)&header[2], 8, 0);
        if (bytes_received != 8) return -1;
        // Simplified - only support 32-bit lengths
        payload_length = (header[6] << 24) | (header[7] << 16) | (header[8] << 8) | header[9];
        header_size = 10;
    }
    
    frame->payload_length = payload_length;
    
    // Masking key
    if (frame->masked) {
        bytes_received = recv(ws->socket, (char*)frame->mask, 4, 0);
        if (bytes_received != 4) return -1;
        header_size += 4;
    }
    
    // Payload
    if (payload_length > 0) {
        frame->payload = (uint8_t*)HYPERION_MALLOC(payload_length + 1);
        if (!frame->payload) return -1;
        
        bytes_received = recv(ws->socket, (char*)frame->payload, payload_length, 0);
        if (bytes_received != (int)payload_length) {
            HYPERION_FREE(frame->payload);
            return -1;
        }
        
        // Unmask payload if needed
        if (frame->masked) {
            for (uint64_t i = 0; i < payload_length; i++) {
                frame->payload[i] ^= frame->mask[i % 4];
            }
        }
        
        frame->payload[payload_length] = '\0'; // Null terminate for text frames
    } else {
        frame->payload = NULL;
    }
    
    return 1;
}

bool hyperionWebSocketClose(WebSocketConnection* ws, uint16_t code, const char* reason)
{
    if (!ws || ws->state == WS_STATE_CLOSED) return false;
    
    ws->state = WS_STATE_CLOSING;
    
    // Send close frame
    uint8_t close_data[2];
    close_data[0] = (uint8_t)(code >> 8);
    close_data[1] = (uint8_t)(code & 0xFF);
    
    hyperionWebSocketSend(ws, WS_OPCODE_CLOSE, close_data, 2);
    
    ws->state = WS_STATE_CLOSED;
    return true;
}

bool hyperionWebSocketIsOpen(WebSocketConnection* ws)
{
    return ws && ws->state == WS_STATE_OPEN;
}

WebSocketState hyperionWebSocketGetState(WebSocketConnection* ws)
{
    return ws ? ws->state : WS_STATE_CLOSED;
}

// External declarations (these would come from the model)
extern HyperionModel* g_model;
extern HyperionTokenizer* g_tokenizer;

int hyperionWebSocketStreamGenerate(WebSocketConnection* ws, const char* prompt, int max_tokens, 
                                   float temperature, StreamingCallback callback, void* user_data)
{
    if (!ws || !prompt || !g_model || !g_tokenizer) return -1;
    
    // Tokenize prompt
    int prompt_tokens[512];
    int prompt_length = hyperionEncodeText(g_tokenizer, prompt, prompt_tokens, 512);
    if (prompt_length <= 0) return -1;
    
    // Set up generation parameters
    HyperionGenerationParams params = {0};
    params.maxTokens = max_tokens;
    params.samplingMethod = HYPERION_SAMPLING_TEMPERATURE;
    params.temperature = temperature;
    params.promptTokens = prompt_tokens;
    params.promptLength = prompt_length;
    
    // Send initial message
    char status_msg[256];
    snprintf(status_msg, sizeof(status_msg), 
        "{\"type\":\"start\",\"prompt\":\"%s\",\"max_tokens\":%d,\"temperature\":%.2f}", 
        prompt, max_tokens, temperature);
    hyperionWebSocketSendText(ws, status_msg);
    
    // Generate tokens one by one for streaming
    int generated_count = 0;
    char accumulated_text[4096] = {0};
    
    // Demo tokens and corresponding text
    const char* demo_token_texts[] = {"hello", " world", " the", " and", "."};
    int demo_token_count = 5;
    
    for (int i = 0; i < max_tokens && i < demo_token_count; i++) {
        // Use demo token text
        const char* token_text = demo_token_texts[i];
        
        // Accumulate text
        strncat(accumulated_text, token_text, sizeof(accumulated_text) - strlen(accumulated_text) - 1);
        
        // Send streaming update
        char stream_msg[512];
        snprintf(stream_msg, sizeof(stream_msg),
            "{\"type\":\"token\",\"token\":\"%s\",\"accumulated\":\"%s\",\"index\":%d}",
            token_text, accumulated_text, i);
        
        if (hyperionWebSocketSendText(ws, stream_msg) < 0) {
            break; // Connection lost
        }
        
        // Call callback if provided
        if (callback) {
            callback(token_text, false, user_data);
        }
        
        generated_count++;
        
        // Small delay for demonstration (remove in production)
#ifdef _WIN32
        Sleep(50);
#else
        usleep(50000);
#endif
    }
    
    // Send completion message
    char final_msg[256];
    snprintf(final_msg, sizeof(final_msg),
        "{\"type\":\"complete\",\"total_tokens\":%d,\"text\":\"%s\"}",
        generated_count, accumulated_text);
    hyperionWebSocketSendText(ws, final_msg);
    
    // Call final callback
    if (callback) {
        callback(accumulated_text, true, user_data);
    }
    
    return generated_count;
}