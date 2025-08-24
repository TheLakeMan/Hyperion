/**
 * @file streaming_optimizer.h
 * @brief Real-time streaming optimization for Hyperion WebSocket performance
 *
 * This module provides advanced streaming optimization capabilities including
 * adaptive buffering, incremental inference, connection pooling, stream multiplexing,
 * bandwidth optimization, and real-time performance monitoring for WebSocket connections.
 */

#ifndef HYPERION_STREAMING_OPTIMIZER_H
#define HYPERION_STREAMING_OPTIMIZER_H

#include "../interface/websocket.h"
#include "../core/config.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of concurrent streams */
#define HYPERION_MAX_CONCURRENT_STREAMS 256
#define HYPERION_MAX_BUFFER_SIZE 1048576  /* 1MB */
#define HYPERION_DEFAULT_CHUNK_SIZE 4096
#define HYPERION_MAX_FRAME_RATE 60       /* FPS for real-time streaming */

/**
 * Streaming optimization strategies
 */
typedef enum {
    HYPERION_STREAM_LATENCY_OPTIMIZED,    /* Minimize latency */
    HYPERION_STREAM_THROUGHPUT_OPTIMIZED, /* Maximize throughput */
    HYPERION_STREAM_BALANCED,             /* Balance latency and throughput */
    HYPERION_STREAM_BANDWIDTH_CONSCIOUS,   /* Optimize for limited bandwidth */
    HYPERION_STREAM_ADAPTIVE              /* Dynamically adapt based on conditions */
} HyperionStreamStrategy;

/**
 * Buffer management strategies
 */
typedef enum {
    HYPERION_BUFFER_FIXED,         /* Fixed size buffer */
    HYPERION_BUFFER_DYNAMIC,       /* Dynamic buffer sizing */
    HYPERION_BUFFER_RING,          /* Ring buffer for continuous streaming */
    HYPERION_BUFFER_PRIORITY       /* Priority-based buffer management */
} HyperionBufferStrategy;

/**
 * Streaming modes
 */
typedef enum {
    HYPERION_STREAM_TOKEN_BY_TOKEN,   /* Send each token immediately */
    HYPERION_STREAM_WORD_BY_WORD,     /* Send complete words */
    HYPERION_STREAM_SENTENCE_CHUNKS,  /* Send sentence fragments */
    HYPERION_STREAM_ADAPTIVE_CHUNKS,  /* Adaptive chunk sizing */
    HYPERION_STREAM_BATCHED          /* Batched streaming with optimal timing */
} HyperionStreamingMode;

/**
 * Quality of Service levels
 */
typedef enum {
    HYPERION_QOS_BEST_EFFORT,     /* No guarantees */
    HYPERION_QOS_LOW_LATENCY,     /* Prioritize low latency */
    HYPERION_QOS_HIGH_THROUGHPUT, /* Prioritize high throughput */
    HYPERION_QOS_RELIABLE         /* Prioritize reliability */
} HyperionQoSLevel;

/**
 * Stream statistics
 */
typedef struct {
    uint64_t totalBytesStreamed;      /* Total bytes streamed */
    uint64_t totalFramesSent;         /* Total WebSocket frames sent */
    uint64_t totalTokensGenerated;    /* Total tokens generated */
    
    float avgLatencyMs;               /* Average frame latency */
    float avgThroughputMbps;          /* Average throughput */
    float currentFrameRate;           /* Current frame rate */
    float bufferUtilization;          /* Buffer utilization percentage */
    
    uint32_t droppedFrames;           /* Number of dropped frames */
    uint32_t retransmissions;         /* Number of retransmissions */
    uint32_t adaptationEvents;        /* Number of adaptation events */
    
    time_t streamStartTime;           /* Stream start timestamp */
    time_t lastFrameTime;             /* Last frame timestamp */
} HyperionStreamStats;

/**
 * Stream buffer
 */
typedef struct {
    uint8_t *data;                    /* Buffer data */
    size_t size;                      /* Buffer size */
    size_t used;                      /* Used bytes */
    size_t readPos;                   /* Read position */
    size_t writePos;                  /* Write position */
    HyperionBufferStrategy strategy;  /* Buffer management strategy */
    bool isCircular;                  /* Whether buffer is circular */
} HyperionStreamBuffer;

/**
 * Incremental inference context
 */
typedef struct {
    int *currentTokens;               /* Current token sequence */
    int tokenCount;                   /* Number of tokens generated */
    int maxTokens;                    /* Maximum tokens to generate */
    float *hiddenStates;              /* Hidden states for incremental generation */
    size_t hiddenStateSize;           /* Size of hidden states */
    bool isActive;                    /* Whether inference is active */
    float lastGenerationTime;         /* Time of last token generation */
} HyperionIncrementalContext;

/**
 * Connection pool entry
 */
typedef struct {
    WebSocketConnection *connection;   /* WebSocket connection */
    char clientId[64];                /* Client identifier */
    HyperionQoSLevel qosLevel;        /* Quality of service level */
    HyperionStreamStats stats;        /* Stream statistics */
    HyperionStreamBuffer buffer;      /* Stream buffer */
    HyperionIncrementalContext incCtx; /* Incremental inference context */
    float priority;                   /* Connection priority */
    bool isActive;                    /* Whether connection is active */
    time_t lastActivity;              /* Last activity timestamp */
} HyperionStreamConnection;

/**
 * Streaming optimizer
 */
typedef struct HyperionStreamingOptimizer HyperionStreamingOptimizer;

/**
 * Streaming optimization configuration
 */
typedef struct {
    HyperionStreamStrategy strategy;          /* Optimization strategy */
    HyperionStreamingMode streamingMode;      /* Streaming mode */
    HyperionBufferStrategy bufferStrategy;    /* Buffer management */
    
    size_t maxBufferSize;                     /* Maximum buffer size per connection */
    size_t chunkSize;                         /* Default chunk size */
    float targetFrameRate;                    /* Target frame rate (FPS) */
    float maxLatencyMs;                       /* Maximum acceptable latency */
    float minThroughputMbps;                  /* Minimum required throughput */
    
    bool enableCompression;                   /* Enable frame compression */
    bool enableAdaptiveBitrate;               /* Enable adaptive bitrate */
    bool enablePredictiveBuffering;           /* Enable predictive buffering */
    bool enableConnectionPooling;             /* Enable connection pooling */
    
    int maxConcurrentStreams;                 /* Maximum concurrent streams */
    float congestionThreshold;                /* Congestion detection threshold */
    float adaptationSensitivity;              /* Adaptation sensitivity (0.0-1.0) */
    
    /* Performance tuning */
    int tcpNoDelay;                          /* TCP_NODELAY setting */
    int socketBufferSize;                    /* Socket buffer size */
    int keepAliveInterval;                   /* Keep-alive interval (seconds) */
} HyperionStreamingConfig;

/**
 * Streaming callback functions
 */
typedef struct {
    void (*onTokenGenerated)(const char *token, void *userData);
    void (*onStreamStart)(const char *streamId, void *userData);
    void (*onStreamEnd)(const char *streamId, void *userData);
    void (*onError)(const char *error, void *userData);
    void (*onBufferFull)(const char *streamId, void *userData);
    void (*onAdaptation)(const char *streamId, const char *reason, void *userData);
} HyperionStreamingCallbacks;

/**
 * Create streaming optimizer
 * @param config Streaming configuration
 * @return Newly created optimizer, or NULL on failure
 */
HyperionStreamingOptimizer *hyperionStreamingOptimizerCreate(const HyperionStreamingConfig *config);

/**
 * Free streaming optimizer
 * @param optimizer Optimizer to free
 */
void hyperionStreamingOptimizerFree(HyperionStreamingOptimizer *optimizer);

/**
 * Add WebSocket connection to optimizer
 * @param optimizer Streaming optimizer
 * @param connection WebSocket connection
 * @param clientId Client identifier
 * @param qosLevel Quality of service level
 * @return Connection index, or -1 on failure
 */
int hyperionStreamingAddConnection(HyperionStreamingOptimizer *optimizer,
                                  WebSocketConnection *connection,
                                  const char *clientId,
                                  HyperionQoSLevel qosLevel);

/**
 * Remove connection from optimizer
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @return true on success, false on failure
 */
bool hyperionStreamingRemoveConnection(HyperionStreamingOptimizer *optimizer,
                                      int connectionIndex);

/**
 * Start streaming inference session
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param prompt Input prompt
 * @param maxTokens Maximum tokens to generate
 * @param callbacks Streaming callbacks
 * @param userData User data for callbacks
 * @return Stream session ID, or NULL on failure
 */
char *hyperionStreamingStartInference(HyperionStreamingOptimizer *optimizer,
                                     int connectionIndex,
                                     const char *prompt,
                                     int maxTokens,
                                     const HyperionStreamingCallbacks *callbacks,
                                     void *userData);

/**
 * Stop streaming inference session
 * @param optimizer Streaming optimizer
 * @param sessionId Stream session ID
 * @return true on success, false on failure
 */
bool hyperionStreamingStopInference(HyperionStreamingOptimizer *optimizer,
                                   const char *sessionId);

/**
 * Send streaming token
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param token Token to send
 * @param isComplete Whether this is the last token
 * @return true on success, false on failure
 */
bool hyperionStreamingSendToken(HyperionStreamingOptimizer *optimizer,
                               int connectionIndex,
                               const char *token,
                               bool isComplete);

/**
 * Optimize buffer for connection
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param targetLatencyMs Target latency in milliseconds
 * @return true on success, false on failure
 */
bool hyperionStreamingOptimizeBuffer(HyperionStreamingOptimizer *optimizer,
                                    int connectionIndex,
                                    float targetLatencyMs);

/**
 * Enable adaptive streaming
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param enable Whether to enable adaptive streaming
 * @return true on success, false on failure
 */
bool hyperionStreamingEnableAdaptive(HyperionStreamingOptimizer *optimizer,
                                     int connectionIndex,
                                     bool enable);

/**
 * Set streaming quality of service
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param qosLevel Quality of service level
 * @return true on success, false on failure
 */
bool hyperionStreamingSetQoS(HyperionStreamingOptimizer *optimizer,
                             int connectionIndex,
                             HyperionQoSLevel qosLevel);

/**
 * Get streaming statistics
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param stats Output statistics structure
 * @return true on success, false on failure
 */
bool hyperionStreamingGetStats(const HyperionStreamingOptimizer *optimizer,
                              int connectionIndex,
                              HyperionStreamStats *stats);

/**
 * Monitor streaming performance
 * @param optimizer Streaming optimizer
 * @param performanceReport Output buffer for performance report JSON
 * @param reportSize Size of performance report buffer
 * @return true on success, false on failure
 */
bool hyperionStreamingMonitorPerformance(const HyperionStreamingOptimizer *optimizer,
                                         char *performanceReport,
                                         size_t reportSize);

/**
 * Compress frame data
 * @param optimizer Streaming optimizer
 * @param inputData Input frame data
 * @param inputSize Input data size
 * @param outputData Output compressed data
 * @param outputSize Output data size
 * @param compressionLevel Compression level (0-9)
 * @return true on success, false on failure
 */
bool hyperionStreamingCompressFrame(HyperionStreamingOptimizer *optimizer,
                                   const uint8_t *inputData, size_t inputSize,
                                   uint8_t *outputData, size_t *outputSize,
                                   int compressionLevel);

/**
 * Detect network congestion
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param congestionLevel Output congestion level (0.0-1.0)
 * @return true on success, false on failure
 */
bool hyperionStreamingDetectCongestion(const HyperionStreamingOptimizer *optimizer,
                                       int connectionIndex,
                                       float *congestionLevel);

/**
 * Adapt streaming parameters
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param networkConditions Current network conditions
 * @return true on success, false on failure
 */
bool hyperionStreamingAdaptParameters(HyperionStreamingOptimizer *optimizer,
                                     int connectionIndex,
                                     const char *networkConditions);

/**
 * Enable frame prediction
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param enable Whether to enable frame prediction
 * @return true on success, false on failure
 */
bool hyperionStreamingEnablePrediction(HyperionStreamingOptimizer *optimizer,
                                       int connectionIndex,
                                       bool enable);

/**
 * Set stream priority
 * @param optimizer Streaming optimizer
 * @param connectionIndex Connection index
 * @param priority Stream priority (0.0-1.0, higher = more priority)
 * @return true on success, false on failure
 */
bool hyperionStreamingSetPriority(HyperionStreamingOptimizer *optimizer,
                                  int connectionIndex,
                                  float priority);

/**
 * Get memory usage of streaming optimizer
 * @param optimizer Streaming optimizer
 * @param totalMemory Output for total memory usage
 * @param bufferMemory Output for buffer memory usage
 * @return true on success, false on failure
 */
bool hyperionStreamingGetMemoryUsage(const HyperionStreamingOptimizer *optimizer,
                                     size_t *totalMemory,
                                     size_t *bufferMemory);

/**
 * Enable/disable SIMD acceleration for streaming
 * @param optimizer Streaming optimizer
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionStreamingEnableSIMD(HyperionStreamingOptimizer *optimizer, bool enable);

/**
 * Benchmark streaming performance
 * @param optimizer Streaming optimizer
 * @param durationSeconds Benchmark duration in seconds
 * @param benchmarkResults Output buffer for benchmark results JSON
 * @param resultsSize Size of benchmark results buffer
 * @return true on success, false on failure
 */
bool hyperionStreamingBenchmark(HyperionStreamingOptimizer *optimizer,
                                int durationSeconds,
                                char *benchmarkResults,
                                size_t resultsSize);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_STREAMING_OPTIMIZER_H */