/**
 * @file hybrid_generate.c
 * @brief Implementation of hybrid text generation for Hyperion
 */

#include "hybrid_generate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Context structure for hybrid text generation
 */
struct HyperionHybridGenerate {
    HyperionModel     *localModel;           /* Local model for generation */
    HyperionMcpClient *mcpClient;            /* MCP client for remote generation */
    bool             usedRemoteExecution;  /* Whether the last generation used remote execution */
    bool             forceRemote;          /* Force remote execution for next generation */
    bool             forceLocal;           /* Force local execution for next generation */
    double           lastLocalTimeMs;      /* Time spent in local execution (ms) */
    double           lastRemoteTimeMs;     /* Time spent in remote execution (ms) */
    int              lastTokenCount;       /* Number of tokens generated in last operation */
    double           lastGenerationTimeMs; /* Total time for last generation (ms) */
    char             lastError[256];       /* Last error message */
};

/* Helper function to get current time in milliseconds */
static double getCurrentTimeMs()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}

/* Helper function to decide if remote execution should be used */
static bool shouldUseRemote(HyperionHybridGenerate *ctx, const HyperionGenerationParams *params)
{
    /* If no MCP client, always use local */
    if (!ctx->mcpClient) {
        return false;
    }

    /* If force local, use local */
    if (ctx->forceLocal) {
        return false;
    }

    /* If force remote, try to use remote */
    if (ctx->forceRemote) {
        return true;
    }

    /* If MCP is not available, use local */
    if (!hyperionMcpIsAvailable(ctx->mcpClient)) {
        return false;
    }

    /* Get execution preference */
    HyperionMcpExecutionPreference pref = hyperionMcpGetExecutionPreference(ctx->mcpClient);

    /* Based on preference, decide execution environment */
    switch (pref) {
    case HYPERION_EXEC_ALWAYS_LOCAL:
        return false;

    case HYPERION_EXEC_PREFER_LOCAL:
        /* Use remote only if local model is inadequate */

        /* If prompt length exceeds local context size, use remote */
        if (params->promptLength > 0 && ctx->localModel) {
            int localContextSize = ctx->localModel->contextSize;
            if (params->promptLength > localContextSize * 0.8) {
                return true;
            }
        }

        /* If requesting too many tokens for local model, use remote */
        if (params->maxTokens > 0 && ctx->localModel) {
            int localContextSize = ctx->localModel->contextSize;
            if (params->maxTokens > localContextSize * 0.5) {
                return true;
            }
        }

        /* Otherwise use local */
        return false;

    case HYPERION_EXEC_PREFER_MCP:
        /* Use local only if remote is inadequate */
        return true;

    case HYPERION_EXEC_CUSTOM_POLICY:
        /* Implement any custom decision logic here */
        /* For now, use a heuristic based on prompt length */
        if (params->promptLength > 100) {
            return true;
        }
        return false;

    default:
        return false;
    }
}

/* Helper function to execute remote generation via MCP */
static int executeRemoteGeneration(HyperionHybridGenerate *ctx, const HyperionGenerationParams *params,
                                   int *outputTokens, int maxTokens)
{
    if (!ctx || !params || !outputTokens || maxTokens <= 0) {
        return -1;
    }

    double startTime = getCurrentTimeMs();

    /* Prepare arguments as JSON */
    char argsJson[1024];
    snprintf(argsJson, sizeof(argsJson) - 1,
             "{"
             "  \"prompt\": [%d",
             params->promptTokens[0]);

    /* Add remaining prompt tokens */
    for (int i = 1; i < params->promptLength; i++) {
        char tokenStr[16];
        snprintf(tokenStr, sizeof(tokenStr) - 1, ", %d", params->promptTokens[i]);
        strncat(argsJson, tokenStr, sizeof(argsJson) - strlen(argsJson) - 1);
    }

    /* Add generation parameters */
    char paramStr[512];
    snprintf(paramStr, sizeof(paramStr) - 1,
             "],"
             "  \"max_tokens\": %d,"
             "  \"temperature\": %.2f,"
             "  \"sampling_method\": \"%s\","
             "  \"top_k\": %d,"
             "  \"top_p\": %.2f,"
             "  \"seed\": %d"
             "}",
             params->maxTokens, params->temperature,
             params->samplingMethod == 0   ? "greedy"
             : params->samplingMethod == 1 ? "top_k"
             : params->samplingMethod == 2 ? "top_p"
                                           : "temperature",
             params->topK, params->topP, params->seed);

    strncat(argsJson, paramStr, sizeof(argsJson) - strlen(argsJson) - 1);

    /* Call MCP tool for text generation */
    char resultJson[8192];
    int  resultCode = hyperionMcpCallTool(ctx->mcpClient, "generate_text", argsJson, resultJson,
                                        sizeof(resultJson));

    if (resultCode < 0) {
        snprintf(ctx->lastError, sizeof(ctx->lastError) - 1, "MCP remote generation failed: %s",
                 resultJson);
        return -1;
    }

    /* Parse result JSON to extract generated tokens */
    /*
     * In a real implementation, we would:
     * 1. Parse the JSON using a proper JSON parser
     * 2. Extract the token array
     * 3. Copy tokens to outputTokens buffer
     *
     * For this demonstration, we'll simulate parsing with a placeholder
     * response that generates 10 tokens.
     */

    /* Simulate tokens for demonstration purposes */
    int generatedTokens = params->maxTokens > 10 ? 10 : params->maxTokens;
    for (int i = 0; i < generatedTokens; i++) {
        outputTokens[i] = i + 100; /* Placeholder token IDs */
    }

    double endTime            = getCurrentTimeMs();
    ctx->lastRemoteTimeMs     = endTime - startTime;
    ctx->lastLocalTimeMs      = 0;
    ctx->lastGenerationTimeMs = ctx->lastRemoteTimeMs;
    ctx->lastTokenCount       = generatedTokens;
    ctx->usedRemoteExecution  = true;

    return generatedTokens;
}

/* Helper function to execute local generation */
static int executeLocalGeneration(HyperionHybridGenerate *ctx, const HyperionGenerationParams *params,
                                  int *outputTokens, int maxTokens)
{
    if (!ctx || !params || !outputTokens || maxTokens <= 0 || !ctx->localModel) {
        return -1;
    }

    double startTime = getCurrentTimeMs();

    /* Call the local model's generation function */
    int result = hyperionGenerateText(ctx->localModel, params, outputTokens, maxTokens);

    double endTime            = getCurrentTimeMs();
    ctx->lastLocalTimeMs      = endTime - startTime;
    ctx->lastRemoteTimeMs     = 0;
    ctx->lastGenerationTimeMs = ctx->lastLocalTimeMs;
    ctx->lastTokenCount       = result > 0 ? result : 0;
    ctx->usedRemoteExecution  = false;

    return result;
}

HyperionHybridGenerate *hyperionCreateHybridGenerate(HyperionModel     *localModel,
                                                 HyperionMcpClient *mcpClient)
{
    HyperionHybridGenerate *ctx = (HyperionHybridGenerate *)malloc(sizeof(HyperionHybridGenerate));
    if (!ctx)
        return NULL;

    memset(ctx, 0, sizeof(HyperionHybridGenerate));
    ctx->localModel = localModel;
    ctx->mcpClient  = mcpClient;

    return ctx;
}

void hyperionDestroyHybridGenerate(HyperionHybridGenerate *ctx)
{
    if (!ctx)
        return;

    /* Note: we don't free localModel or mcpClient as they are owned elsewhere */

    free(ctx);
}

int hyperionHybridGenerateText(HyperionHybridGenerate *ctx, const HyperionGenerationParams *params,
                             int *outputTokens, int maxTokens)
{
    if (!ctx || !params || !outputTokens || maxTokens <= 0) {
        return -1;
    }

    /* Decide whether to use remote or local execution */
    bool useRemote = shouldUseRemote(ctx, params);

    /* Reset force flags for next call */
    ctx->forceLocal  = false;
    ctx->forceRemote = false;

    int result;
    if (useRemote) {
        /* Try remote execution */
        result = executeRemoteGeneration(ctx, params, outputTokens, maxTokens);

        /* If remote fails, fall back to local if available */
        if (result < 0 && ctx->localModel) {
            result = executeLocalGeneration(ctx, params, outputTokens, maxTokens);
        }
    }
    else {
        /* Use local execution */
        result = executeLocalGeneration(ctx, params, outputTokens, maxTokens);
    }

    return result;
}

bool hyperionHybridGenerateUsedRemote(HyperionHybridGenerate *ctx)
{
    if (!ctx)
        return false;
    return ctx->usedRemoteExecution;
}

void hyperionHybridGenerateGetStats(HyperionHybridGenerate *ctx, double *localTimeMs,
                                  double *remoteTimeMs, double *tokensPerSecond)
{
    if (!ctx)
        return;

    if (localTimeMs)
        *localTimeMs = ctx->lastLocalTimeMs;
    if (remoteTimeMs)
        *remoteTimeMs = ctx->lastRemoteTimeMs;

    if (tokensPerSecond) {
        double totalTimeMs = ctx->lastGenerationTimeMs;
        if (totalTimeMs > 0 && ctx->lastTokenCount > 0) {
            *tokensPerSecond = (ctx->lastTokenCount * 1000.0) / totalTimeMs;
        }
        else {
            *tokensPerSecond = 0;
        }
    }
}

bool hyperionHybridGenerateForceMode(HyperionHybridGenerate *ctx, bool forceRemote)
{
    if (!ctx)
        return false;

    if (forceRemote) {
        /* Check if remote is available */
        if (!ctx->mcpClient || !hyperionMcpIsAvailable(ctx->mcpClient)) {
            return false;
        }

        ctx->forceRemote = true;
        ctx->forceLocal  = false;
    }
    else {
        /* Check if local is available */
        if (!ctx->localModel) {
            return false;
        }

        ctx->forceLocal  = true;
        ctx->forceRemote = false;
    }

    return true;
}

bool hyperionHybridGenerateHasRemote(HyperionHybridGenerate *ctx)
{
    if (!ctx || !ctx->mcpClient)
        return false;
    return hyperionMcpIsAvailable(ctx->mcpClient);
}

bool hyperionHybridGenerateWouldUseRemote(HyperionHybridGenerate         *ctx,
                                        const HyperionGenerationParams *params)
{
    if (!ctx || !params)
        return false;
    return shouldUseRemote(ctx, params);
}