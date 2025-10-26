#include "test_framework.h"
#include "../models/model_format.h"
#include "../core/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void buildTestWeights(uint8_t *buffer, size_t bufferSize, size_t *outLength)
{
    (void)bufferSize;

    uint8_t *cursor = buffer;

    uint32_t modelType   = 1; /* transformer */
    uint32_t layerCount  = 1;
    uint32_t hiddenSize  = 2;
    uint32_t contextSize = 16;

    memcpy(cursor, &modelType, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &layerCount, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &hiddenSize, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &contextSize, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    uint32_t layerType    = 1; /* dense */
    uint32_t inputSize    = 2;
    uint32_t outputSize   = 2;
    uint32_t activation   = 1; /* relu */
    float    scale        = 0.5f;
    float    zeroPoint    = 0.0f;
    uint32_t weightBytes  = 2;
    uint32_t biasBytes    = outputSize * sizeof(float);
    uint8_t  weightData[2] = {0x12, 0x34};
    float    biasData[2]    = {0.1f, 0.2f};

    memcpy(cursor, &layerType, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &inputSize, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &outputSize, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &activation, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &scale, sizeof(float));
    cursor += sizeof(float);
    memcpy(cursor, &zeroPoint, sizeof(float));
    cursor += sizeof(float);
    memcpy(cursor, &weightBytes, sizeof(uint32_t));
    cursor += sizeof(uint32_t);
    memcpy(cursor, &biasBytes, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    memcpy(cursor, weightData, sizeof(weightData));
    cursor += sizeof(weightData);

    memcpy(cursor, biasData, sizeof(biasData));
    cursor += sizeof(biasData);

    *outLength = (size_t)(cursor - buffer);
}

static void makeTempPath(char *buffer, size_t size, const char *hint)
{
    snprintf(buffer, size, "hyperion_%s_%ld.tmp", hint, (long)rand());
}

HYPERION_TEST(test_model_format_roundtrip)
{
    HyperionModelHeader header;
    memset(&header, 0, sizeof(header));
    header.versionMajor   = HYPERION_MODEL_FORMAT_VERSION_MAJOR;
    header.versionMinor   = HYPERION_MODEL_FORMAT_VERSION_MINOR;
    header.domain         = HYPERION_MODEL_DOMAIN_TEXT;
    header.quantization   = HYPERION_MODEL_QUANT_INT4;
    header.parameterCount = 4;
    header.metadataLength = sizeof(HyperionModelMetadata);
    header.capabilities   = HYPERION_MODEL_CAP_TEXT_GENERATION;

    HyperionModelMetadata metadata;
    memset(&metadata, 0, sizeof(metadata));
    strcpy(metadata.modelName, "Test Tiny");
    strcpy(metadata.author, "Factory");
    metadata.vocabSize = 8192;
    metadata.contextWindow = 16;
    metadata.embeddingSize = 2;
    metadata.reserved[HYPERION_MODEL_META_RESERVED_LAYER_COUNT] = 1;
    metadata.reserved[HYPERION_MODEL_META_RESERVED_HIDDEN_SIZE] = 2;
    metadata.reserved[HYPERION_MODEL_META_RESERVED_CONTEXT_SIZE] = 16;

    uint8_t weightsBuffer[256];
    size_t  weightsLength = 0;
    buildTestWeights(weightsBuffer, sizeof(weightsBuffer), &weightsLength);

    char packagePath[128];
    makeTempPath(packagePath, sizeof(packagePath), "model");

    HYPERION_ASSERT(hyperionModelWrite(packagePath, &header, &metadata, weightsBuffer,
                                       weightsLength) == 0,
                    "Failed to write model package");

    HyperionModelVerification verification;
    HYPERION_ASSERT(hyperionModelVerify(packagePath, &verification) == 0,
                    "Verification routine failed");
    HYPERION_ASSERT(verification.success == 1, "Model verification should succeed");

    HyperionModelInfo info;
    void             *loadedWeights      = NULL;
    size_t            loadedWeightsBytes = 0;
    HYPERION_ASSERT(hyperionModelRead(packagePath, &info, &loadedWeights, &loadedWeightsBytes) == 0,
                    "Model read should succeed");
    HYPERION_ASSERT(info.header.domain == header.domain, "Domain mismatch");
    HYPERION_ASSERT(info.metadata.vocabSize == metadata.vocabSize, "Metadata mismatch");
    HYPERION_ASSERT(loadedWeightsBytes == weightsLength, "Weights length mismatch");
    HYPERION_ASSERT(memcmp(loadedWeights, weightsBuffer, weightsLength) == 0,
                    "Weights content mismatch");

    HYPERION_FREE(loadedWeights);

    remove(packagePath);

    return 0;
}

HYPERION_TEST(test_model_format_invalid_magic)
{
    HyperionModelHeader header;
    memset(&header, 0, sizeof(header));
    header.versionMajor   = HYPERION_MODEL_FORMAT_VERSION_MAJOR;
    header.versionMinor   = HYPERION_MODEL_FORMAT_VERSION_MINOR;
    header.domain         = HYPERION_MODEL_DOMAIN_TEXT;
    header.quantization   = HYPERION_MODEL_QUANT_INT4;
    header.parameterCount = 4;
    header.metadataLength = sizeof(HyperionModelMetadata);
    header.capabilities   = HYPERION_MODEL_CAP_TEXT_GENERATION;

    HyperionModelMetadata metadata;
    memset(&metadata, 0, sizeof(metadata));

    uint8_t weightsBuffer[64];
    size_t  weightsLength = 0;
    buildTestWeights(weightsBuffer, sizeof(weightsBuffer), &weightsLength);

    char packagePath[128];
    makeTempPath(packagePath, sizeof(packagePath), "invalid_magic");

    HYPERION_ASSERT(hyperionModelWrite(packagePath, &header, &metadata, weightsBuffer,
                                       weightsLength) == 0,
                    "Failed to write baseline model package");

    FILE *file = fopen(packagePath, "r+b");
    HYPERION_ASSERT(file != NULL, "Failed to open package for corruption");
    uint32_t badMagic = 0x0u;
    fwrite(&badMagic, sizeof(badMagic), 1, file);
    fclose(file);

    HyperionModelVerification verification;
    memset(&verification, 0, sizeof(verification));
    HYPERION_ASSERT(hyperionModelVerify(packagePath, &verification) == 0,
                    "Verification routine failed on invalid magic");
    HYPERION_ASSERT(verification.invalidMagic == 1, "Invalid magic should be reported");
    HYPERION_ASSERT(verification.success == 0, "Verification should fail for invalid magic");

    remove(packagePath);

    return 0;
}

HYPERION_TEST(test_model_format_version_mismatch)
{
    HyperionModelHeader header;
    memset(&header, 0, sizeof(header));
    header.versionMajor   = HYPERION_MODEL_FORMAT_VERSION_MAJOR + 1;
    header.versionMinor   = HYPERION_MODEL_FORMAT_VERSION_MINOR;
    header.domain         = HYPERION_MODEL_DOMAIN_TEXT;
    header.quantization   = HYPERION_MODEL_QUANT_INT4;
    header.parameterCount = 4;
    header.metadataLength = sizeof(HyperionModelMetadata);
    header.capabilities   = HYPERION_MODEL_CAP_TEXT_GENERATION;

    HyperionModelMetadata metadata;
    memset(&metadata, 0, sizeof(metadata));

    uint8_t weightsBuffer[64];
    size_t  weightsLength = 0;
    buildTestWeights(weightsBuffer, sizeof(weightsBuffer), &weightsLength);

    char packagePath[128];
    makeTempPath(packagePath, sizeof(packagePath), "version_mismatch");

    HYPERION_ASSERT(hyperionModelWrite(packagePath, &header, &metadata, weightsBuffer,
                                       weightsLength) == 0,
                    "Failed to write model with future version");

    HyperionModelVerification verification;
    memset(&verification, 0, sizeof(verification));
    HYPERION_ASSERT(hyperionModelVerify(packagePath, &verification) == 0,
                    "Verification routine failed on version mismatch");
    HYPERION_ASSERT(verification.versionMismatch == 1,
                    "Version mismatch should be reported");
    HYPERION_ASSERT(verification.success == 0, "Verification should fail for version mismatch");

    remove(packagePath);

    return 0;
}

HYPERION_TEST(test_model_format_checksum_mismatch)
{
    HyperionModelHeader header;
    memset(&header, 0, sizeof(header));
    header.versionMajor   = HYPERION_MODEL_FORMAT_VERSION_MAJOR;
    header.versionMinor   = HYPERION_MODEL_FORMAT_VERSION_MINOR;
    header.domain         = HYPERION_MODEL_DOMAIN_TEXT;
    header.quantization   = HYPERION_MODEL_QUANT_INT4;
    header.parameterCount = 4;
    header.metadataLength = sizeof(HyperionModelMetadata);
    header.capabilities   = HYPERION_MODEL_CAP_TEXT_GENERATION;

    HyperionModelMetadata metadata;
    memset(&metadata, 0, sizeof(metadata));

    uint8_t weightsBuffer[64];
    size_t  weightsLength = 0;
    buildTestWeights(weightsBuffer, sizeof(weightsBuffer), &weightsLength);

    char packagePath[128];
    makeTempPath(packagePath, sizeof(packagePath), "checksum_mismatch");

    HYPERION_ASSERT(hyperionModelWrite(packagePath, &header, &metadata, weightsBuffer,
                                       weightsLength) == 0,
                    "Failed to write model for checksum test");

    FILE *file = fopen(packagePath, "r+b");
    HYPERION_ASSERT(file != NULL, "Failed to open package for checksum corruption");
    fseek(file, -(long)sizeof(uint8_t), SEEK_END);
    uint8_t byte;
    fread(&byte, sizeof(byte), 1, file);
    fseek(file, -(long)sizeof(uint8_t), SEEK_END);
    byte ^= 0xFFu;
    fwrite(&byte, sizeof(byte), 1, file);
    fclose(file);

    HyperionModelVerification verification;
    memset(&verification, 0, sizeof(verification));
    HYPERION_ASSERT(hyperionModelVerify(packagePath, &verification) == 0,
                    "Verification routine failed on checksum mismatch");
    HYPERION_ASSERT(verification.checksumMismatch == 1,
                    "Checksum mismatch should be reported");
    HYPERION_ASSERT(verification.success == 0,
                    "Verification should fail for checksum mismatch");

    remove(packagePath);

    return 0;
}

const HyperionTestCase g_model_format_tests[] = {
    {"model_format_roundtrip", "core", test_model_format_roundtrip},
    {"model_format_invalid_magic", "core", test_model_format_invalid_magic},
    {"model_format_version_mismatch", "core", test_model_format_version_mismatch},
    {"model_format_checksum_mismatch", "core", test_model_format_checksum_mismatch},
};

const size_t g_model_format_test_count = sizeof(g_model_format_tests) / sizeof(g_model_format_tests[0]);
