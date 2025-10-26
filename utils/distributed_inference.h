/**
 * @file distributed_inference.h
 * @brief Distributed inference system for Hyperion multi-node processing
 *
 * This module provides comprehensive distributed inference capabilities including
 * model partitioning, node communication, load balancing, fault tolerance,
 * and distributed model execution across multiple compute nodes.
 */

#ifndef HYPERION_DISTRIBUTED_INFERENCE_H
#define HYPERION_DISTRIBUTED_INFERENCE_H

#include "../core/config.h"
#include "../models/text/generate.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of nodes in a distributed cluster */
#define HYPERION_MAX_CLUSTER_NODES 64
#define HYPERION_MAX_NODE_NAME_LEN 256
#define HYPERION_MAX_MESSAGE_SIZE 1048576  /* 1MB */
#define HYPERION_DEFAULT_CLUSTER_PORT 8888

/**
 * Node communication protocols
 */
typedef enum {
    HYPERION_COMM_TCP,           /* TCP socket communication */
    HYPERION_COMM_UDP,          /* UDP socket communication */
    HYPERION_COMM_RDMA,         /* Remote Direct Memory Access */
    HYPERION_COMM_MPI,          /* Message Passing Interface */
    HYPERION_COMM_WEBSOCKET     /* WebSocket for web-based nodes */
} HyperionCommProtocol;

/**
 * Node roles in distributed inference
 */
typedef enum {
    HYPERION_NODE_COORDINATOR,  /* Coordinates the cluster and distributes work */
    HYPERION_NODE_WORKER,       /* Executes inference tasks */
    HYPERION_NODE_AGGREGATOR,   /* Aggregates results from workers */
    HYPERION_NODE_GATEWAY       /* Handles external requests */
} HyperionNodeRole;

/**
 * Node status in the cluster
 */
typedef enum {
    HYPERION_NODE_INITIALIZING, /* Node is starting up */
    HYPERION_NODE_ACTIVE,       /* Node is active and ready */
    HYPERION_NODE_BUSY,         /* Node is processing tasks */
    HYPERION_NODE_FAILED,       /* Node has failed */
    HYPERION_NODE_DISCONNECTED  /* Node is disconnected */
} HyperionNodeStatus;

/**
 * Model partitioning strategies
 */
typedef enum {
    HYPERION_PARTITION_LAYER_WISE,     /* Distribute layers across nodes */
    HYPERION_PARTITION_TENSOR_PARALLEL, /* Split tensors across nodes */
    HYPERION_PARTITION_PIPELINE,       /* Pipeline parallelism */
    HYPERION_PARTITION_DATA_PARALLEL,   /* Replicate model, split data */
    HYPERION_PARTITION_HYBRID          /* Combine multiple strategies */
} HyperionPartitionStrategy;

/**
 * Load balancing algorithms
 */
typedef enum {
    HYPERION_LB_ROUND_ROBIN,    /* Simple round-robin assignment */
    HYPERION_LB_LEAST_LOADED,   /* Assign to least loaded node */
    HYPERION_LB_WEIGHTED,       /* Weighted based on node capacity */
    HYPERION_LB_DYNAMIC,        /* Dynamic load balancing */
    HYPERION_LB_CUSTOM         /* Custom load balancing algorithm */
} HyperionLoadBalanceStrategy;

/**
 * Fault tolerance modes
 */
typedef enum {
    HYPERION_FT_NONE,          /* No fault tolerance */
    HYPERION_FT_CHECKPOINT,    /* Checkpoint-based recovery */
    HYPERION_FT_REPLICATION,   /* Node replication */
    HYPERION_FT_MIGRATION,     /* Task migration on failure */
    HYPERION_FT_ADAPTIVE       /* Adaptive fault tolerance */
} HyperionFaultToleranceMode;

/**
 * Distributed communication message types
 */
typedef enum {
    HYPERION_MSG_HEARTBEAT,        /* Node heartbeat */
    HYPERION_MSG_TASK_REQUEST,     /* Task execution request */
    HYPERION_MSG_TASK_RESPONSE,    /* Task execution response */
    HYPERION_MSG_NODE_JOIN,        /* Node joining cluster */
    HYPERION_MSG_NODE_LEAVE,       /* Node leaving cluster */
    HYPERION_MSG_LOAD_REPORT,      /* Node load report */
    HYPERION_MSG_MODEL_SYNC,       /* Model synchronization */
    HYPERION_MSG_GRADIENT_UPDATE,  /* Gradient update message */
    HYPERION_MSG_BARRIER_SYNC,     /* Synchronization barrier */
    HYPERION_MSG_ERROR_REPORT      /* Error report */
} HyperionMessageType;

/**
 * Node information structure
 */
typedef struct {
    char nodeId[HYPERION_MAX_NODE_NAME_LEN];  /* Unique node identifier */
    char hostname[HYPERION_MAX_NODE_NAME_LEN]; /* Node hostname/IP */
    int port;                      /* Communication port */
    HyperionNodeRole role;         /* Node role */
    HyperionNodeStatus status;     /* Current node status */
    HyperionCommProtocol protocol; /* Communication protocol */
    
    /* Node capabilities */
    int cpuCores;                  /* Number of CPU cores */
    size_t memorySize;             /* Available memory in bytes */
    float computePower;            /* Relative compute power (1.0 = baseline) */
    bool hasGPU;                   /* Whether node has GPU */
    int gpuCount;                  /* Number of GPUs */
    
    /* Performance metrics */
    float currentLoad;             /* Current CPU/GPU load (0.0-1.0) */
    size_t memoryUsed;            /* Current memory usage */
    float avgLatency;             /* Average task latency */
    int activeTasks;              /* Number of active tasks */
    
    /* Network statistics */
    float networkLatency;         /* Network latency to coordinator */
    float networkBandwidth;       /* Available network bandwidth */
    uint64_t bytesSent;           /* Total bytes sent */
    uint64_t bytesReceived;       /* Total bytes received */
    
    time_t lastHeartbeat;         /* Last heartbeat timestamp */
    int heartbeatMissed;          /* Consecutive missed heartbeats */
} HyperionNodeInfo;

/**
 * Distributed task structure
 */
typedef struct {
    char taskId[64];              /* Unique task identifier */
    HyperionMessageType taskType; /* Type of task */
    int assignedNodeIndex;        /* Index of assigned node */
    float priority;               /* Task priority (higher = more urgent) */
    size_t dataSize;             /* Size of task data */
    void *taskData;              /* Task-specific data */
    time_t createdTime;          /* Task creation timestamp */
    time_t startTime;            /* Task start timestamp */
    time_t completedTime;        /* Task completion timestamp */
    bool isCompleted;            /* Whether task is completed */
    int retryCount;              /* Number of retry attempts */
} HyperionDistributedTask;

/**
 * Model partition information
 */
typedef struct {
    HyperionPartitionStrategy strategy; /* Partitioning strategy */
    int numPartitions;           /* Number of model partitions */
    int *partitionSizes;         /* Size of each partition */
    int *nodeAssignments;        /* Node assignment for each partition */
    void **partitionData;        /* Data for each partition */
    
    /* Pipeline specific */
    int pipelineDepth;           /* Pipeline depth for pipeline parallelism */
    int *layerToStage;          /* Mapping from layer to pipeline stage */
    
    /* Tensor parallel specific */
    int tensorParallelSize;      /* Number of tensor parallel groups */
    int *tensorSplitDims;       /* Dimensions to split for each layer */
} HyperionModelPartition;

/**
 * Distributed inference cluster
 */
typedef struct HyperionDistributedCluster HyperionDistributedCluster;

/**
 * Distributed inference configuration
 */
typedef struct {
    HyperionPartitionStrategy partitionStrategy; /* Model partitioning strategy */
    HyperionLoadBalanceStrategy loadBalanceStrategy; /* Load balancing algorithm */
    HyperionFaultToleranceMode faultTolerance; /* Fault tolerance mode */
    HyperionCommProtocol commProtocol; /* Communication protocol */
    
    int maxNodes;                /* Maximum number of nodes */
    int coordinatorPort;         /* Coordinator port */
    int workerBasePort;         /* Base port for worker nodes */
    float heartbeatInterval;    /* Heartbeat interval in seconds */
    int maxMissedHeartbeats;    /* Max missed heartbeats before failure */
    
    size_t maxMessageSize;      /* Maximum message size */
    float taskTimeout;          /* Task timeout in seconds */
    int maxRetries;             /* Maximum task retries */
    
    bool enableCompression;     /* Enable message compression */
    bool enableEncryption;      /* Enable message encryption */
    bool enableCheckpointing;   /* Enable checkpointing for fault tolerance */
    
    /* Performance tuning */
    int batchSize;              /* Batch size for distributed processing */
    int pipelineWidth;          /* Pipeline width for pipeline parallelism */
    float loadBalanceThreshold; /* Load imbalance threshold */
} HyperionDistributedConfig;

/**
 * Create distributed inference cluster
 * @param config Cluster configuration
 * @return Newly created cluster, or NULL on failure
 */
HyperionDistributedCluster *hyperionDistributedClusterCreate(const HyperionDistributedConfig *config);

/**
 * Free distributed inference cluster
 * @param cluster Cluster to free
 */
void hyperionDistributedClusterFree(HyperionDistributedCluster *cluster);

/**
 * Start cluster as coordinator node
 * @param cluster Distributed cluster
 * @param bindAddress Address to bind coordinator (NULL for all interfaces)
 * @return true on success, false on failure
 */
bool hyperionDistributedStartCoordinator(HyperionDistributedCluster *cluster, const char *bindAddress);

/**
 * Join cluster as worker node
 * @param cluster Distributed cluster
 * @param coordinatorAddress Coordinator address
 * @param coordinatorPort Coordinator port
 * @param nodeRole Role of this node
 * @return true on success, false on failure
 */
bool hyperionDistributedJoinCluster(HyperionDistributedCluster *cluster,
                                   const char *coordinatorAddress, int coordinatorPort,
                                   HyperionNodeRole nodeRole);

/**
 * Leave the distributed cluster
 * @param cluster Distributed cluster
 * @return true on success, false on failure
 */
bool hyperionDistributedLeaveCluster(HyperionDistributedCluster *cluster);

/**
 * Partition model for distributed inference
 * @param cluster Distributed cluster
 * @param model Model to partition
 * @param strategy Partitioning strategy
 * @return Model partition information, or NULL on failure
 */
HyperionModelPartition *hyperionDistributedPartitionModel(HyperionDistributedCluster *cluster,
                                                        const HyperionModel *model,
                                                        HyperionPartitionStrategy strategy);

/**
 * Distribute model partitions to nodes
 * @param cluster Distributed cluster
 * @param partition Model partition information
 * @return true on success, false on failure
 */
bool hyperionDistributedDeployModel(HyperionDistributedCluster *cluster,
                                   const HyperionModelPartition *partition);

/**
 * Distributed text generation
 * @param cluster Distributed cluster
 * @param params Generation parameters
 * @param outputTokens Output buffer for generated tokens
 * @param maxTokens Maximum number of tokens to generate
 * @return Number of tokens generated, or -1 on failure
 */
int hyperionDistributedGenerateText(HyperionDistributedCluster *cluster,
                                   HyperionGenerationParams *params,
                                   int *outputTokens, int maxTokens);

/**
 * Distributed batch inference
 * @param cluster Distributed cluster
 * @param batchParams Array of generation parameters for batch
 * @param batchSize Number of items in batch
 * @param results Output buffer for results
 * @param maxTokensPerItem Maximum tokens per batch item
 * @return Number of successfully processed items, or -1 on failure
 */
int hyperionDistributedBatchInference(HyperionDistributedCluster *cluster,
                                     HyperionGenerationParams *batchParams,
                                     int batchSize,
                                     int **results, int maxTokensPerItem);

/**
 * Add node to cluster
 * @param cluster Distributed cluster
 * @param nodeInfo Node information
 * @return Node index in cluster, or -1 on failure
 */
int hyperionDistributedAddNode(HyperionDistributedCluster *cluster, const HyperionNodeInfo *nodeInfo);

/**
 * Remove node from cluster
 * @param cluster Distributed cluster
 * @param nodeIndex Index of node to remove
 * @return true on success, false on failure
 */
bool hyperionDistributedRemoveNode(HyperionDistributedCluster *cluster, int nodeIndex);

/**
 * Get cluster statistics
 * @param cluster Distributed cluster
 * @param totalNodes Output for total number of nodes
 * @param activeNodes Output for number of active nodes
 * @param totalTasks Output for total tasks processed
 * @param avgLatency Output for average task latency
 * @return true on success, false on failure
 */
bool hyperionDistributedGetStats(const HyperionDistributedCluster *cluster,
                                int *totalNodes, int *activeNodes,
                                uint64_t *totalTasks, float *avgLatency);

/**
 * Get node information
 * @param cluster Distributed cluster
 * @param nodeIndex Index of node
 * @return Node information, or NULL if invalid index
 */
const HyperionNodeInfo *hyperionDistributedGetNodeInfo(const HyperionDistributedCluster *cluster,
                                                      int nodeIndex);

/**
 * Update node load information
 * @param cluster Distributed cluster
 * @param nodeIndex Index of node
 * @param cpuLoad Current CPU load (0.0-1.0)
 * @param memoryUsed Current memory usage in bytes
 * @param activeTasks Number of active tasks
 * @return true on success, false on failure
 */
bool hyperionDistributedUpdateNodeLoad(HyperionDistributedCluster *cluster,
                                      int nodeIndex, float cpuLoad,
                                      size_t memoryUsed, int activeTasks);

/**
 * Enable/disable fault tolerance
 * @param cluster Distributed cluster
 * @param mode Fault tolerance mode
 * @return true on success, false on failure
 */
bool hyperionDistributedSetFaultTolerance(HyperionDistributedCluster *cluster,
                                         HyperionFaultToleranceMode mode);

/**
 * Create checkpoint for fault tolerance
 * @param cluster Distributed cluster
 * @param checkpointPath Path to save checkpoint
 * @return true on success, false on failure
 */
bool hyperionDistributedCreateCheckpoint(HyperionDistributedCluster *cluster,
                                        const char *checkpointPath);

/**
 * Restore from checkpoint
 * @param cluster Distributed cluster
 * @param checkpointPath Path to checkpoint file
 * @return true on success, false on failure
 */
bool hyperionDistributedRestoreCheckpoint(HyperionDistributedCluster *cluster,
                                         const char *checkpointPath);

/**
 * Set load balancing strategy
 * @param cluster Distributed cluster
 * @param strategy Load balancing strategy
 * @return true on success, false on failure
 */
bool hyperionDistributedSetLoadBalancing(HyperionDistributedCluster *cluster,
                                        HyperionLoadBalanceStrategy strategy);

/**
 * Monitor cluster health
 * @param cluster Distributed cluster
 * @param healthReport Output buffer for health report JSON
 * @param reportSize Size of health report buffer
 * @return true on success, false on failure
 */
bool hyperionDistributedMonitorHealth(const HyperionDistributedCluster *cluster,
                                     char *healthReport, size_t reportSize);

/**
 * Get memory usage of distributed cluster
 * @param cluster Distributed cluster
 * @param localMemory Output for local memory usage
 * @param totalClusterMemory Output for total cluster memory usage
 * @return true on success, false on failure
 */
bool hyperionDistributedGetMemoryUsage(const HyperionDistributedCluster *cluster,
                                      size_t *localMemory, size_t *totalClusterMemory);

/**
 * Enable/disable SIMD acceleration
 * @param cluster Distributed cluster
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionDistributedEnableSIMD(HyperionDistributedCluster *cluster, bool enable);

/**
 * Set message compression level
 * @param cluster Distributed cluster
 * @param compressionLevel Compression level (0-9, 0=none, 9=max)
 * @return true on success, false on failure
 */
bool hyperionDistributedSetCompression(HyperionDistributedCluster *cluster, int compressionLevel);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_DISTRIBUTED_INFERENCE_H */