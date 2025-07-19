/**
 * @file document_processor.h
 * @brief Header for document processor in Hyperion
 *
 * This header defines the document processor API for Hyperion, which provides
 * document classification, summarization, and information extraction.
 */

#ifndef HYPERION_DOCUMENT_PROCESSOR_H
#define HYPERION_DOCUMENT_PROCESSOR_H

#include "../../models/text/generate.h"
#include "../../models/text/tokenizer.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Document processor modes
 */
typedef enum {
    HYPERION_DOC_MODE_CLASSIFY,    /* Document classification */
    HYPERION_DOC_MODE_SUMMARIZE,   /* Document summarization */
    HYPERION_DOC_MODE_EXTRACT_INFO /* Information extraction */
} HyperionDocumentProcessorMode;

/**
 * Document processor configuration
 */
typedef struct {
    HyperionDocumentProcessorMode mode;            /* Processing mode */
    const char                 *modelPath;       /* Path to model structure */
    const char                 *weightsPath;     /* Path to model weights */
    const char                 *tokenizerPath;   /* Path to tokenizer vocab */
    bool                        useQuantization; /* Whether to use 4-bit quantization */
    bool                        useSIMD;         /* Whether to use SIMD acceleration */
    int                         maxInputLength;  /* Maximum input document length in tokens */
    int                         maxOutputLength; /* Maximum output length in tokens */
    int                         numClasses;      /* Number of classes for classification mode */
    const char                **classLabels;     /* Class labels for classification mode */
} HyperionDocumentProcessorConfig;

/**
 * Document processor handle
 */
typedef struct HyperionDocumentProcessor HyperionDocumentProcessor;

/**
 * Classification result
 */
typedef struct {
    int         classId;    /* Class ID */
    float       confidence; /* Confidence score (0-1) */
    const char *label;      /* Class label (if available) */
} HyperionDocumentClassResult;

/**
 * Create a document processor
 *
 * @param config Configuration for the processor
 * @return New processor or NULL on error
 */
HyperionDocumentProcessor *hyperionDocumentProcessorCreate(const HyperionDocumentProcessorConfig *config);

/**
 * Free a document processor
 *
 * @param processor Processor to free
 */
void hyperionDocumentProcessorFree(HyperionDocumentProcessor *processor);

/**
 * Process a document file
 *
 * @param processor Processor to use
 * @param filePath Path to document file
 * @param outputBuffer Buffer to store output (must be pre-allocated)
 * @param outputSize Size of output buffer
 * @return True on success, false on failure
 */
bool hyperionDocumentProcessFile(HyperionDocumentProcessor *processor, const char *filePath,
                               char *outputBuffer, int outputSize);

/**
 * Process document text
 *
 * @param processor Processor to use
 * @param text Document text
 * @param outputBuffer Buffer to store output (must be pre-allocated)
 * @param outputSize Size of output buffer
 * @return True on success, false on failure
 */
bool hyperionDocumentProcessText(HyperionDocumentProcessor *processor, const char *text,
                               char *outputBuffer, int outputSize);

/**
 * Classify a document
 *
 * @param processor Processor to use
 * @param text Document text
 * @param results Array to store classification results (must be pre-allocated)
 * @param maxResults Maximum number of results to return
 * @return Number of results or -1 on error
 */
int hyperionDocumentClassify(HyperionDocumentProcessor *processor, const char *text,
                           HyperionDocumentClassResult *results, int maxResults);

/**
 * Summarize a document
 *
 * @param processor Processor to use
 * @param text Document text
 * @param summary Buffer to store summary (must be pre-allocated)
 * @param maxLength Maximum length of summary
 * @return True on success, false on failure
 */
bool hyperionDocumentSummarize(HyperionDocumentProcessor *processor, const char *text, char *summary,
                             int maxLength);

/**
 * Extract information from a document
 *
 * @param processor Processor to use
 * @param text Document text
 * @param prompt Specific information to extract (e.g., "Extract all dates")
 * @param result Buffer to store extracted information (must be pre-allocated)
 * @param maxLength Maximum length of result
 * @return True on success, false on failure
 */
bool hyperionDocumentExtractInfo(HyperionDocumentProcessor *processor, const char *text,
                               const char *prompt, char *result, int maxLength);

/**
 * Get memory usage statistics
 *
 * @param processor Processor to query
 * @param weightMemory Output parameter for weight memory (in bytes)
 * @param activationMemory Output parameter for activation memory (in bytes)
 * @return True on success, false on failure
 */
bool hyperionDocumentProcessorGetMemoryUsage(const HyperionDocumentProcessor *processor,
                                           size_t *weightMemory, size_t *activationMemory);

/**
 * Enable or disable SIMD acceleration
 *
 * @param processor Processor to configure
 * @param enable Whether to enable SIMD
 * @return True on success, false on failure
 */
bool hyperionDocumentProcessorEnableSIMD(HyperionDocumentProcessor *processor, bool enable);

#ifdef __cplusplus
}
#endif

#endif /* HYPERION_DOCUMENT_PROCESSOR_H */