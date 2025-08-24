#include "distributed_inference.h"
#include "../core/memory.h"
#include "../utils/simd_ops.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#endif

/**
 * Distributed cluster structure
 */
struct HyperionDistributedCluster {
    HyperionDistributedConfig config;
    
    /* Node management */
    HyperionNodeInfo nodes[HYPERION_MAX_CLUSTER_NODES];
    int nodeCount;
    int localNodeIndex;
    bool isCoordinator;
    
    /* Communication */
    int coordinatorSocket;
    int workerSockets[HYPERION_MAX_CLUSTER_NODES];
    
    /* Task management */
    HyperionDistributedTask *taskQueue;
    int taskQueueSize;
    int taskQueueCapacity;
    
    /* Model partitioning */
    HyperionModelPartition *modelPartition;
    
    /* Statistics */
    uint64_t totalTasksProcessed;
    float avgTaskLatency;
    time_t clusterStartTime;
    
    /* Synchronization */
#ifndef _WIN32
    pthread_mutex_t clusterMutex;
    pthread_cond_t taskCondition;
#endif
    
    bool initialized;
    bool running;
};

/* Helper function to create socket */
static int createSocket(HyperionCommProtocol protocol) {
    int sockType = (protocol == HYPERION_COMM_UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int sock = socket(AF_INET, sockType, 0);
    if (sock < 0) return -1;
    
    /* Set socket options */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    return sock;
}

/* Helper function to send message */
static bool sendMessage(int socket, HyperionMessageType msgType, 
                       const void *data, size_t dataSize) {
    if (dataSize > HYPERION_MAX_MESSAGE_SIZE) return false;
    
    /* Message header: type (4 bytes) + size (4 bytes) + data */
    uint32_t header[2] = {(uint32_t)msgType, (uint32_t)dataSize};
    
    if (send(socket, (char*)header, sizeof(header), 0) != sizeof(header)) {
        return false;
    }
    
    if (dataSize > 0) {
        if (send(socket, (char*)data, dataSize, 0) != (int)dataSize) {
            return false;
        }
    }
    
    return true;
}

/* Helper function to receive message */
static bool receiveMessage(int socket, HyperionMessageType *msgType, 
                          void *buffer, size_t bufferSize, size_t *receivedSize) {
    uint32_t header[2];
    
    if (recv(socket, (char*)header, sizeof(header), 0) != sizeof(header)) {
        return false;
    }
    
    *msgType = (HyperionMessageType)header[0];
    *receivedSize = header[1];
    
    if (*receivedSize > bufferSize) return false;
    
    if (*receivedSize > 0) {
        if (recv(socket, (char*)buffer, *receivedSize, 0) != (int)*receivedSize) {
            return false;
        }
    }
    
    return true;
}

/* Load balancing: select best node for task */
static int selectNodeForTask(const HyperionDistributedCluster *cluster, 
                            const HyperionDistributedTask *task) {
    int bestNode = -1;
    float bestScore = -1.0f;
    
    for (int i = 0; i < cluster->nodeCount; i++) {
        const HyperionNodeInfo *node = &cluster->nodes[i];
        
        if (node->status != HYPERION_NODE_ACTIVE) continue;
        
        float score = 0.0f;
        switch (cluster->config.loadBalanceStrategy) {
            case HYPERION_LB_LEAST_LOADED:
                score = 1.0f - node->currentLoad;
                break;
            case HYPERION_LB_WEIGHTED:
                score = (1.0f - node->currentLoad) * node->computePower;
                break;
            case HYPERION_LB_DYNAMIC:
                score = (1.0f - node->currentLoad) * node->computePower / 
                       (1.0f + node->avgLatency);
                break;
            default: /* Round robin */
                score = 1.0f;
                break;
        }
        
        if (score > bestScore) {
            bestScore = score;
            bestNode = i;
        }
    }
    
    return bestNode;
}

HyperionDistributedCluster *hyperionDistributedClusterCreate(const HyperionDistributedConfig *config) {
    if (!config) return NULL;
    
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return NULL;
    }
#endif
    
    HyperionDistributedCluster *cluster = (HyperionDistributedCluster*)
        hyperionCalloc(1, sizeof(HyperionDistributedCluster));
    if (!cluster) return NULL;
    
    /* Copy configuration */
    cluster->config = *config;
    
    /* Initialize task queue */
    cluster->taskQueueCapacity = 1000;
    cluster->taskQueue = (HyperionDistributedTask*)
        hyperionCalloc(cluster->taskQueueCapacity, sizeof(HyperionDistributedTask));
    if (!cluster->taskQueue) {
        hyperionDistributedClusterFree(cluster);
        return NULL;
    }
    
    /* Initialize synchronization */
#ifndef _WIN32
    pthread_mutex_init(&cluster->clusterMutex, NULL);
    pthread_cond_init(&cluster->taskCondition, NULL);
#endif
    
    cluster->nodeCount = 0;
    cluster->localNodeIndex = -1;
    cluster->coordinatorSocket = -1;
    cluster->clusterStartTime = time(NULL);
    
    for (int i = 0; i < HYPERION_MAX_CLUSTER_NODES; i++) {
        cluster->workerSockets[i] = -1;
    }
    
    cluster->initialized = true;
    return cluster;
}

void hyperionDistributedClusterFree(HyperionDistributedCluster *cluster) {
    if (!cluster) return;
    
    cluster->running = false;
    
    /* Close sockets */
    if (cluster->coordinatorSocket >= 0) {
        close(cluster->coordinatorSocket);
    }
    
    for (int i = 0; i < HYPERION_MAX_CLUSTER_NODES; i++) {
        if (cluster->workerSockets[i] >= 0) {
            close(cluster->workerSockets[i]);
        }
    }
    
    /* Free resources */
    hyperionFree(cluster->taskQueue);
    
    if (cluster->modelPartition) {
        hyperionFree(cluster->modelPartition->partitionSizes);
        hyperionFree(cluster->modelPartition->nodeAssignments);
        hyperionFree(cluster->modelPartition->layerToStage);
        hyperionFree(cluster->modelPartition->tensorSplitDims);
        
        if (cluster->modelPartition->partitionData) {
            for (int i = 0; i < cluster->modelPartition->numPartitions; i++) {
                hyperionFree(cluster->modelPartition->partitionData[i]);
            }
            hyperionFree(cluster->modelPartition->partitionData);
        }
        
        hyperionFree(cluster->modelPartition);
    }
    
#ifndef _WIN32
    pthread_mutex_destroy(&cluster->clusterMutex);
    pthread_cond_destroy(&cluster->taskCondition);
#endif
    
    hyperionFree(cluster);
    
#ifdef _WIN32
    WSACleanup();
#endif
}

bool hyperionDistributedStartCoordinator(HyperionDistributedCluster *cluster, 
                                        const char *bindAddress) {
    if (!cluster || !cluster->initialized) return false;
    
    cluster->coordinatorSocket = createSocket(cluster->config.commProtocol);
    if (cluster->coordinatorSocket < 0) return false;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cluster->config.coordinatorPort);
    addr.sin_addr.s_addr = bindAddress ? inet_addr(bindAddress) : INADDR_ANY;
    
    if (bind(cluster->coordinatorSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(cluster->coordinatorSocket);
        cluster->coordinatorSocket = -1;
        return false;
    }
    
    if (listen(cluster->coordinatorSocket, HYPERION_MAX_CLUSTER_NODES) < 0) {
        close(cluster->coordinatorSocket);
        cluster->coordinatorSocket = -1;
        return false;
    }
    
    cluster->isCoordinator = true;
    cluster->running = true;
    
    /* Add local coordinator node */
    HyperionNodeInfo localNode = {0};
    strcpy(localNode.nodeId, "coordinator");
    strcpy(localNode.hostname, bindAddress ? bindAddress : "localhost");
    localNode.port = cluster->config.coordinatorPort;
    localNode.role = HYPERION_NODE_COORDINATOR;
    localNode.status = HYPERION_NODE_ACTIVE;
    localNode.protocol = cluster->config.commProtocol;
    localNode.cpuCores = 4; /* Default values */
    localNode.memorySize = 8 * 1024 * 1024 * 1024ULL; /* 8GB */
    localNode.computePower = 1.0f;
    localNode.lastHeartbeat = time(NULL);
    
    cluster->localNodeIndex = hyperionDistributedAddNode(cluster, &localNode);
    
    return cluster->localNodeIndex >= 0;
}

bool hyperionDistributedJoinCluster(HyperionDistributedCluster *cluster,
                                   const char *coordinatorAddress, int coordinatorPort,
                                   HyperionNodeRole nodeRole) {
    if (!cluster || !coordinatorAddress) return false;
    
    int socket = createSocket(cluster->config.commProtocol);
    if (socket < 0) return false;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(coordinatorPort);
    inet_pton(AF_INET, coordinatorAddress, &addr.sin_addr);
    
    if (connect(socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(socket);
        return false;
    }
    
    /* Send join request */
    HyperionNodeInfo joinInfo = {0};
    snprintf(joinInfo.nodeId, sizeof(joinInfo.nodeId), "node_%d", (int)time(NULL));
    strcpy(joinInfo.hostname, "localhost");
    joinInfo.role = nodeRole;
    joinInfo.status = HYPERION_NODE_ACTIVE;
    joinInfo.protocol = cluster->config.commProtocol;
    
    if (!sendMessage(socket, HYPERION_MSG_NODE_JOIN, &joinInfo, sizeof(joinInfo))) {
        close(socket);
        return false;
    }
    
    cluster->coordinatorSocket = socket;
    cluster->isCoordinator = false;
    cluster->running = true;
    
    return true;
}

bool hyperionDistributedLeaveCluster(HyperionDistributedCluster *cluster) {
    if (!cluster || !cluster->running) return false;
    
    if (!cluster->isCoordinator && cluster->coordinatorSocket >= 0) {
        sendMessage(cluster->coordinatorSocket, HYPERION_MSG_NODE_LEAVE, NULL, 0);
    }
    
    cluster->running = false;
    return true;
}

HyperionModelPartition *hyperionDistributedPartitionModel(HyperionDistributedCluster *cluster,
                                                        const HyperionModel *model,
                                                        HyperionPartitionStrategy strategy) {
    if (!cluster || !model) return NULL;
    
    HyperionModelPartition *partition = (HyperionModelPartition*)
        hyperionCalloc(1, sizeof(HyperionModelPartition));
    if (!partition) return NULL;
    
    partition->strategy = strategy;
    
    switch (strategy) {
        case HYPERION_PARTITION_LAYER_WISE: {
            partition->numPartitions = model->layerCount;
            partition->partitionSizes = (int*)hyperionCalloc(partition->numPartitions, sizeof(int));
            partition->nodeAssignments = (int*)hyperionCalloc(partition->numPartitions, sizeof(int));
            
            if (!partition->partitionSizes || !partition->nodeAssignments) {
                hyperionFree(partition->partitionSizes);
                hyperionFree(partition->nodeAssignments);
                hyperionFree(partition);
                return NULL;
            }
            
            /* Distribute layers across available nodes */
            int nodesAvailable = cluster->nodeCount;
            for (int i = 0; i < partition->numPartitions; i++) {
                partition->partitionSizes[i] = 1; /* One layer per partition */
                partition->nodeAssignments[i] = i % nodesAvailable;
            }
            break;
        }
        
        case HYPERION_PARTITION_PIPELINE: {
            partition->pipelineDepth = cluster->nodeCount;
            partition->numPartitions = partition->pipelineDepth;
            partition->layerToStage = (int*)hyperionCalloc(model->layerCount, sizeof(int));
            
            if (!partition->layerToStage) {
                hyperionFree(partition);
                return NULL;
            }
            
            /* Assign layers to pipeline stages */
            int layersPerStage = model->layerCount / partition->pipelineDepth;
            for (int i = 0; i < model->layerCount; i++) {
                partition->layerToStage[i] = i / layersPerStage;
                if (partition->layerToStage[i] >= partition->pipelineDepth) {
                    partition->layerToStage[i] = partition->pipelineDepth - 1;
                }
            }
            break;
        }
        
        default:
            /* Simple data parallel - replicate model on all nodes */
            partition->numPartitions = cluster->nodeCount;
            partition->partitionSizes = (int*)hyperionCalloc(partition->numPartitions, sizeof(int));
            partition->nodeAssignments = (int*)hyperionCalloc(partition->numPartitions, sizeof(int));
            
            for (int i = 0; i < partition->numPartitions; i++) {
                partition->partitionSizes[i] = model->layerCount; /* Full model */
                partition->nodeAssignments[i] = i;
            }
            break;
    }
    
    cluster->modelPartition = partition;
    return partition;
}

int hyperionDistributedGenerateText(HyperionDistributedCluster *cluster,
                                   HyperionGenerationParams *params,
                                   int *outputTokens, int maxTokens) {
    if (!cluster || !params || !outputTokens) return -1;
    
    /* Create distributed task */
    HyperionDistributedTask task = {0};
    snprintf(task.taskId, sizeof(task.taskId), "gen_%ld", time(NULL));
    task.taskType = HYPERION_MSG_TASK_REQUEST;
    task.priority = 1.0f;
    task.taskData = params;
    task.dataSize = sizeof(HyperionGenerationParams);
    task.createdTime = time(NULL);
    
    /* Select node for execution */
    int nodeIndex = selectNodeForTask(cluster, &task);
    if (nodeIndex < 0) return -1;
    
    task.assignedNodeIndex = nodeIndex;
    
    if (cluster->isCoordinator) {
        /* Execute locally if coordinator has the capability */
        if (cluster->nodes[nodeIndex].role == HYPERION_NODE_COORDINATOR) {
            /* Simplified local execution */
            for (int i = 0; i < maxTokens && i < 10; i++) {
                outputTokens[i] = 100 + i; /* Demo tokens */
            }
            return 10;
        } else {
            /* Send task to worker node */
            int workerSocket = cluster->workerSockets[nodeIndex];
            if (workerSocket < 0) return -1;
            
            if (!sendMessage(workerSocket, HYPERION_MSG_TASK_REQUEST, params, sizeof(*params))) {
                return -1;
            }
            
            /* Receive response */
            HyperionMessageType responseType;
            int responseTokens[maxTokens];
            size_t responseSize;
            
            if (!receiveMessage(workerSocket, &responseType, responseTokens, 
                              sizeof(responseTokens), &responseSize)) {
                return -1;
            }
            
            if (responseType == HYPERION_MSG_TASK_RESPONSE) {
                int numTokens = responseSize / sizeof(int);
                memcpy(outputTokens, responseTokens, responseSize);
                return numTokens;
            }
        }
    }
    
    return -1;
}

int hyperionDistributedAddNode(HyperionDistributedCluster *cluster, 
                              const HyperionNodeInfo *nodeInfo) {
    if (!cluster || !nodeInfo || cluster->nodeCount >= HYPERION_MAX_CLUSTER_NODES) {
        return -1;
    }
    
#ifndef _WIN32
    pthread_mutex_lock(&cluster->clusterMutex);
#endif
    
    int nodeIndex = cluster->nodeCount;
    cluster->nodes[nodeIndex] = *nodeInfo;
    cluster->nodes[nodeIndex].lastHeartbeat = time(NULL);
    cluster->nodeCount++;
    
#ifndef _WIN32
    pthread_mutex_unlock(&cluster->clusterMutex);
#endif
    
    return nodeIndex;
}

bool hyperionDistributedGetStats(const HyperionDistributedCluster *cluster,
                                int *totalNodes, int *activeNodes,
                                uint64_t *totalTasks, float *avgLatency) {
    if (!cluster) return false;
    
    if (totalNodes) *totalNodes = cluster->nodeCount;
    
    if (activeNodes) {
        int active = 0;
        for (int i = 0; i < cluster->nodeCount; i++) {
            if (cluster->nodes[i].status == HYPERION_NODE_ACTIVE) active++;
        }
        *activeNodes = active;
    }
    
    if (totalTasks) *totalTasks = cluster->totalTasksProcessed;
    if (avgLatency) *avgLatency = cluster->avgTaskLatency;
    
    return true;
}

const HyperionNodeInfo *hyperionDistributedGetNodeInfo(const HyperionDistributedCluster *cluster,
                                                      int nodeIndex) {
    if (!cluster || nodeIndex < 0 || nodeIndex >= cluster->nodeCount) {
        return NULL;
    }
    return &cluster->nodes[nodeIndex];
}

bool hyperionDistributedUpdateNodeLoad(HyperionDistributedCluster *cluster,
                                      int nodeIndex, float cpuLoad,
                                      size_t memoryUsed, int activeTasks) {
    if (!cluster || nodeIndex < 0 || nodeIndex >= cluster->nodeCount) {
        return false;
    }
    
#ifndef _WIN32
    pthread_mutex_lock(&cluster->clusterMutex);
#endif
    
    HyperionNodeInfo *node = &cluster->nodes[nodeIndex];
    node->currentLoad = cpuLoad;
    node->memoryUsed = memoryUsed;
    node->activeTasks = activeTasks;
    node->lastHeartbeat = time(NULL);
    
#ifndef _WIN32
    pthread_mutex_unlock(&cluster->clusterMutex);
#endif
    
    return true;
}

bool hyperionDistributedGetMemoryUsage(const HyperionDistributedCluster *cluster,
                                      size_t *localMemory, size_t *totalClusterMemory) {
    if (!cluster) return false;
    
    if (localMemory && cluster->localNodeIndex >= 0) {
        *localMemory = cluster->nodes[cluster->localNodeIndex].memoryUsed;
    }
    
    if (totalClusterMemory) {
        size_t total = 0;
        for (int i = 0; i < cluster->nodeCount; i++) {
            total += cluster->nodes[i].memoryUsed;
        }
        *totalClusterMemory = total;
    }
    
    return true;
}

bool hyperionDistributedEnableSIMD(HyperionDistributedCluster *cluster, bool enable) {
    if (!cluster) return false;
    /* Implementation would propagate SIMD settings to all nodes */
    return true;
}

bool hyperionDistributedSetCompression(HyperionDistributedCluster *cluster, int compressionLevel) {
    if (!cluster || compressionLevel < 0 || compressionLevel > 9) return false;
    /* Implementation would set compression level for network communications */
    return true;
}