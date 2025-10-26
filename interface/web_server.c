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
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

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
#include "../core/logging.h"
#include "../utils/deployment_manager.h"
#include "../utils/monitoring_center.h"

#ifdef _WIN32
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#endif

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

typedef struct {
    char api_key[128];
    int require_api_key;
    int rate_limit_window_seconds;
    int rate_limit_max_requests;
} HyperionSecurityConfig;

typedef struct {
    char ip[64];
    time_t window_start;
    int request_count;
} HyperionRateLimitEntry;

static HyperionSecurityConfig g_security_config = {0};
static HyperionRateLimitEntry g_rate_limit_table[64] = {0};
static bool g_security_config_loaded = false;
static HyperionDeploymentManager *g_deployment_manager = NULL;
static HyperionMonitoringCenter *g_monitoring_center = NULL;

static void load_security_config(void)
{
    if (g_security_config_loaded) return;

    memset(&g_security_config, 0, sizeof(g_security_config));

    const char *api_key = hyperionConfigGetString("security.api_key", NULL);
    if (api_key && api_key[0] != '\0') {
        strncpy(g_security_config.api_key, api_key, sizeof(g_security_config.api_key) - 1);
    }

    int require_key_default = g_security_config.api_key[0] ? 1 : 0;
    int require_key = hyperionConfigGetBool("security.require_api_key", require_key_default);
    if (require_key && g_security_config.api_key[0] == '\0') {
        HYPERION_LOG_WARN("security.require_api_key enabled but security.api_key missing; disabling authentication");
        require_key = 0;
    }
    g_security_config.require_api_key = require_key ? 1 : 0;

    int window_seconds = hyperionConfigGetInt("security.rate_limit.window_seconds", 60);
    if (window_seconds <= 0) window_seconds = 60;
    g_security_config.rate_limit_window_seconds = window_seconds;

    int max_requests = hyperionConfigGetInt("security.rate_limit.max_requests", 120);
    if (max_requests < 1) max_requests = 120;
    g_security_config.rate_limit_max_requests = max_requests;

    memset(g_rate_limit_table, 0, sizeof(g_rate_limit_table));
    g_security_config_loaded = true;
}

static void log_security_event(const char *ip, const char *message)
{
    const char *addr = (ip && ip[0]) ? ip : "unknown";
    HYPERION_LOG_WARN("[security] %s (client=%s)", message, addr);
    monitor_increment_counter("security.events_total", "Security events", 1.0);
    if (g_monitoring_center) {
        char logbuf[160];
        snprintf(logbuf, sizeof(logbuf), "Security event: %s (client=%s)", message, addr);
        hyperionMonitoringRecordLog(g_monitoring_center, "WARN", logbuf);
    }
}

static bool constant_time_equals(const char *expected, const char *provided)
{
    if (!expected || !provided) return false;
    size_t expected_len = strlen(expected);
    size_t provided_len = strlen(provided);
    size_t max_len = expected_len > provided_len ? expected_len : provided_len;
    unsigned char diff = (unsigned char)(expected_len ^ provided_len);
    for (size_t i = 0; i < max_len; ++i) {
        unsigned char a = (i < expected_len) ? (unsigned char)expected[i] : 0;
        unsigned char b = (i < provided_len) ? (unsigned char)provided[i] : 0;
        diff |= (unsigned char)(a ^ b);
    }
    return diff == 0;
}

static void trim_whitespace(char *value)
{
    if (!value) return;
    size_t len = strlen(value);
    size_t start = 0;
    while (start < len && isspace((unsigned char)value[start])) start++;
    size_t end = len;
    while (end > start && isspace((unsigned char)value[end - 1])) end--;
    if (start > 0 || end < len) {
        memmove(value, value + start, end - start);
        value[end - start] = '\0';
    }
}

static bool extract_header_value(const char *request, const char *header, char *out, size_t out_len)
{
    if (!request || !header || !out || out_len == 0) return false;
    size_t header_len = strlen(header);
    const char *pos = request;
    while ((pos = strstr(pos, header)) != NULL) {
        if (pos == request || pos[-1] == '\n') {
            const char *value_start = pos + header_len;
            while (*value_start == ' ' || *value_start == '\t') value_start++;
            size_t index = 0;
            while (value_start[index] && value_start[index] != '\r' && value_start[index] != '\n' && index + 1 < out_len) {
                out[index] = value_start[index];
                index++;
            }
            out[index] = '\0';
            trim_whitespace(out);
            return index > 0;
        }
        pos += header_len;
    }
    return false;
}

static bool extract_query_param(const char *path, const char *key, char *out, size_t out_len)
{
    if (!path || !key || !out || out_len == 0) return false;
    const char *query = strchr(path, '?');
    if (!query) return false;
    query++;
    size_t key_len = strlen(key);
    while (*query) {
        const char *amp = strchr(query, '&');
        size_t segment_len = amp ? (size_t)(amp - query) : strlen(query);
        const char *eq = memchr(query, '=', segment_len);
        size_t name_len = eq ? (size_t)(eq - query) : segment_len;
        if (eq && name_len == key_len && strncmp(query, key, key_len) == 0) {
            size_t value_len = segment_len - key_len - 1;
            if (value_len >= out_len) value_len = out_len - 1;
            memcpy(out, eq + 1, value_len);
            out[value_len] = '\0';
            return true;
        }
        if (!amp) break;
        query = amp + 1;
    }
    return false;
}

static bool rate_limit_allow(const char *ip)
{
    load_security_config();
    if (!g_monitoring_center) {
        g_monitoring_center = hyperionMonitoringInstance();
    }
    if (g_monitoring_center) {
        hyperionMonitoringRecordLog(g_monitoring_center, "INFO", "Web server starting");
        monitor_set_gauge_value("http.websocket_connections", "Active WebSocket connections", 0.0);
    }
    if (!g_deployment_manager) {
        g_deployment_manager = hyperionDeploymentManagerCreate(16);
    }
    if (g_security_config.rate_limit_max_requests <= 0) {
        return true;
    }

    const char *addr = (ip && ip[0]) ? ip : "unknown";
    time_t now = time(NULL);
    int free_index = -1;
    time_t oldest_time = now;
    int oldest_index = 0;

    for (int i = 0; i < (int)(sizeof(g_rate_limit_table) / sizeof(g_rate_limit_table[0])); ++i) {
        if (g_rate_limit_table[i].ip[0] == '\0') {
            if (free_index == -1) free_index = i;
            continue;
        }
        if (strncmp(g_rate_limit_table[i].ip, addr, sizeof(g_rate_limit_table[i].ip)) == 0) {
            if ((now - g_rate_limit_table[i].window_start) >= g_security_config.rate_limit_window_seconds) {
                g_rate_limit_table[i].window_start = now;
                g_rate_limit_table[i].request_count = 0;
            }
            if (g_rate_limit_table[i].request_count >= g_security_config.rate_limit_max_requests) {
                monitor_increment_counter("security.rate_limited", "Rate-limited requests", 1.0);
                if (g_monitoring_center) {
                    char message[128];
                    snprintf(message, sizeof(message), "Rate limit exceeded for %s", addr);
                    hyperionMonitoringRecordLog(g_monitoring_center, "WARN", message);
                }
                return false;
            }
            g_rate_limit_table[i].request_count++;
            return true;
        }
        if (g_rate_limit_table[i].window_start < oldest_time) {
            oldest_time = g_rate_limit_table[i].window_start;
            oldest_index = i;
        }
    }

    int index = (free_index != -1) ? free_index : oldest_index;
    strncpy(g_rate_limit_table[index].ip, addr, sizeof(g_rate_limit_table[index].ip) - 1);
    g_rate_limit_table[index].ip[sizeof(g_rate_limit_table[index].ip) - 1] = '\0';
    g_rate_limit_table[index].window_start = now;
    g_rate_limit_table[index].request_count = 1;
    return true;
}

static void monitor_increment_counter(const char *name, const char *description, double delta)
{
    if (!g_monitoring_center) return;
    hyperionMonitoringIncrementCounter(g_monitoring_center, name, "count", description, delta);
}

static void monitor_set_gauge_value(const char *name, const char *description, double value)
{
    if (!g_monitoring_center) return;
    hyperionMonitoringSetGauge(g_monitoring_center, name, "count", description, value);
}

static void monitor_record_response(int status_code)
{
    monitor_increment_counter("http.responses_total", "HTTP responses", 1.0);
    char metric[64];
    snprintf(metric, sizeof(metric), "http.status_%d_total", status_code);
    monitor_increment_counter(metric, "HTTP responses by status", 1.0);
    if (status_code >= 500) {
        monitor_increment_counter("http.responses_5xx_total", "Server error responses", 1.0);
    } else if (status_code >= 400) {
        monitor_increment_counter("http.responses_4xx_total", "Client error responses", 1.0);
    }
}

static void monitor_record_endpoint(const char *path)
{
    if (!g_monitoring_center || !path) return;
    char metric[128] = "http.endpoint.";
    size_t idx = strlen(metric);
    const size_t base_len = idx;
    for (const char *p = path; *p && idx < sizeof(metric) - 1; ++p) {
        char ch = *p;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9')) {
            metric[idx++] = (char)tolower((unsigned char)ch);
        } else if (ch == '/') {
            if (idx == base_len) continue;
            metric[idx++] = '.';
        } else {
            metric[idx++] = '_';
        }
    }
    metric[idx] = '\0';
    monitor_increment_counter(metric, "HTTP endpoint requests", 1.0);
}

static bool authorize_request(const char *headers, const char *path, const char *ip, bool is_websocket)
{
    load_security_config();
    if (!g_security_config.require_api_key) {
        return true;
    }

    char provided_key[128] = {0};
    if (!extract_header_value(headers, "X-API-Key:", provided_key, sizeof(provided_key))) {
        extract_header_value(headers, "x-api-key:", provided_key, sizeof(provided_key));
    }

    if (provided_key[0] == '\0') {
        char auth_header[256] = {0};
        if (extract_header_value(headers, "Authorization:", auth_header, sizeof(auth_header)) ||
            extract_header_value(headers, "authorization:", auth_header, sizeof(auth_header))) {
            const char *bearer = auth_header;
            if (strncasecmp(bearer, "Bearer", 6) == 0) {
                bearer += 6;
                while (*bearer == ' ') bearer++;
                strncpy(provided_key, bearer, sizeof(provided_key) - 1);
            }
        }
    }

    if (provided_key[0] == '\0' && is_websocket) {
        extract_query_param(path, "api_key", provided_key, sizeof(provided_key));
    }

    if (provided_key[0] == '\0') {
        log_security_event(ip, "missing API key");
        monitor_increment_counter("security.unauthorized", "Unauthorized requests", 1.0);
        if (g_monitoring_center) {
            hyperionMonitoringRecordLog(g_monitoring_center, "WARN", "Rejected request: missing API key");
        }
        return false;
    }

    if (!constant_time_equals(g_security_config.api_key, provided_key)) {
        log_security_event(ip, "invalid API key");
        monitor_increment_counter("security.unauthorized", "Unauthorized requests", 1.0);
        if (g_monitoring_center) {
            hyperionMonitoringRecordLog(g_monitoring_center, "WARN", "Rejected request: invalid API key");
        }
        return false;
    }

    return true;
}

// WebSocket connections (simple array for demo)
#define MAX_WS_CONNECTIONS 16
static WebSocketConnection* g_ws_connections[MAX_WS_CONNECTIONS];
static int g_ws_connection_count = 0;

// WebSocket connection management
static void add_websocket_connection(WebSocketConnection* ws)
{
    if (g_ws_connection_count < MAX_WS_CONNECTIONS) {
        g_ws_connections[g_ws_connection_count++] = ws;
        monitor_set_gauge_value("http.websocket_connections", "Active WebSocket connections",
                                (double)g_ws_connection_count);
        monitor_increment_counter("http.websocket_upgrades", "WebSocket upgrades", 1.0);
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
    monitor_set_gauge_value("http.websocket_connections", "Active WebSocket connections",
                            (double)g_ws_connection_count);
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
    const char *response_body = body ? body : "";
    size_t body_len = strlen(response_body);
    const char *status_text = (status_code == 200) ? "OK" :
                              (status_code == 204) ? "No Content" :
                              (status_code == 400) ? "Bad Request" :
                              (status_code == 401) ? "Unauthorized" :
                              (status_code == 404) ? "Not Found" :
                              (status_code == 429) ? "Too Many Requests" :
                              (status_code == 500) ? "Internal Server Error" : "Unknown";

    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Authorization, X-API-Key\r\n"
        "Strict-Transport-Security: max-age=63072000; includeSubDomains\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "X-Frame-Options: DENY\r\n"
        "X-XSS-Protection: 1; mode=block\r\n"
        "Referrer-Policy: no-referrer\r\n"
        "Permissions-Policy: camera=(), microphone=()\r\n",
        status_code,
        status_text,
        content_type ? content_type : "application/octet-stream",
        body_len);

    if (status_code == 401) {
        header_len += snprintf(header + header_len, sizeof(header) - header_len,
                               "WWW-Authenticate: Bearer realm=\"Hyperion\"\r\n");
    }

    header_len += snprintf(header + header_len, sizeof(header) - header_len, "\r\n");

    send(client_socket, header, header_len, 0);
    if (body_len > 0) {
        send(client_socket, response_body, body_len, 0);
    }

    monitor_record_response(status_code);
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

static void handle_api_health(SOCKET client_socket)
{
    HyperionDeploymentHealth health = {0};
    if (g_deployment_manager) {
        hyperionDeploymentEvaluateHealth(g_deployment_manager, &health);
    }

    monitor_increment_counter("health.checks_total", "Health endpoint checks", 1.0);

    size_t total_memory = 0;
    size_t used_memory = 0;
    size_t peak_memory = 0;
    size_t allocations = 0;
    hyperionMemPoolStats(&total_memory, &used_memory, &peak_memory, &allocations);

    const char *status = (health.ready && (total_memory == 0 || used_memory * 100 < total_memory * 85))
                             ? "ok"
                             : "degraded";

    char response[512];
    snprintf(response, sizeof(response),
             "{\"status\":\"%s\",\"deployment\":{\"ready\":%s,\"last_state\":\"%s\",\"success_rate\":%.2f,\"active_replicas\":%zu,\"rollback_count\":%zu},"
             "\"memory\":{\"used\":%zu,\"total\":%zu,\"peak\":%zu,\"allocations\":%zu}}",
             status,
             health.ready ? "true" : "false",
             hyperionDeploymentStateName(health.last_state),
             health.success_rate,
             health.active_replicas,
             health.rollback_count,
             used_memory,
             total_memory,
             peak_memory,
             allocations);

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
static void handle_request(SOCKET client_socket, const char *document_root, const char *client_ip)
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
    monitor_increment_counter("http.requests_total", "HTTP requests", 1.0);
    monitor_record_endpoint(path);
    if (g_monitoring_center) {
        if (strcmp(method, "GET") == 0) {
            hyperionMonitoringIncrementCounter(g_monitoring_center, "http.method_get_total", "count",
                                               "GET requests", 1.0);
        } else if (strcmp(method, "POST") == 0) {
            hyperionMonitoringIncrementCounter(g_monitoring_center, "http.method_post_total", "count",
                                               "POST requests", 1.0);
        } else {
            hyperionMonitoringIncrementCounter(g_monitoring_center, "http.method_other_total", "count",
                                               "Other HTTP requests", 1.0);
        }
    }

    bool is_websocket = hyperionIsWebSocketUpgrade(buffer);
    bool is_api = strncmp(path, "/api/", 5) == 0;

    if (!rate_limit_allow(client_ip)) {
        log_security_event(client_ip, "rate limit exceeded");
        send_http_response(client_socket, 429, "application/json", "{\"error\": \"Too many requests\"}");
        return;
    }
    
    if ((is_api || is_websocket) && !authorize_request(buffer, path, client_ip, is_websocket)) {
        send_http_response(client_socket, 401, "application/json", "{\"error\": \"Unauthorized\"}");
        return;
    }
    
    // Check for WebSocket upgrade request
    if (is_websocket) {
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
            monitor_increment_counter("http.websocket_upgrade_failed", "Failed WebSocket upgrades", 1.0);
            send_http_response(client_socket, 400, "text/plain", "WebSocket handshake failed");
        }
        return;
    }
    
    // Handle CORS preflight
    if (strcmp(method, "OPTIONS") == 0) {
        send_http_response(client_socket, 204, "text/plain", "");
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
        else if (strcmp(path, "/api/health") == 0 && strcmp(method, "GET") == 0) {
            handle_api_health(client_socket);
        }
        else if (strcmp(path, "/api/monitoring") == 0 && strcmp(method, "GET") == 0) {
            char metrics[4096];
            char logs[4096];
            if (g_monitoring_center) {
                hyperionMonitoringExport(g_monitoring_center, metrics, sizeof(metrics));
                hyperionMonitoringExportLogs(g_monitoring_center, logs, sizeof(logs), 20);
            } else {
                strcpy(metrics, "{}");
                strcpy(logs, "[]");
            }
            char response[8192];
            snprintf(response, sizeof(response), "{\"metrics\":%s,\"logs\":%s}", metrics, logs);
            send_http_response(client_socket, 200, "application/json", response);
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

    load_security_config();
    
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
        
        char client_ip[64];
        const char *resolved_ip = inet_ntoa(client_addr.sin_addr);
        if (resolved_ip) {
            strncpy(client_ip, resolved_ip, sizeof(client_ip) - 1);
            client_ip[sizeof(client_ip) - 1] = '\0';
        } else {
            strncpy(client_ip, "unknown", sizeof(client_ip) - 1);
            client_ip[sizeof(client_ip) - 1] = '\0';
        }
        
        handle_request(client_socket, document_root, client_ip);
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

    if (g_deployment_manager) {
        hyperionDeploymentManagerDestroy(g_deployment_manager);
        g_deployment_manager = NULL;
    }

    if (g_monitoring_center) {
        hyperionMonitoringRecordLog(g_monitoring_center, "INFO", "Web server stopped");
        monitor_set_gauge_value("http.websocket_connections", "Active WebSocket connections", 0.0);
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