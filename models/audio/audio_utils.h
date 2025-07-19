/**
 * @file audio_utils.h
 * @brief Audio utilities for Hyperion
 *
 * This header defines utility functions for audio processing in Hyperion,
 * including loading/saving audio files, format conversion, resampling,
 * and basic signal processing operations.
 */

#ifndef HYPERION_AUDIO_UTILS_H
#define HYPERION_AUDIO_UTILS_H

#include "audio_model.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Supported audio file formats
 */
typedef enum {
    HYPERION_AUDIO_FORMAT_WAV,  /* WAVE audio format */
    HYPERION_AUDIO_FORMAT_MP3,  /* MP3 audio format */
    HYPERION_AUDIO_FORMAT_FLAC, /* FLAC audio format */
    HYPERION_AUDIO_FORMAT_OGG,  /* OGG Vorbis audio format */
    HYPERION_AUDIO_FORMAT_RAW   /* Raw PCM data */
} HyperionAudioFileFormat;

/**
 * Audio playback state
 * Used for real-time audio processing
 */
typedef struct HyperionAudioPlayback HyperionAudioPlayback;

/**
 * Audio recording state
 * Used for real-time audio recording
 */
typedef struct HyperionAudioRecording HyperionAudioRecording;

/**
 * Detect audio file format from file extension
 * @param filePath Path to audio file
 * @return Detected file format, or HYPERION_AUDIO_FORMAT_WAV on failure
 */
HyperionAudioFileFormat hyperionAudioDetectFormat(const char *filePath);

/**
 * Load audio data from file
 * @param filePath Path to audio file
 * @param format Format of audio file (or -1 to detect from extension)
 * @param audio Output structure to receive audio data
 * @return true on success, false on failure
 */
bool hyperionAudioLoadFile(const char *filePath, HyperionAudioFileFormat format,
                         HyperionAudioData *audio);

/**
 * Save audio data to file
 * @param filePath Path to output file
 * @param format Format of output file
 * @param audio Audio data to save
 * @return true on success, false on failure
 */
bool hyperionAudioSaveFile(const char *filePath, HyperionAudioFileFormat format,
                         const HyperionAudioData *audio);

/**
 * Convert audio data between formats
 * @param input Input audio data
 * @param output Output structure to receive converted audio
 * @param targetFormat Target audio format
 * @return true on success, false on failure
 */
bool hyperionAudioConvertFormat(const HyperionAudioData *input, HyperionAudioData *output,
                              const HyperionAudioFormat *targetFormat);

/**
 * Resample audio data
 * @param input Input audio data
 * @param output Output structure to receive resampled audio
 * @param targetSampleRate Target sample rate in Hz
 * @return true on success, false on failure
 */
bool hyperionAudioResample(const HyperionAudioData *input, HyperionAudioData *output,
                         int targetSampleRate);

/**
 * Apply gain to audio data
 * @param audio Audio data to modify
 * @param gainDb Gain in decibels (positive = amplify, negative = attenuate)
 * @return true on success, false on failure
 */
bool hyperionAudioApplyGain(HyperionAudioData *audio, float gainDb);

/**
 * Normalize audio data to target level
 * @param audio Audio data to modify
 * @param targetLevelDb Target peak level in decibels (e.g., -3.0)
 * @return true on success, false on failure
 */
bool hyperionAudioNormalize(HyperionAudioData *audio, float targetLevelDb);

/**
 * Trim silence from start and end of audio
 * @param input Input audio data
 * @param output Output structure to receive trimmed audio
 * @param thresholdDb Silence threshold in decibels (e.g., -60.0)
 * @param minDurationMs Minimum silence duration to trim (in milliseconds)
 * @return true on success, false on failure
 */
bool hyperionAudioTrimSilence(const HyperionAudioData *input, HyperionAudioData *output,
                            float thresholdDb, int minDurationMs);

/**
 * Apply fade-in to audio
 * @param audio Audio data to modify
 * @param durationMs Duration of fade-in in milliseconds
 * @return true on success, false on failure
 */
bool hyperionAudioFadeIn(HyperionAudioData *audio, int durationMs);

/**
 * Apply fade-out to audio
 * @param audio Audio data to modify
 * @param durationMs Duration of fade-out in milliseconds
 * @return true on success, false on failure
 */
bool hyperionAudioFadeOut(HyperionAudioData *audio, int durationMs);

/**
 * Mix two audio streams
 * @param audio1 First audio stream
 * @param audio2 Second audio stream
 * @param output Output structure to receive mixed audio
 * @param mixRatio Mixing ratio (0.0 = only audio1, 1.0 = only audio2, 0.5 = equal mix)
 * @return true on success, false on failure
 */
bool hyperionAudioMix(const HyperionAudioData *audio1, const HyperionAudioData *audio2,
                    HyperionAudioData *output, float mixRatio);

/**
 * Apply band-pass filter to audio
 * @param input Input audio data
 * @param output Output structure to receive filtered audio
 * @param lowFreqHz Low cutoff frequency in Hz
 * @param highFreqHz High cutoff frequency in Hz
 * @param order Filter order (1, 2, 4, 8, etc.)
 * @return true on success, false on failure
 */
bool hyperionAudioBandpassFilter(const HyperionAudioData *input, HyperionAudioData *output,
                               float lowFreqHz, float highFreqHz, int order);

/**
 * Apply noise reduction to audio
 * @param input Input audio data
 * @param output Output structure to receive noise-reduced audio
 * @param strengthDb Noise reduction strength in decibels
 * @return true on success, false on failure
 */
bool hyperionAudioReduceNoise(const HyperionAudioData *input, HyperionAudioData *output,
                            float strengthDb);

/**
 * Create audio data from raw samples
 * @param samples Array of raw audio samples (float)
 * @param numSamples Number of samples
 * @param format Audio format information
 * @param audio Output structure to receive audio data
 * @return true on success, false on failure
 */
bool hyperionAudioCreateFromSamples(const float *samples, int numSamples,
                                  const HyperionAudioFormat *format, HyperionAudioData *audio);

/**
 * Convert audio samples to float array
 * @param audio Input audio data
 * @param samples Output array of float samples
 * @param maxSamples Maximum number of samples to convert
 * @param numSamples Output number of samples converted
 * @return true on success, false on failure
 */
bool hyperionAudioToFloatSamples(const HyperionAudioData *audio, float *samples, int maxSamples,
                               int *numSamples);

/**
 * Start audio playback
 * @param audio Audio data to play
 * @param loop Whether to loop playback
 * @return Playback state, or NULL on failure
 */
HyperionAudioPlayback *hyperionAudioStartPlayback(const HyperionAudioData *audio, bool loop);

/**
 * Stop audio playback
 * @param playback Playback state to stop
 * @return true on success, false on failure
 */
bool hyperionAudioStopPlayback(HyperionAudioPlayback *playback);

/**
 * Start audio recording
 * @param format Audio format for recording
 * @param maxDurationMs Maximum recording duration in milliseconds (0 for unlimited)
 * @return Recording state, or NULL on failure
 */
HyperionAudioRecording *hyperionAudioStartRecording(const HyperionAudioFormat *format, int maxDurationMs);

/**
 * Stop audio recording
 * @param recording Recording state to stop
 * @param audio Output structure to receive recorded audio
 * @return true on success, false on failure
 */
bool hyperionAudioStopRecording(HyperionAudioRecording *recording, HyperionAudioData *audio);

/**
 * Calculate root mean square (RMS) level of audio
 * @param audio Audio data to analyze
 * @param windowMs Window size in milliseconds (0 for entire audio)
 * @param rmsLevels Output array of RMS levels
 * @param maxLevels Maximum number of levels to calculate
 * @param numLevels Output number of levels calculated
 * @return true on success, false on failure
 */
bool hyperionAudioCalculateRMSLevel(const HyperionAudioData *audio, int windowMs, float *rmsLevels,
                                  int maxLevels, int *numLevels);

/**
 * Calculate peak level of audio
 * @param audio Audio data to analyze
 * @param windowMs Window size in milliseconds (0 for entire audio)
 * @param peakLevels Output array of peak levels
 * @param maxLevels Maximum number of levels to calculate
 * @param numLevels Output number of levels calculated
 * @return true on success, false on failure
 */
bool hyperionAudioCalculatePeakLevel(const HyperionAudioData *audio, int windowMs, float *peakLevels,
                                   int maxLevels, int *numLevels);

/**
 * Calculate zero-crossing rate of audio
 * @param audio Audio data to analyze
 * @param windowMs Window size in milliseconds (0 for entire audio)
 * @param rates Output array of zero-crossing rates
 * @param maxRates Maximum number of rates to calculate
 * @param numRates Output number of rates calculated
 * @return true on success, false on failure
 */
bool hyperionAudioCalculateZeroCrossingRate(const HyperionAudioData *audio, int windowMs, float *rates,
                                          int maxRates, int *numRates);

/**
 * Detect voice activity in audio
 * @param audio Audio data to analyze
 * @param windowMs Window size in milliseconds
 * @param activity Output array of voice activity (1.0 = voice, 0.0 = silence)
 * @param maxFrames Maximum number of frames to analyze
 * @param numFrames Output number of frames analyzed
 * @param sensitivity Sensitivity (0.0-1.0, higher = more sensitive)
 * @return true on success, false on failure
 */
bool hyperionAudioDetectVoiceActivity(const HyperionAudioData *audio, int windowMs, float *activity,
                                    int maxFrames, int *numFrames, float sensitivity);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_AUDIO_UTILS_H */