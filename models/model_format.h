/**
 * @file model_format.h
 * @brief Definition of the canonical Hyperion model binary format.
 */

#ifndef HYPERION_MODEL_FORMAT_H
#define HYPERION_MODEL_FORMAT_H

#include <stdint.h>
#include <stddef.h>

#define HYPERION_MODEL_FORMAT_MAGIC 0x484D4F44u /* 'HMOD' */
#define HYPERION_MODEL_FORMAT_VERSION_MAJOR 1
#define HYPERION_MODEL_FORMAT_VERSION_MINOR 0

typedef enum {
    HYPERION_MODEL_DOMAIN_TEXT       = 1,
    HYPERION_MODEL_DOMAIN_AUDIO      = 2,
    HYPERION_MODEL_DOMAIN_IMAGE      = 3,
    HYPERION_MODEL_DOMAIN_MULTIMODAL = 4
} HyperionModelDomain;

typedef enum {
    HYPERION_MODEL_QUANT_UNKNOWN = 0,
    HYPERION_MODEL_QUANT_FP32    = 32,
    HYPERION_MODEL_QUANT_INT8    = 8,
    HYPERION_MODEL_QUANT_INT4    = 4
} HyperionModelQuantization;

typedef enum {
    HYPERION_MODEL_CAP_TEXT_GENERATION = 1u << 0,
    HYPERION_MODEL_CAP_TEXT_EMBEDDING  = 1u << 1,
    HYPERION_MODEL_CAP_AUDIO           = 1u << 2,
    HYPERION_MODEL_CAP_VISION          = 1u << 3,
    HYPERION_MODEL_CAP_REASONING       = 1u << 4,
    HYPERION_MODEL_CAP_HYBRID_READY    = 1u << 5
} HyperionModelCapability;

typedef struct {
    uint32_t magic;          /* Magic value HYPERION_MODEL_FORMAT_MAGIC */
    uint16_t versionMajor;   /* Format major version */
    uint16_t versionMinor;   /* Format minor version */
    uint32_t domain;         /* HyperionModelDomain */
    uint32_t quantization;   /* HyperionModelQuantization */
    uint64_t parameterCount; /* Number of parameters */
    uint32_t metadataLength; /* Length of metadata block in bytes */
    uint32_t capabilities;   /* HyperionModelCapability bitmask */
    uint64_t weightsLength;  /* Length of weights block in bytes */
    uint32_t checksum;       /* CRC32 of metadata and weights blocks */
    uint32_t reserved;       /* Reserved for future use */
} HyperionModelHeader;

typedef struct {
    char     modelName[64];
    char     author[64];
    char     description[128];
    uint32_t vocabSize;
    uint32_t contextWindow;
    uint32_t embeddingSize;
    uint32_t reserved[10];
} HyperionModelMetadata;

typedef enum {
    HYPERION_MODEL_META_RESERVED_LAYER_COUNT = 0,
    HYPERION_MODEL_META_RESERVED_HIDDEN_SIZE = 1,
    HYPERION_MODEL_META_RESERVED_CONTEXT_SIZE = 2,
    HYPERION_MODEL_META_RESERVED_BLOCK_COUNT = 3,
    HYPERION_MODEL_META_RESERVED_EXTRA_FLAGS = 4
} HyperionModelMetadataReservedIndex;

typedef struct {
    HyperionModelHeader   header;
    HyperionModelMetadata metadata;
} HyperionModelInfo;

typedef struct {
    int     success;          /* 1 if verification succeeded */
    int     checksumMismatch; /* 1 if checksum mismatch */
    int     versionMismatch;  /* 1 if unsupported version */
    int     invalidMagic;     /* 1 if magic invalid */
} HyperionModelVerification;

int hyperionModelWrite(const char *path, const HyperionModelHeader *header,
                       const HyperionModelMetadata *metadata, const void *weights,
                       size_t weightsLength);

int hyperionModelRead(const char *path, HyperionModelInfo *info, void **weightsOut,
                      size_t *weightsLength);

int hyperionModelVerify(const char *path, HyperionModelVerification *result);

uint32_t hyperionModelChecksum(const HyperionModelHeader *header,
                               const HyperionModelMetadata *metadata, const void *weights,
                               size_t weightsLength);

#endif /* HYPERION_MODEL_FORMAT_H */
