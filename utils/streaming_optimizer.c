#include "streaming_optimizer.h"
#include "../core/memory.h"
#include "../utils/simd_ops.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <pthread.h>
#endif

/**
 * Streaming optimizer structure
 */
struct HyperionStreamingOptimizer {
    HyperionStreamingConfig config;
    
    /* Connection pool */
    HyperionStreamConnection connections[HYPERION_MAX_CONCURRENT_STREAMS];
    int connectionCount;
    
    /* Performance monitoring */
    HyperionStreamStats globalStats;
    float adaptiveParameters[4]; /* Latency, throughput, frame rate, buffer size */
    
    /* Streaming sessions */
    char activeSessions[HYPERION_MAX_CONCURRENT_STREAMS][64];
    int sessionCount;
    
    /* Synchronization */
#ifndef _WIN32
    pthread_mutex_t optimizerMutex;
    pthread_cond_t streamCondition;
#endif
    
    /* Adaptive algorithm state */
    float congestionHistory[10];    /* Recent congestion measurements */
    int historyIndex;
    float adaptationFactor;
    
    bool initialized;
    bool simdEnabled;
};

/* Helper function to initialize stream buffer */
static bool initStreamBuffer(HyperionStreamBuffer *buffer, size_t size,
                           HyperionBufferStrategy strategy) {
    buffer->data = (uint8_t*)hyperionAlloc(size);
    if (!buffer->data) return false;
    
    buffer->size = size;
    buffer->used = 0;
    buffer->readPos = 0;
    buffer->writePos = 0;
    buffer->strategy = strategy;
    buffer->isCircular = (strategy == HYPERION_BUFFER_RING);
    
    return true;
}

/* Helper function to free stream buffer */
static void freeStreamBuffer(HyperionStreamBuffer *buffer) {
    if (buffer->data) {
        hyperionFree(buffer->data);
        buffer->data = NULL;
    }
}

/* Helper function to write to buffer */
static size_t writeToBuffer(HyperionStreamBuffer *buffer, const uint8_t *data, size_t dataSize) {
    if (!buffer->data) return 0;
    
    if (buffer->isCircular) {
        /* Ring buffer logic */
        size_t availableSpace = buffer->size - buffer->used;
        size_t writeSize = (dataSize > availableSpace) ? availableSpace : dataSize;
        
        if (buffer->writePos + writeSize > buffer->size) {
            /* Wrap around */
            size_t firstChunk = buffer->size - buffer->writePos;
            size_t secondChunk = writeSize - firstChunk;
            
            memcpy(buffer->data + buffer->writePos, data, firstChunk);
            memcpy(buffer->data, data + firstChunk, secondChunk);
            buffer->writePos = secondChunk;
        } else {
            memcpy(buffer->data + buffer->writePos, data, writeSize);
            buffer->writePos += writeSize;
        }
        
        buffer->used += writeSize;
        return writeSize;
    } else {
        /* Linear buffer */
        size_t availableSpace = buffer->size - buffer->writePos;
        size_t writeSize = (dataSize > availableSpace) ? availableSpace : dataSize;
        
        memcpy(buffer->data + buffer->writePos, data, writeSize);
        buffer->writePos += writeSize;
        buffer->used += writeSize;
        
        return writeSize;
    }
}

/* Helper function to read from buffer */
static size_t readFromBuffer(HyperionStreamBuffer *buffer, uint8_t *data, size_t maxSize) {
    if (!buffer->data || buffer->used == 0) return 0;
    
    size_t readSize = (maxSize > buffer->used) ? buffer->used : maxSize;
    
    if (buffer->isCircular) {
        /* Ring buffer logic */
        if (buffer->readPos + readSize > buffer->size) {
            /* Wrap around */
            size_t firstChunk = buffer->size - buffer->readPos;
            size_t secondChunk = readSize - firstChunk;
            
            memcpy(data, buffer->data + buffer->readPos, firstChunk);
            memcpy(data + firstChunk, buffer->data, secondChunk);
            buffer->readPos = secondChunk;
        } else {
            memcpy(data, buffer->data + buffer->readPos, readSize);
            buffer->readPos += readSize;
        }
    } else {
        /* Linear buffer */
        memcpy(data, buffer->data + buffer->readPos, readSize);
        buffer->readPos += readSize;
    }
    
    buffer->used -= readSize;
    return readSize;
}

/* Adaptive algorithm for optimizing streaming parameters */
static void adaptiveOptimize(HyperionStreamingOptimizer *optimizer, int connectionIndex) {
    HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    HyperionStreamStats *stats = &conn->stats;
    
    /* Update adaptive parameters based on performance metrics */
    float targetLatency = optimizer->config.maxLatencyMs;
    float currentLatency = stats->avgLatencyMs;
    
    if (currentLatency > targetLatency * 1.2f) {
        /* High latency - reduce buffer size and chunk size */
        optimizer->adaptiveParameters[0] *= 0.9f; /* Reduce latency target */
        optimizer->adaptiveParameters[3] *= 0.8f; /* Reduce buffer size */
        
        if (conn->buffer.size > 1024) {
            size_t newSize = conn->buffer.size * 0.8f;
            uint8_t *newData = (uint8_t*)hyperionRealloc(conn->buffer.data, newSize);
            if (newData) {
                conn->buffer.data = newData;
                conn->buffer.size = newSize;
            }
        }
    } else if (currentLatency < targetLatency * 0.5f) {
        /* Low latency - can increase buffer for better throughput */
        optimizer->adaptiveParameters[1] *= 1.1f; /* Increase throughput target */
        optimizer->adaptiveParameters[3] *= 1.2f; /* Increase buffer size */
    }
    
    /* Frame rate adaptation */
    if (stats->currentFrameRate < optimizer->config.targetFrameRate * 0.8f) {
        optimizer->adaptiveParameters[2] *= 0.9f; /* Reduce frame rate target */
    }
    
    stats->adaptationEvents++;
}

/* Congestion detection algorithm */
static float detectCongestion(const HyperionStreamConnection *conn) {
    const HyperionStreamStats *stats = &conn->stats;
    
    /* Simple congestion detection based on multiple factors */
    float latencyFactor = (stats->avgLatencyMs > 100.0f) ? 
        (stats->avgLatencyMs / 100.0f) : 0.0f;
    float dropFactor = (stats->droppedFrames > 0) ? 
        ((float)stats->droppedFrames / stats->totalFramesSent) : 0.0f;
    float bufferFactor = (conn->buffer.used > conn->buffer.size * 0.8f) ? 
        ((float)conn->buffer.used / conn->buffer.size) : 0.0f;
    
    float congestion = (latencyFactor * 0.4f + dropFactor * 0.4f + bufferFactor * 0.2f);
    return (congestion > 1.0f) ? 1.0f : congestion;
}

HyperionStreamingOptimizer *hyperionStreamingOptimizerCreate(const HyperionStreamingConfig *config) {
    if (!config) return NULL;
    
    HyperionStreamingOptimizer *optimizer = (HyperionStreamingOptimizer*)
        hyperionCalloc(1, sizeof(HyperionStreamingOptimizer));
    if (!optimizer) return NULL;
    
    /* Copy configuration */
    optimizer->config = *config;
    
    /* Initialize adaptive parameters */
    optimizer->adaptiveParameters[0] = config->maxLatencyMs;      /* Target latency */
    optimizer->adaptiveParameters[1] = config->minThroughputMbps; /* Target throughput */
    optimizer->adaptiveParameters[2] = config->targetFrameRate;   /* Target frame rate */
    optimizer->adaptiveParameters[3] = (float)config->maxBufferSize; /* Target buffer size */
    
    optimizer->adaptationFactor = config->adaptationSensitivity;
    
    /* Initialize synchronization */
#ifndef _WIN32
    pthread_mutex_init(&optimizer->optimizerMutex, NULL);
    pthread_cond_init(&optimizer->streamCondition, NULL);
#endif
    
    optimizer->connectionCount = 0;
    optimizer->sessionCount = 0;
    optimizer->historyIndex = 0;
    optimizer->simdEnabled = true;
    
    /* Initialize global statistics */
    memset(&optimizer->globalStats, 0, sizeof(HyperionStreamStats));
    optimizer->globalStats.streamStartTime = time(NULL);
    
    optimizer->initialized = true;
    return optimizer;
}

void hyperionStreamingOptimizerFree(HyperionStreamingOptimizer *optimizer) {
    if (!optimizer) return;
    
    /* Free connection buffers */
    for (int i = 0; i < optimizer->connectionCount; i++) {
        freeStreamBuffer(&optimizer->connections[i].buffer);
        hyperionFree(optimizer->connections[i].incCtx.currentTokens);
        hyperionFree(optimizer->connections[i].incCtx.hiddenStates);
    }
    
#ifndef _WIN32
    pthread_mutex_destroy(&optimizer->optimizerMutex);
    pthread_cond_destroy(&optimizer->streamCondition);
#endif
    
    hyperionFree(optimizer);
}

int hyperionStreamingAddConnection(HyperionStreamingOptimizer *optimizer,
                                  WebSocketConnection *connection,
                                  const char *clientId,
                                  HyperionQoSLevel qosLevel) {
    if (!optimizer || !connection || !clientId || 
        optimizer->connectionCount >= HYPERION_MAX_CONCURRENT_STREAMS) {
        return -1;
    }
    
#ifndef _WIN32
    pthread_mutex_lock(&optimizer->optimizerMutex);
#endif
    
    int connectionIndex = optimizer->connectionCount;
    HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    
    /* Initialize connection */
    conn->connection = connection;
    strncpy(conn->clientId, clientId, sizeof(conn->clientId) - 1);
    conn->clientId[sizeof(conn->clientId) - 1] = '\0';
    conn->qosLevel = qosLevel;
    conn->priority = (qosLevel == HYPERION_QOS_LOW_LATENCY) ? 1.0f : 0.5f;
    conn->isActive = true;
    conn->lastActivity = time(NULL);
    
    /* Initialize stream buffer */
    size_t bufferSize = optimizer->config.maxBufferSize;
    if (qosLevel == HYPERION_QOS_LOW_LATENCY) {
        bufferSize /= 2; /* Smaller buffer for low latency */
    }
    
    if (!initStreamBuffer(&conn->buffer, bufferSize, optimizer->config.bufferStrategy)) {
#ifndef _WIN32
        pthread_mutex_unlock(&optimizer->optimizerMutex);
#endif
        return -1;
    }
    
    /* Initialize incremental inference context */
    conn->incCtx.maxTokens = 1000; /* Default */
    conn->incCtx.currentTokens = (int*)hyperionCalloc(conn->incCtx.maxTokens, sizeof(int));
    conn->incCtx.hiddenStateSize = 2048; /* Default hidden state size */
    conn->incCtx.hiddenStates = (float*)hyperionCalloc(conn->incCtx.hiddenStateSize, sizeof(float));
    
    if (!conn->incCtx.currentTokens || !conn->incCtx.hiddenStates) {
        freeStreamBuffer(&conn->buffer);
        hyperionFree(conn->incCtx.currentTokens);
        hyperionFree(conn->incCtx.hiddenStates);
#ifndef _WIN32
        pthread_mutex_unlock(&optimizer->optimizerMutex);
#endif
        return -1;
    }
    
    /* Initialize statistics */
    memset(&conn->stats, 0, sizeof(HyperionStreamStats));
    conn->stats.streamStartTime = time(NULL);
    
    /* Configure socket for optimization */
    if (optimizer->config.tcpNoDelay) {
        int flag = 1;
        setsockopt(connection->socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    }
    
    if (optimizer->config.socketBufferSize > 0) {
        int bufSize = optimizer->config.socketBufferSize;
        setsockopt(connection->socket, SOL_SOCKET, SO_SNDBUF, (char*)&bufSize, sizeof(bufSize));
        setsockopt(connection->socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufSize, sizeof(bufSize));
    }
    
    optimizer->connectionCount++;
    
#ifndef _WIN32
    pthread_mutex_unlock(&optimizer->optimizerMutex);
#endif
    
    return connectionIndex;
}

bool hyperionStreamingRemoveConnection(HyperionStreamingOptimizer *optimizer,
                                      int connectionIndex) {
    if (!optimizer || connectionIndex < 0 || connectionIndex >= optimizer->connectionCount) {
        return false;
    }
    
#ifndef _WIN32
    pthread_mutex_lock(&optimizer->optimizerMutex);
#endif
    
    HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    
    /* Clean up resources */
    freeStreamBuffer(&conn->buffer);
    hyperionFree(conn->incCtx.currentTokens);
    hyperionFree(conn->incCtx.hiddenStates);
    
    /* Shift remaining connections */
    for (int i = connectionIndex; i < optimizer->connectionCount - 1; i++) {
        optimizer->connections[i] = optimizer->connections[i + 1];
    }
    
    optimizer->connectionCount--;
    
#ifndef _WIN32
    pthread_mutex_unlock(&optimizer->optimizerMutex);
#endif
    
    return true;
}

char *hyperionStreamingStartInference(HyperionStreamingOptimizer *optimizer,
                                     int connectionIndex,
                                     const char *prompt,
                                     int maxTokens,
                                     const HyperionStreamingCallbacks *callbacks,
                                     void *userData) {
    if (!optimizer || !prompt || connectionIndex < 0 || 
        connectionIndex >= optimizer->connectionCount) {
        return NULL;
    }
    
    HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    
    /* Generate session ID */
    static int sessionCounter = 0;
    char *sessionId = (char*)hyperionAlloc(64);
    if (!sessionId) return NULL;
    
    snprintf(sessionId, 64, "stream_%d_%ld_%d", connectionIndex, time(NULL), sessionCounter++);
    
    /* Initialize incremental inference */
    conn->incCtx.tokenCount = 0;
    conn->incCtx.maxTokens = maxTokens;
    conn->incCtx.isActive = true;
    conn->incCtx.lastGenerationTime = 0.0f;
    
    /* Add to active sessions */
    if (optimizer->sessionCount < HYPERION_MAX_CONCURRENT_STREAMS) {
        strcpy(optimizer->activeSessions[optimizer->sessionCount], sessionId);
        optimizer->sessionCount++;
    }
    
    /* Call start callback */
    if (callbacks && callbacks->onStreamStart) {
        callbacks->onStreamStart(sessionId, userData);
    }
    
    return sessionId;
}

bool hyperionStreamingSendToken(HyperionStreamingOptimizer *optimizer,
                               int connectionIndex,
                               const char *token,
                               bool isComplete) {
    if (!optimizer || !token || connectionIndex < 0 || 
        connectionIndex >= optimizer->connectionCount) {
        return false;
    }
    
    HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    
    /* Create WebSocket frame */
    char frameData[1024];
    int frameSize = snprintf(frameData, sizeof(frameData),
        "{\"type\":\"%s\",\"token\":\"%s\",\"timestamp\":%ld}",
        isComplete ? "complete" : "token",
        token,
        time(NULL));
    
    if (frameSize >= sizeof(frameData)) {
        return false; /* Frame too large */
    }
    
    /* Adaptive streaming based on mode */
    bool shouldSend = true;
    
    switch (optimizer->config.streamingMode) {
        case HYPERION_STREAM_TOKEN_BY_TOKEN:
            /* Send immediately */
            break;
            
        case HYPERION_STREAM_WORD_BY_WORD:
            /* Buffer until word boundary (space or punctuation) */
            if (!isComplete && token[strlen(token) - 1] != ' ' && 
                strchr(".,!?;:", token[strlen(token) - 1]) == NULL) {
                shouldSend = false;
            }
            break;
            
        case HYPERION_STREAM_ADAPTIVE_CHUNKS:
            /* Use adaptive algorithm */
            float currentLatency = conn->stats.avgLatencyMs;
            if (currentLatency > optimizer->config.maxLatencyMs && 
                conn->buffer.used > conn->buffer.size * 0.5f) {
                shouldSend = true; /* Force send to reduce latency */
            } else if (conn->buffer.used < optimizer->config.chunkSize && !isComplete) {
                shouldSend = false; /* Buffer more data */
            }
            break;
            
        default:
            break;
    }
    
    if (shouldSend || isComplete) {
        /* Write to buffer or send directly */
        if (conn->qosLevel == HYPERION_QOS_LOW_LATENCY) {
            /* Send directly for low latency */
            time_t startTime = time(NULL);
            int result = hyperionWebSocketSend(conn->connection, WS_OPCODE_TEXT, 
                                              frameData, frameSize);
            
            if (result >= 0) {
                /* Update statistics */
                conn->stats.totalFramesSent++;
                conn->stats.totalBytesStreamed += frameSize;
                
                float latency = (float)(time(NULL) - startTime) * 1000.0f;
                conn->stats.avgLatencyMs = (conn->stats.avgLatencyMs * (conn->stats.totalFramesSent - 1) + 
                                          latency) / conn->stats.totalFramesSent;
                conn->stats.lastFrameTime = time(NULL);
                
                return true;
            } else {
                conn->stats.droppedFrames++;
                return false;
            }
        } else {
            /* Buffer for higher throughput modes */
            size_t written = writeToBuffer(&conn->buffer, (uint8_t*)frameData, frameSize);
            return written == frameSize;
        }
    }
    
    return true;
}

bool hyperionStreamingOptimizeBuffer(HyperionStreamingOptimizer *optimizer,
                                    int connectionIndex,
                                    float targetLatencyMs) {
    if (!optimizer || connectionIndex < 0 || connectionIndex >= optimizer->connectionCount) {
        return false;
    }
    
    HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    
    /* Calculate optimal buffer size based on target latency */
    float throughputMbps = conn->stats.avgThroughputMbps;
    if (throughputMbps <= 0) {
        throughputMbps = 1.0f; /* Default assumption */
    }
    
    /* Buffer size = (throughput * latency) / 8 (bits to bytes) */
    size_t optimalSize = (size_t)((throughputMbps * 1000000.0f * targetLatencyMs / 1000.0f) / 8.0f);
    
    /* Clamp to reasonable bounds */
    if (optimalSize < 1024) optimalSize = 1024;
    if (optimalSize > optimizer->config.maxBufferSize) {
        optimalSize = optimizer->config.maxBufferSize;
    }
    
    /* Resize buffer if needed */
    if (optimalSize != conn->buffer.size) {
        uint8_t *newData = (uint8_t*)hyperionRealloc(conn->buffer.data, optimalSize);
        if (newData) {
            conn->buffer.data = newData;
            conn->buffer.size = optimalSize;
            
            /* Adjust pointers if necessary */
            if (conn->buffer.writePos > optimalSize) conn->buffer.writePos = optimalSize;
            if (conn->buffer.readPos > optimalSize) conn->buffer.readPos = 0;
            if (conn->buffer.used > optimalSize) conn->buffer.used = optimalSize;
            
            return true;
        }
    }
    
    return false;
}

bool hyperionStreamingGetStats(const HyperionStreamingOptimizer *optimizer,
                              int connectionIndex,
                              HyperionStreamStats *stats) {
    if (!optimizer || !stats || connectionIndex < 0 || 
        connectionIndex >= optimizer->connectionCount) {
        return false;
    }
    
    *stats = optimizer->connections[connectionIndex].stats;
    
    /* Update real-time metrics */
    time_t currentTime = time(NULL);
    time_t duration = currentTime - stats->streamStartTime;
    
    if (duration > 0) {
        stats->avgThroughputMbps = (float)(stats->totalBytesStreamed * 8) / 
                                  (duration * 1000000.0f);
    }
    
    if (stats->totalFramesSent > 0) {
        stats->currentFrameRate = (float)stats->totalFramesSent / duration;
    }
    
    const HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    stats->bufferUtilization = (float)conn->buffer.used / conn->buffer.size;
    
    return true;
}

bool hyperionStreamingDetectCongestion(const HyperionStreamingOptimizer *optimizer,
                                       int connectionIndex,
                                       float *congestionLevel) {
    if (!optimizer || !congestionLevel || connectionIndex < 0 || 
        connectionIndex >= optimizer->connectionCount) {
        return false;
    }
    
    const HyperionStreamConnection *conn = &optimizer->connections[connectionIndex];
    *congestionLevel = detectCongestion(conn);
    
    return true;
}

bool hyperionStreamingEnableSIMD(HyperionStreamingOptimizer *optimizer, bool enable) {
    if (!optimizer) return false;
    
    optimizer->simdEnabled = enable;
    return true;
}

bool hyperionStreamingGetMemoryUsage(const HyperionStreamingOptimizer *optimizer,
                                     size_t *totalMemory,
                                     size_t *bufferMemory) {
    if (!optimizer) return false;
    
    size_t total = sizeof(HyperionStreamingOptimizer);
    size_t buffers = 0;
    
    for (int i = 0; i < optimizer->connectionCount; i++) {
        const HyperionStreamConnection *conn = &optimizer->connections[i];
        total += sizeof(HyperionStreamConnection);
        buffers += conn->buffer.size;
        total += conn->incCtx.maxTokens * sizeof(int);
        total += conn->incCtx.hiddenStateSize * sizeof(float);
    }
    
    if (totalMemory) *totalMemory = total;
    if (bufferMemory) *bufferMemory = buffers;
    
    return true;
}