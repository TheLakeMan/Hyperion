/**
 * Hyperion Web Server Implementation
 * 
 * A lightweight HTTP server providing RESTful API endpoints for text generation,
 * model loading, and status monitoring as specified in the Hyperion analysis.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

#include "../core/config.h"
#include "../core/memory.h"
#include "../models/text/generate.h"
#include "../models/text/tokenizer.h"
#include "websocket.h"
#include "web_server.h"

// Simplified model creation for demo purposes
static HyperionModel* create_demo_model()
{
    HyperionModel* model = (HyperionModel*)HYPERION_MALLOC(sizeof(HyperionModel));
    if (!model) return NULL;
    
    // Initialize with demo values
    model->type = 1;
    model->layerCount = 1;
    model->layers = NULL;
    model->tokenizer = NULL;
    model->hiddenSize = 512;
    model->contextSize = 2048;
    model->activations[0] = NULL;
    model->activations[1] = NULL;
    model->activeBuffer = 0;
    
    return model;
}

// Simplified text generation for demo purposes
static int demo_generate_text(HyperionModel* model, HyperionGenerationParams* params, 
                             int* output_tokens, int max_tokens)
{
    // Demo implementation - just return some sample tokens
    const int demo_tokens[] = {1, 2, 3, 4, 5}; // Sample token IDs
    int count = (max_tokens < 5) ? max_tokens : 5;
    
    for (int i = 0; i < count; i++) {
        output_tokens[i] = demo_tokens[i];
    }
    
    return count;
}

// Global state
static HyperionModel *g_model = NULL;
static HyperionTokenizer *g_tokenizer = NULL;
static volatile int s_exit_flag = 0;

// WebSocket connections (simple array for demo)
#define MAX_WS_CONNECTIONS 16
static WebSocketConnection* g_ws_connections[MAX_WS_CONNECTIONS];
static int g_ws_connection_count = 0;

// WebSocket connection management
static void add_websocket_connection(WebSocketConnection* ws)
{
    if (g_ws_connection_count < MAX_WS_CONNECTIONS) {
        g_ws_connections[g_ws_connection_count++] = ws;
    }
}

static void remove_websocket_connection(WebSocketConnection* ws)
{
    for (int i = 0; i < g_ws_connection_count; i++) {
        if (g_ws_connections[i] == ws) {
            // Shift remaining connections
            for (int j = i; j < g_ws_connection_count - 1; j++) {
                g_ws_connections[j] = g_ws_connections[j + 1];
            }
            g_ws_connection_count--;
            break;
        }
    }
}

// Broadcast message to all WebSocket connections
static void broadcast_websocket_message(const char* message)
{
    for (int i = 0; i < g_ws_connection_count; i++) {
        if (hyperionWebSocketIsOpen(g_ws_connections[i])) {
            hyperionWebSocketSendText(g_ws_connections[i], message);
        }
    }
}

// HTTP response helper
static void send_http_response(SOCKET client_socket, int status_code, const char *content_type, const char *body)
{
    char header[1024];
    const char *status_text = (status_code == 200) ? "OK" : 
                             (status_code == 400) ? "Bad Request" :
                             (status_code == 404) ? "Not Found" :
                             (status_code == 500) ? "Internal Server Error" : "Unknown";
    
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "\r\n",
        status_code, status_text, content_type, strlen(body));
    
    send(client_socket, header, header_len, 0);
    send(client_socket, body, strlen(body), 0);
}

// Simple JSON value extraction
static char* extract_json_string(const char *json, const char *key)
{
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);
    
    const char *start = strstr(json, search_pattern);
    if (!start) return NULL;
    
    start += strlen(search_pattern);
    const char *end = strchr(start, '"');
    if (!end) return NULL;
    
    size_t len = end - start;
    char *result = malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, start, len);
    result[len] = '\0';
    return result;
}

// API Handler for /api/status
static void handle_api_status(SOCKET client_socket)
{
    const char *model_status = (g_model && g_tokenizer) ? "loaded" : "not_loaded";
    
    size_t total_memory = 0;
    size_t used_memory = 0;
    hyperionMemPoolStats(&total_memory, &used_memory, NULL, NULL);
    
    char response[512];
    snprintf(response, sizeof(response),
        "{"
        "\"status\": \"online\","
        "\"model_status\": \"%s\","
        "\"memory_used\": %zu,"
        "\"memory_total\": %zu,"
        "\"version\": \"0.1.0\""
        "}",
        model_status, used_memory, total_memory);
    
    send_http_response(client_socket, 200, "application/json", response);
}

// API Handler for /api/model/info
static void handle_api_model_info(SOCKET client_socket)
{
    if (!g_model || !g_tokenizer) {
        send_http_response(client_socket, 404, "application/json", 
            "{\"error\": \"No model loaded\"}");
        return;
    }
    
    char response[512];
    snprintf(response, sizeof(response),
        "{"
        "\"model_loaded\": true,"
        "\"context_size\": %d,"
        "\"hidden_size\": %d,"
        "\"vocab_size\": %d"
        "}",
        g_model->contextSize,
        g_model->hiddenSize,
        g_tokenizer->tokenCount);
    
    send_http_response(client_socket, 200, "application/json", response);
}

// API Handler for /api/generate
static void handle_api_generate(SOCKET client_socket, const char *body)
{
    char *prompt = extract_json_string(body, "prompt");
    if (!prompt) {
        send_http_response(client_socket, 400, "application/json", 
            "{\"error\": \"Missing or invalid prompt field\"}");
        return;
    }
    
    if (!g_model || !g_tokenizer) {
        free(prompt);
        send_http_response(client_socket, 503, "application/json", 
            "{\"error\": \"Model or tokenizer not loaded\"}");
        return;
    }
    
    // Prepare generation parameters
    HyperionGenerationParams params = {0};
    params.maxTokens = hyperionConfigGetInt("generate.max_tokens", 128);
    params.samplingMethod = HYPERION_SAMPLING_TEMPERATURE;
    params.temperature = hyperionConfigGetFloat("generate.temperature", 0.7f);
    params.topK = hyperionConfigGetInt("generate.top_k", 40);
    params.topP = hyperionConfigGetFloat("generate.top_p", 0.9f);
    params.seed = 0;
    
    // Tokenize prompt
    int prompt_tokens[512];
    params.promptLength = hyperionEncodeText(g_tokenizer, prompt, prompt_tokens, 512);
    if (params.promptLength <= 0) {
        free(prompt);
        send_http_response(client_socket, 400, "application/json", 
            "{\"error\": \"Failed to tokenize prompt\"}");
        return;
    }
    params.promptTokens = prompt_tokens;
    
    // Generate text
    int *output_tokens = malloc(params.maxTokens * sizeof(int));
    if (!output_tokens) {
        free(prompt);
        send_http_response(client_socket, 500, "application/json", 
            "{\"error\": \"Memory allocation failed\"}");
        return;
    }
    
    int generated_count = demo_generate_text(g_model, &params, output_tokens, params.maxTokens);
    
    if (generated_count > 0) {
        char result_text[4096];
        int decoded_len = hyperionDecodeTokens(g_tokenizer, output_tokens, generated_count, 
                                               result_text, sizeof(result_text) - 1);
        if (decoded_len > 0) {
            result_text[decoded_len] = '\0';
            char response[4096];
            snprintf(response, sizeof(response), "{\"result\": \"%s\"}", result_text);
            send_http_response(client_socket, 200, "application/json", response);
        } else {
            send_http_response(client_socket, 500, "application/json", 
                "{\"error\": \"Failed to decode tokens\"}");
        }
    } else {
        send_http_response(client_socket, 500, "application/json", 
            "{\"error\": \"Text generation failed\"}");
    }
    
    free(prompt);
    free(output_tokens);
}

// WebSocket streaming generation handler
static void handle_websocket_generate(WebSocketConnection* ws, const char* message)
{
    // Parse JSON message for generation parameters
    char* prompt = extract_json_string(message, "prompt");
    if (!prompt) {
        hyperionWebSocketSendText(ws, "{\"error\": \"Missing prompt field\"}");
        return;
    }
    
    if (!g_model || !g_tokenizer) {
        free(prompt);
        hyperionWebSocketSendText(ws, "{\"error\": \"Model or tokenizer not loaded\"}");
        return;
    }
    
    // Extract parameters (with defaults)
    int max_tokens = 100; // Default
    float temperature = 0.7f; // Default
    
    // Simple parameter extraction (in production, use proper JSON parser)
    const char* max_tokens_str = strstr(message, "\"max_tokens\":");
    if (max_tokens_str) {
        max_tokens_str += 13; // Skip "max_tokens":
        max_tokens = atoi(max_tokens_str);
        if (max_tokens <= 0 || max_tokens > 500) max_tokens = 100;
    }
    
    const char* temp_str = strstr(message, "\"temperature\":");
    if (temp_str) {
        temp_str += 14; // Skip "temperature":
        temperature = (float)atof(temp_str);
        if (temperature < 0.1f || temperature > 2.0f) temperature = 0.7f;
    }
    
    // Start streaming generation
    hyperionWebSocketStreamGenerate(ws, prompt, max_tokens, temperature, NULL, NULL);
    
    free(prompt);
}

// WebSocket message handler
static void handle_websocket_message(WebSocketConnection* ws, const char* message)
{
    printf("WebSocket message received: %s\n", message);
    
    // Parse message type
    if (strstr(message, "\"type\":\"generate\"")) {
        handle_websocket_generate(ws, message);
    }
    else if (strstr(message, "\"type\":\"ping\"")) {
        hyperionWebSocketSendText(ws, "{\"type\": \"pong\"}");
    }
    else if (strstr(message, "\"type\":\"status\"")) {
        // Send status via WebSocket
        const char* model_status = (g_model && g_tokenizer) ? "loaded" : "not_loaded";
        size_t used_memory = 0, total_memory = 0;
        hyperionMemPoolStats(&total_memory, &used_memory, NULL, NULL);
        
        char status_response[512];
        snprintf(status_response, sizeof(status_response),
            "{"
            "\"type\": \"status\","
            "\"status\": \"online\","
            "\"model_status\": \"%s\","
            "\"memory_used\": %zu,"
            "\"memory_total\": %zu,"
            "\"version\": \"0.1.0\","
            "\"websocket_connections\": %d"
            "}",
            model_status, used_memory, total_memory, g_ws_connection_count);
        
        hyperionWebSocketSendText(ws, status_response);
    }
    else {
        // Echo unknown messages
        char echo_response[256];
        snprintf(echo_response, sizeof(echo_response), 
            "{\"type\": \"echo\", \"message\": \"%.200s\"}", message);
        hyperionWebSocketSendText(ws, echo_response);
    }
}

// Serve static files
static void serve_file(SOCKET client_socket, const char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        send_http_response(client_socket, 404, "text/html", 
            "<html><body><h1>404 Not Found</h1></body></html>");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    const char *content_type = "text/html";
    if (strstr(file_path, ".css")) content_type = "text/css";
    else if (strstr(file_path, ".js")) content_type = "application/javascript";
    
    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        content_type, file_size);
    
    send(client_socket, header, header_len, 0);
    
    char buffer[8192];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }
    
    fclose(file);
}

// Handle HTTP requests
static void handle_request(SOCKET client_socket, const char *document_root)
{
    char buffer[8192];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) return;
    
    buffer[bytes_received] = '\0';
    
    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        send_http_response(client_socket, 400, "text/html", 
            "<html><body><h1>400 Bad Request</h1></body></html>");
        return;
    }
    
    printf("Request: %s %s\n", method, path);
    
    // Check for WebSocket upgrade request
    if (hyperionIsWebSocketUpgrade(buffer)) {
        printf("WebSocket upgrade request detected\n");
        
        if (hyperionWebSocketHandshake(client_socket, buffer)) {
            printf("WebSocket handshake successful\n");
            
            // Create WebSocket connection
            WebSocketConnection* ws = hyperionWebSocketCreate(client_socket, true);
            if (ws) {
                add_websocket_connection(ws);
                
                // Handle WebSocket messages
                WebSocketFrame frame;
                while (hyperionWebSocketIsOpen(ws)) {
                    int result = hyperionWebSocketReceive(ws, &frame);
                    if (result <= 0) break;
                    
                    switch (frame.opcode) {
                        case WS_OPCODE_TEXT:
                            if (frame.payload) {
                                handle_websocket_message(ws, (char*)frame.payload);
                                HYPERION_FREE(frame.payload);
                            }
                            break;
                            
                        case WS_OPCODE_PING:
                            hyperionWebSocketPong(ws, frame.payload, frame.payload_length);
                            if (frame.payload) HYPERION_FREE(frame.payload);
                            break;
                            
                        case WS_OPCODE_CLOSE:
                            hyperionWebSocketClose(ws, 1000, "Normal closure");
                            if (frame.payload) HYPERION_FREE(frame.payload);
                            break;
                            
                        default:
                            if (frame.payload) HYPERION_FREE(frame.payload);
                            break;
                    }
                }
                
                remove_websocket_connection(ws);
                hyperionWebSocketDestroy(ws);
            }
        } else {
            printf("WebSocket handshake failed\n");
            send_http_response(client_socket, 400, "text/plain", "WebSocket handshake failed");
        }
        return;
    }
    
    // Handle CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client_socket, 200, "text/plain", "");
        return;
    }
    
    // Find request body
    const char *body = strstr(buffer, "\r\n\r\n");
    if (body) body += 4;
    else body = "";
    
    // Route API endpoints
    if (strncmp(path, "/api/", 5) == 0) {
        if (strcmp(path, "/api/generate") == 0 && strcmp(method, "POST") == 0) {
            handle_api_generate(client_socket, body);
        }
        else if (strcmp(path, "/api/status") == 0 && strcmp(method, "GET") == 0) {
            handle_api_status(client_socket);
        }
        else if (strcmp(path, "/api/model/info") == 0 && strcmp(method, "GET") == 0) {
            handle_api_model_info(client_socket);
        }
        else {
            send_http_response(client_socket, 404, "application/json", 
                "{\"error\": \"Endpoint not found\"}");
        }
    }
    else {
        // Serve static files
        if (strcmp(path, "/") == 0) {
            strcpy(path, "/index.html");
        }
        
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s%s", document_root, path);
        serve_file(client_socket, file_path);
    }
}

// Main web server function
int start_web_server(picolInterp *interp, const char *port, const char *document_root)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    // Load model and tokenizer if configured
    const char *model_file = hyperionConfigGet("model.path", NULL);
    const char *tokenizer_file = hyperionConfigGet("tokenizer.path", NULL);
    
    if (model_file && tokenizer_file) {
        printf("Creating demo tokenizer...\n");
        g_tokenizer = hyperionCreateTokenizer();
        if (g_tokenizer) {
            // Add some demo tokens
            hyperionAddToken(g_tokenizer, "hello", 100);
            hyperionAddToken(g_tokenizer, "world", 90);
            hyperionAddToken(g_tokenizer, "the", 80);
            hyperionAddToken(g_tokenizer, "and", 70);
            hyperionAddToken(g_tokenizer, ".", 60);
            
            printf("Creating demo model...\n");
            g_model = create_demo_model();
            if (g_model) {
                g_model->tokenizer = g_tokenizer;
            }
        }
    }
    
    // Create socket
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed\n");
        return 1;
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(port));
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed on port %s\n", port);
        close(server_socket);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_socket, 10) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed\n");
        close(server_socket);
        return 1;
    }
    
    printf("Web server listening on port %s, serving %s\n", port, document_root);
    
    // Accept and handle connections
    while (!s_exit_flag) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }
        
        handle_request(client_socket, document_root);
        close(client_socket);
    }
    
    // Cleanup
    close(server_socket);
    
    if (g_model) {
        HYPERION_FREE(g_model);
        g_model = NULL;
    }
    if (g_tokenizer) {
        hyperionDestroyTokenizer(g_tokenizer);
        g_tokenizer = NULL;
    }
    
#ifdef _WIN32
    WSACleanup();
#endif
    
    return 0;
}

void stop_web_server() 
{
    s_exit_flag = 1;
}