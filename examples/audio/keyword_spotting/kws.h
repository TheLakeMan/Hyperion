/**
 * @file kws.h
 * @brief Keyword Spotting for Hyperion
 *
 * This header defines the keyword spotting functionality
 * for Hyperion, providing a lightweight implementation for
 * detecting specific keywords in audio.
 */

#ifndef HYPERION_KWS_H
#define HYPERION_KWS_H

#include "../../../models/audio/audio_features.h"
#include "../../../models/audio/audio_model.h"
#include "../../../models/audio/audio_utils.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum number of supported keywords
 */
#define HYPERION_KWS_MAX_KEYWORDS 10

/**
 * Maximum keyword length in characters
 */
#define HYPERION_KWS_MAX_KEYWORD_LENGTH 32

/**
 * Default detection threshold
 */
#define HYPERION_KWS_DEFAULT_THRESHOLD 0.5f

/**
 * Keyword information structure
 */
typedef struct {
    char  word[HYPERION_KWS_MAX_KEYWORD_LENGTH]; /* Keyword text */
    float threshold;                           /* Detection threshold */
    int   modelIndex;                          /* Index in the model for this keyword */
} HyperionKWSKeyword;

/**
 * Keyword detection result
 */
typedef struct {
    int   keywordIndex; /* Index of detected keyword, or -1 if none */
    float confidence;   /* Detection confidence (0.0-1.0) */
    int   startFrame;   /* Start frame of detection */
    int   endFrame;     /* End frame of detection */
    float startTime;    /* Start time in seconds */
    float endTime;      /* End time in seconds */
} HyperionKWSDetection;

/**
 * Keyword spotting configuration
 */
typedef struct {
    float detectionThreshold;   /* Global detection threshold (0.0-1.0) */
    int   frameSize;            /* Frame size in milliseconds */
    int   frameShift;           /* Frame shift in milliseconds */
    bool  useDeltas;            /* Whether to use delta features */
    int   numMfccCoefficients;  /* Number of MFCC coefficients to use */
    int   numContextFrames;     /* Number of context frames (before/after) */
    bool  smoothDetections;     /* Whether to apply smoothing to detections */
    int   minDetectionDuration; /* Minimum detection duration in milliseconds */
    float noiseAdaptationRate;  /* Noise adaptation rate (0.0-1.0) */
} HyperionKWSConfig;

/**
 * Keyword spotting model
 */
typedef struct HyperionKWSModel HyperionKWSModel;

/**
 * Keyword spotting state
 */
typedef struct {
    HyperionKWSConfig  config;                            /* Configuration */
    HyperionKWSModel  *model;                             /* Model */
    int              numKeywords;                       /* Number of keywords */
    HyperionKWSKeyword keywords[HYPERION_KWS_MAX_KEYWORDS]; /* Keywords */

    /* Feature extraction */
    HyperionAudioFeaturesConfig featuresConfig;     /* Feature extraction configuration */
    HyperionAudioFeatures      *features;           /* Extracted features */
    float                    *featureBuffer;      /* Buffer for features */
    int                       featureBufferSize;  /* Size of feature buffer */
    int                       featureBufferIndex; /* Current index in feature buffer */

    /* Detection state */
    float *scores;           /* Detection scores */
    int    scoresSize;       /* Size of scores buffer */
    float *smoothedScores;   /* Smoothed detection scores */
    int   *activeDetections; /* Flags for active detections */
    float  noiseLevel;       /* Current noise level estimate */

    /* Results */
    HyperionKWSDetection *detections;     /* Detection results */
    int                 detectionsSize; /* Size of detections buffer */
    int                 numDetections;  /* Number of detections */
} HyperionKWSState;

/**
 * Initialize the default keyword spotting configuration
 * @param config Configuration structure to initialize
 */
void hyperionKWSInitConfig(HyperionKWSConfig *config);

/**
 * Create a new keyword spotting state
 * @param config Configuration
 * @param modelPath Path to model file
 * @return New keyword spotting state, or NULL on failure
 */
HyperionKWSState *hyperionKWSCreate(const HyperionKWSConfig *config, const char *modelPath);

/**
 * Free keyword spotting state
 * @param state State to free
 */
void hyperionKWSFree(HyperionKWSState *state);

/**
 * Reset keyword spotting state
 * @param state State to reset
 * @return true on success, false on failure
 */
bool hyperionKWSReset(HyperionKWSState *state);

/**
 * Add a keyword to detect
 * @param state Keyword spotting state
 * @param keyword Keyword text
 * @param threshold Detection threshold (0.0-1.0), or negative to use default
 * @return true on success, false on failure
 */
bool hyperionKWSAddKeyword(HyperionKWSState *state, const char *keyword, float threshold);

/**
 * Process audio frame for keyword detection
 * @param state Keyword spotting state
 * @param frame Audio frame (float samples)
 * @param frameSize Number of samples in frame
 * @return true on success, false on failure
 */
bool hyperionKWSProcessFrame(HyperionKWSState *state, const float *frame, int frameSize);

/**
 * Process full audio buffer for keyword detection
 * @param state Keyword spotting state
 * @param audio Audio data
 * @param detections Output array of detections (caller must free)
 * @param numDetections Output number of detections
 * @return true on success, false on failure
 */
bool hyperionKWSProcessAudio(HyperionKWSState *state, const HyperionAudioData *audio,
                           HyperionKWSDetection **detections, int *numDetections);

/**
 * Get current detection results
 * @param state Keyword spotting state
 * @param detections Output array of detections (not to be freed by caller)
 * @param maxDetections Maximum number of detections to return
 * @param numDetections Output number of detections
 * @return true on success, false on failure
 */
bool hyperionKWSGetDetections(HyperionKWSState *state, HyperionKWSDetection *detections,
                            int maxDetections, int *numDetections);

/**
 * Get list of available keywords in the model
 * @param state Keyword spotting state
 * @param keywords Output array of keywords (caller allocates)
 * @param maxKeywords Maximum number of keywords to return
 * @param numKeywords Output number of keywords
 * @return true on success, false on failure
 */
bool hyperionKWSGetAvailableKeywords(HyperionKWSState *state, char **keywords, int maxKeywords,
                                   int *numKeywords);

/**
 * Visualize keyword detection results
 * @param state Keyword spotting state
 * @param audio Audio data
 * @param detections Detection results
 * @param numDetections Number of detections
 * @param width Width of visualization in characters
 */
void hyperionKWSVisualizeDetections(HyperionKWSState *state, const HyperionAudioData *audio,
                                  const HyperionKWSDetection *detections, int numDetections,
                                  int width);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_KWS_H */