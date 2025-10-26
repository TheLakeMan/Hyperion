/**
 * @file audio_model.h
 * @brief Audio model interface for Hyperion
 *
 * This header defines the interface for audio models in Hyperion,
 * including structure definitions and function declarations for
 * audio classification, keyword spotting, and feature extraction.
 */

#ifndef HYPERION_AUDIO_MODEL_H
#define HYPERION_AUDIO_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Audio format configuration
 */
typedef struct {
    int sampleRate;    /* Sample rate in Hz (e.g., 16000) */
    int channels;      /* Number of audio channels (1 for mono, 2 for stereo) */
    int bitsPerSample; /* Bits per sample (8, 16, 24, etc.) */
} HyperionAudioFormat;

/**
 * Raw audio data
 */
typedef struct {
    void             *data;       /* Pointer to raw audio samples */
    size_t            dataSize;   /* Size of audio data in bytes */
    HyperionAudioFormat format;     /* Audio format information */
    int               durationMs; /* Duration in milliseconds */
} HyperionAudioData;

/**
 * Audio feature extraction type
 */
typedef enum {
    HYPERION_AUDIO_FEATURES_MFCC,        /* Mel-frequency cepstral coefficients */
    HYPERION_AUDIO_FEATURES_MEL,         /* Mel spectrogram */
    HYPERION_AUDIO_FEATURES_SPECTROGRAM, /* Regular spectrogram */
    HYPERION_AUDIO_FEATURES_RAW          /* Raw waveform (no feature extraction) */
} HyperionAudioFeaturesType;

/**
 * Audio feature extraction configuration
 */
typedef struct {
    HyperionAudioFeaturesType type;              /* Type of features to extract */
    int                     frameLength;       /* Frame length in samples */
    int                     frameShift;        /* Frame shift in samples */
    int                     numFilters;        /* Number of mel filters (for MFCC or MEL) */
    int                     numCoefficients;   /* Number of coefficients (for MFCC) */
    bool                    includeDelta;      /* Whether to include delta features */
    bool                    includeDeltaDelta; /* Whether to include delta-delta features */
    bool                    useLogMel;         /* Whether to use log-mel features */
    float                   preEmphasis;       /* Pre-emphasis coefficient (0.0 to disable) */
} HyperionAudioFeaturesConfig;

/**
 * Extracted audio features
 */
typedef struct {
    float                  *data;        /* Pointer to feature data */
    size_t                  dataSize;    /* Size of feature data in bytes */
    int                     numFrames;   /* Number of frames */
    int                     numFeatures; /* Number of features per frame */
    HyperionAudioFeaturesType type;        /* Type of features */
} HyperionAudioFeatures;

/**
 * Audio model configuration
 */
typedef struct {
    HyperionAudioFeaturesConfig featuresConfig;      /* Feature extraction configuration */
    int                       hiddenSize;          /* Size of hidden layers */
    int                       numLayers;           /* Number of model layers */
    int                       numClasses;          /* Number of output classes */
    bool                      use4BitQuantization; /* Whether to use 4-bit quantization */
    bool                      useSIMD;             /* Whether to use SIMD acceleration */
    const char               *weightsFile;         /* Path to weights file (NULL for random) */
} HyperionAudioModelConfig;

/**
 * Audio model output
 */
typedef struct {
    float *logits;         /* Raw model output logits */
    float *probabilities;  /* Softmax probabilities */
    int    predictedClass; /* Index of highest probability class */
    float  confidence;     /* Confidence of prediction (0.0-1.0) */
} HyperionAudioModelOutput;

/**
 * Audio model structure
 * Opaque pointer to hide implementation details
 */
typedef struct HyperionAudioModel HyperionAudioModel;

/**
 * Create an audio model
 * @param config Configuration for the audio model
 * @return New audio model, or NULL on failure
 */
HyperionAudioModel *hyperionAudioModelCreate(const HyperionAudioModelConfig *config);

/**
 * Free an audio model
 * @param model The model to free
 */
void hyperionAudioModelFree(HyperionAudioModel *model);

/**
 * Process audio data with the model
 * @param model The audio model to use
 * @param audio The audio data to process
 * @param output The output structure to fill
 * @return true on success, false on failure
 */
bool hyperionAudioModelProcess(HyperionAudioModel *model, const HyperionAudioData *audio,
                             HyperionAudioModelOutput *output);

/**
 * Extract features from audio data
 * @param audio The audio data to process
 * @param config Feature extraction configuration
 * @param features Output structure to receive extracted features
 * @return true on success, false on failure
 */
bool hyperionAudioFeaturesExtract(const HyperionAudioData           *audio,
                                const HyperionAudioFeaturesConfig *config,
                                HyperionAudioFeatures             *features);

/**
 * Free audio features
 * @param features The features to free
 */
void hyperionAudioFeaturesFree(HyperionAudioFeatures *features);

/**
 * Load audio data from a file
 * @param path Path to the audio file
 * @param audio Output structure to receive audio data
 * @return true on success, false on failure
 */
bool hyperionAudioDataLoad(const char *path, HyperionAudioData *audio);

/**
 * Free audio data
 * @param audio The audio data to free
 */
void hyperionAudioDataFree(HyperionAudioData *audio);

/**
 * Initialize audio model output structure
 * @param output The output structure to initialize
 * @param numClasses Number of output classes
 * @return true on success, false on failure
 */
bool hyperionAudioModelOutputInit(HyperionAudioModelOutput *output, int numClasses);

/**
 * Free audio model output
 * @param output The output to free
 */
void hyperionAudioModelOutputFree(HyperionAudioModelOutput *output);

/**
 * Enable SIMD acceleration for audio model
 * @param model The model to configure
 * @param enable Whether to enable SIMD
 * @return true on success, false on failure
 */
bool hyperionAudioModelEnableSIMD(HyperionAudioModel *model, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_AUDIO_MODEL_H */