/**
 * @file test_audio_model.c
 * @brief Test program for audio model functionality in Hyperion
 */

#include "../models/audio/audio_features.h"
#include "../models/audio/audio_model.h"
#include "../models/audio/audio_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * Create a sample audio file for testing
 * @param filename Output file path
 * @param durationMs Duration in milliseconds
 * @param frequency Tone frequency in Hz
 * @return true on success, false on failure
 */
static bool createSampleAudio(const char *filename, int durationMs, float frequency)
{
    int    sampleRate = 16000; /* 16kHz sample rate */
    int    numSamples = (int)(durationMs * sampleRate / 1000);
    float *samples    = (float *)malloc(numSamples * sizeof(float));
    if (!samples) {
        fprintf(stderr, "Failed to allocate sample buffer\n");
        return false;
    }

    /* Generate a simple sine wave */
    for (int i = 0; i < numSamples; i++) {
        float t    = (float)i / sampleRate;
        samples[i] = 0.5f * sinf(2.0f * 3.14159f * frequency * t);
    }

    /* Create audio data */
    HyperionAudioData   audio;
    HyperionAudioFormat format;
    format.sampleRate    = sampleRate;
    format.channels      = 1;  /* Mono */
    format.bitsPerSample = 16; /* 16-bit */

    if (!hyperionAudioCreateFromSamples(samples, numSamples, &format, &audio)) {
        fprintf(stderr, "Failed to create audio data\n");
        free(samples);
        return false;
    }

    /* Save audio to file */
    bool result = hyperionAudioSaveFile(filename, TINYAI_AUDIO_FORMAT_WAV, &audio);

    /* Clean up */
    hyperionAudioDataFree(&audio);
    free(samples);

    return result;
}

/**
 * Test audio feature extraction
 * @return true on success, false on failure
 */
static bool testAudioFeatures()
{
    printf("Testing audio feature extraction...\n");

    /* Create a sample audio file */
    const char *sampleFile = "test_audio.wav";
    if (!createSampleAudio(sampleFile, 2000, 440.0f)) {
        fprintf(stderr, "Failed to create sample audio\n");
        return false;
    }

    /* Load audio file */
    HyperionAudioData audio;
    if (!hyperionAudioLoadFile(sampleFile, TINYAI_AUDIO_FORMAT_WAV, &audio)) {
        fprintf(stderr, "Failed to load audio file\n");
        return false;
    }

    /* Set up feature extraction configuration */
    HyperionAudioFeaturesConfig config;
    memset(&config, 0, sizeof(config));
    config.type              = TINYAI_AUDIO_FEATURES_MFCC;
    config.frameLength       = 400; /* 25ms at 16kHz */
    config.frameShift        = 160; /* 10ms at 16kHz */
    config.numFilters        = 26;
    config.numCoefficients   = 13;
    config.includeDelta      = true;
    config.includeDeltaDelta = false;

    /* Set up advanced options */
    HyperionAudioFeaturesAdvancedOptions options;
    hyperionAudioFeaturesInitAdvancedOptions(&options);

    /* Extract features */
    HyperionAudioFeatures features;
    if (!hyperionAudioExtractMFCC(&audio, &config, &options, &features)) {
        fprintf(stderr, "Failed to extract MFCC features\n");
        hyperionAudioDataFree(&audio);
        return false;
    }

    /* Print feature dimensions */
    printf("Extracted features: %d frames x %d coefficients\n", features.numFrames,
           features.numFeatures);

    /* Clean up */
    hyperionAudioFeaturesFree(&features);
    hyperionAudioDataFree(&audio);

    printf("Audio feature extraction test passed!\n");
    return true;
}

/**
 * Test audio model creation and processing
 * @return true on success, false on failure
 */
static bool testAudioModel()
{
    printf("Testing audio model creation and processing...\n");

    /* Create a sample audio file */
    const char *sampleFile = "test_audio.wav";
    if (!createSampleAudio(sampleFile, 2000, 440.0f)) {
        fprintf(stderr, "Failed to create sample audio\n");
        return false;
    }

    /* Load audio file */
    HyperionAudioData audio;
    if (!hyperionAudioLoadFile(sampleFile, TINYAI_AUDIO_FORMAT_WAV, &audio)) {
        fprintf(stderr, "Failed to load audio file\n");
        return false;
    }

    /* Set up model configuration */
    HyperionAudioModelConfig config;
    memset(&config, 0, sizeof(config));

    /* Feature configuration */
    config.featuresConfig.type              = TINYAI_AUDIO_FEATURES_MFCC;
    config.featuresConfig.frameLength       = 400; /* 25ms at 16kHz */
    config.featuresConfig.frameShift        = 160; /* 10ms at 16kHz */
    config.featuresConfig.numFilters        = 26;
    config.featuresConfig.numCoefficients   = 13;
    config.featuresConfig.includeDelta      = true;
    config.featuresConfig.includeDeltaDelta = false;

    /* Model architecture */
    config.hiddenSize          = 64;
    config.numLayers           = 2;
    config.numClasses          = 10; /* Example: 10 speech commands */
    config.use4BitQuantization = true;
    config.useSIMD             = true;
    config.weightsFile         = NULL; /* Initialize with random weights */

    /* Create audio model */
    HyperionAudioModel *model = hyperionAudioModelCreate(&config);
    if (!model) {
        fprintf(stderr, "Failed to create audio model\n");
        hyperionAudioDataFree(&audio);
        return false;
    }

    /* Initialize output structure */
    HyperionAudioModelOutput output;
    if (!hyperionAudioModelOutputInit(&output, config.numClasses)) {
        fprintf(stderr, "Failed to initialize audio model output\n");
        hyperionAudioModelFree(model);
        hyperionAudioDataFree(&audio);
        return false;
    }

    /* Process audio with model */
    if (!hyperionAudioModelProcess(model, &audio, &output)) {
        fprintf(stderr, "Failed to process audio with model\n");
        hyperionAudioModelOutputFree(&output);
        hyperionAudioModelFree(model);
        hyperionAudioDataFree(&audio);
        return false;
    }

    /* Print results */
    printf("Model prediction: class %d with confidence %.2f%%\n", output.predictedClass,
           output.confidence * 100.0f);

    /* Print top 3 probabilities */
    printf("Top probabilities:\n");
    for (int i = 0; i < 3 && i < config.numClasses; i++) {
        int   maxIdx  = 0;
        float maxProb = output.probabilities[0];

        /* Find max probability among remaining classes */
        for (int j = 1; j < config.numClasses; j++) {
            if (output.probabilities[j] > maxProb) {
                maxIdx  = j;
                maxProb = output.probabilities[j];
            }
        }

        printf("  Class %d: %.2f%%\n", maxIdx, maxProb * 100.0f);

        /* Set this probability to zero so we find the next highest */
        output.probabilities[maxIdx] = 0.0f;
    }

    /* Clean up */
    hyperionAudioModelOutputFree(&output);
    hyperionAudioModelFree(model);
    hyperionAudioDataFree(&audio);

    printf("Audio model test passed!\n");
    return true;
}

/**
 * Main test function
 */
int main(int argc, char *argv[])
{
    printf("Hyperion Audio Model Tests\n");
    printf("=========================\n");

    /* Seed random number generator */
    srand((unsigned int)time(NULL));

    /* Run tests */
    bool featuresResult = testAudioFeatures();
    bool modelResult    = testAudioModel();

    /* Print overall result */
    printf("\nTest Results:\n");
    printf("  Audio Features: %s\n", featuresResult ? "PASSED" : "FAILED");
    printf("  Audio Model: %s\n", modelResult ? "PASSED" : "FAILED");

    return (featuresResult && modelResult) ? 0 : 1;
}
