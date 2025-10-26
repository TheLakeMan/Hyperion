/**
 * Hyperion Android JNI Interface
 * 
 * Java Native Interface for integrating Hyperion AI into Android applications.
 * Provides Java/Kotlin bindings for the ultra-lightweight AI framework.
 */

#include <jni.h>
#include <android/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Include Hyperion core headers
#include "../../../core/memory.h"
#include "../../../core/config.h"
#include "../../../models/text/generate.h"
#include "../../../models/text/tokenizer.h"

// Android logging
#define LOG_TAG "HyperionAI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// Global state
static HyperionModel* g_model = NULL;
static HyperionTokenizer* g_tokenizer = NULL;
static int g_initialized = 0;

/**
 * Initialize Hyperion AI framework
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 * @param configPath Path to configuration file (can be null for defaults)
 * @return 0 on success, negative on error
 */
JNIEXPORT jint JNICALL
Java_com_hyperion_ai_HyperionAI_nativeInit(JNIEnv *env, jobject thiz, jstring configPath) {
    LOGI("Initializing Hyperion AI framework");
    
    // Check if already initialized
    if (g_initialized) {
        LOGD("Hyperion already initialized");
        return 0;
    }
    
    // Initialize memory management
    if (hyperionMemoryInit() != 0) {
        LOGE("Failed to initialize memory management");
        return -1;
    }
    
    // Initialize configuration
    if (hyperionConfigInit() != 0) {
        LOGE("Failed to initialize configuration");
        hyperionMemoryCleanup();
        return -2;
    }
    
    // Set mobile-specific configuration
    hyperionSetConfigInt("mobile_mode", 1);
    hyperionSetConfigInt("memory_limit_mb", 128);  // Conservative limit for mobile
    hyperionSetConfigInt("quantization_bits", 4);  // 4-bit quantization
    hyperionSetConfigFloat("temperature", 0.7f);
    
    // Handle custom config path
    if (configPath != NULL) {
        const char* path = (*env)->GetStringUTFChars(env, configPath, NULL);
        if (path != NULL) {
            LOGD("Loading config from: %s", path);
            hyperionLoadConfig(path);
            (*env)->ReleaseStringUTFChars(env, configPath, path);
        }
    }
    
    // Create tokenizer
    g_tokenizer = hyperionCreateTokenizer();
    if (!g_tokenizer) {
        LOGE("Failed to create tokenizer");
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        return -3;
    }
    
    // Create model
    g_model = hyperionCreateModel();
    if (!g_model) {
        LOGE("Failed to create model");
        hyperionDestroyTokenizer(g_tokenizer);
        g_tokenizer = NULL;
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        return -4;
    }
    
    // Initialize model with mobile-optimized configuration
    HyperionModelConfig modelConfig = {
        .vocabSize = 5000,      // Smaller vocabulary for mobile
        .hiddenSize = 128,      // Reduced hidden size
        .numLayers = 4,         // Fewer layers
        .maxSequenceLength = 256  // Shorter sequences
    };
    
    if (hyperionInitializeModel(g_model, &modelConfig) != 0) {
        LOGE("Failed to initialize model");
        hyperionDestroyModel(g_model);
        hyperionDestroyTokenizer(g_tokenizer);
        g_model = NULL;
        g_tokenizer = NULL;
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        return -5;
    }
    
    g_initialized = 1;
    LOGI("Hyperion AI initialized successfully");
    return 0;
}

/**
 * Generate text from a prompt
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 * @param prompt Input prompt string
 * @param maxTokens Maximum tokens to generate
 * @param temperature Sampling temperature (0.0 - 2.0)
 * @param topK Top-K sampling parameter
 * @return Generated text as Java string
 */
JNIEXPORT jstring JNICALL
Java_com_hyperion_ai_HyperionAI_nativeGenerateText(JNIEnv *env, jobject thiz, 
                                                   jstring prompt, jint maxTokens, 
                                                   jfloat temperature, jint topK) {
    if (!g_initialized || !g_model || !g_tokenizer) {
        LOGE("Hyperion not initialized");
        return (*env)->NewStringUTF(env, "");
    }
    
    // Get prompt string
    const char* promptStr = (*env)->GetStringUTFChars(env, prompt, NULL);
    if (!promptStr) {
        LOGE("Failed to get prompt string");
        return (*env)->NewStringUTF(env, "");
    }
    
    LOGD("Generating text for prompt: %.50s...", promptStr);
    LOGD("Parameters: maxTokens=%d, temperature=%.2f, topK=%d", maxTokens, temperature, topK);
    
    // Tokenize prompt
    int promptTokens[256];
    int promptLength = hyperionTokenize(g_tokenizer, promptStr, promptTokens, 256);
    
    if (promptLength <= 0) {
        LOGE("Failed to tokenize prompt");
        (*env)->ReleaseStringUTFChars(env, prompt, promptStr);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Prepare generation parameters
    HyperionGenerationParams params = {
        .maxTokens = (int)maxTokens,
        .temperature = temperature,
        .samplingMethod = HYPERION_SAMPLING_TOP_K,
        .topK = (int)topK,
        .topP = 0.9f,
        .promptTokens = promptTokens,
        .promptLength = promptLength,
        .seed = (unsigned int)time(NULL)
    };
    
    // Allocate output buffer
    int* outputTokens = malloc(maxTokens * sizeof(int));
    if (!outputTokens) {
        LOGE("Failed to allocate output buffer");
        (*env)->ReleaseStringUTFChars(env, prompt, promptStr);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Generate text
    int generatedTokens = hyperionGenerateText(g_model, &params, outputTokens, maxTokens);
    
    if (generatedTokens <= 0) {
        LOGE("Text generation failed");
        free(outputTokens);
        (*env)->ReleaseStringUTFChars(env, prompt, promptStr);
        return (*env)->NewStringUTF(env, "");
    }
    
    // Detokenize output
    char outputBuffer[4096];
    int outputLength = hyperionDetokenize(g_tokenizer, outputTokens, generatedTokens, 
                                         outputBuffer, sizeof(outputBuffer));
    
    free(outputTokens);
    (*env)->ReleaseStringUTFChars(env, prompt, promptStr);
    
    if (outputLength <= 0) {
        LOGE("Failed to detokenize output");
        return (*env)->NewStringUTF(env, "");
    }
    
    LOGD("Generated %d tokens successfully", generatedTokens);
    
    // Return Java string
    return (*env)->NewStringUTF(env, outputBuffer);
}

/**
 * Get memory usage statistics
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 * @return Memory usage in MB
 */
JNIEXPORT jfloat JNICALL
Java_com_hyperion_ai_HyperionAI_nativeGetMemoryUsage(JNIEnv *env, jobject thiz) {
    if (!g_initialized) {
        return 0.0f;
    }
    
    size_t memoryUsage = hyperionGetMemoryUsage();
    return (float)memoryUsage / (1024.0f * 1024.0f);  // Convert to MB
}

/**
 * Get performance statistics
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 * @return Performance stats as formatted string
 */
JNIEXPORT jstring JNICALL
Java_com_hyperion_ai_HyperionAI_nativeGetPerformanceStats(JNIEnv *env, jobject thiz) {
    if (!g_initialized) {
        return (*env)->NewStringUTF(env, "Not initialized");
    }
    
    char statsBuffer[512];
    snprintf(statsBuffer, sizeof(statsBuffer),
        "Memory: %.2f MB, Model: %s, Quantization: 4-bit",
        (float)hyperionGetMemoryUsage() / (1024.0f * 1024.0f),
        "Ultra-Light Mobile"
    );
    
    return (*env)->NewStringUTF(env, statsBuffer);
}

/**
 * Check if Hyperion is initialized
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 * @return true if initialized, false otherwise
 */
JNIEXPORT jboolean JNICALL
Java_com_hyperion_ai_HyperionAI_nativeIsInitialized(JNIEnv *env, jobject thiz) {
    return (jboolean)g_initialized;
}

/**
 * Set configuration parameter
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 * @param key Configuration key
 * @param value Configuration value
 */
JNIEXPORT void JNICALL
Java_com_hyperion_ai_HyperionAI_nativeSetConfig(JNIEnv *env, jobject thiz, 
                                                jstring key, jstring value) {
    if (!g_initialized) {
        LOGE("Cannot set config: Hyperion not initialized");
        return;
    }
    
    const char* keyStr = (*env)->GetStringUTFChars(env, key, NULL);
    const char* valueStr = (*env)->GetStringUTFChars(env, value, NULL);
    
    if (keyStr && valueStr) {
        LOGD("Setting config: %s = %s", keyStr, valueStr);
        hyperionSetConfigString(keyStr, valueStr);
    }
    
    if (keyStr) (*env)->ReleaseStringUTFChars(env, key, keyStr);
    if (valueStr) (*env)->ReleaseStringUTFChars(env, value, valueStr);
}

/**
 * Cleanup Hyperion resources
 * 
 * @param env JNI environment
 * @param thiz Java object reference
 */
JNIEXPORT void JNICALL
Java_com_hyperion_ai_HyperionAI_nativeCleanup(JNIEnv *env, jobject thiz) {
    if (!g_initialized) {
        LOGD("Hyperion already cleaned up");
        return;
    }
    
    LOGI("Cleaning up Hyperion AI framework");
    
    if (g_model) {
        hyperionDestroyModel(g_model);
        g_model = NULL;
    }
    
    if (g_tokenizer) {
        hyperionDestroyTokenizer(g_tokenizer);
        g_tokenizer = NULL;
    }
    
    hyperionConfigCleanup();
    hyperionMemoryCleanup();
    
    g_initialized = 0;
    LOGI("Hyperion AI cleanup completed");
}

/**
 * JNI library load callback
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGI("Hyperion AI native library loaded");
    return JNI_VERSION_1_6;
}

/**
 * JNI library unload callback
 */
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGI("Hyperion AI native library unloaded");
    
    // Ensure cleanup
    if (g_initialized) {
        if (g_model) {
            hyperionDestroyModel(g_model);
            g_model = NULL;
        }
        
        if (g_tokenizer) {
            hyperionDestroyTokenizer(g_tokenizer);
            g_tokenizer = NULL;
        }
        
        hyperionConfigCleanup();
        hyperionMemoryCleanup();
        g_initialized = 0;
    }
}