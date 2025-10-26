/**
 * @file model_format.c
 * @brief Implementation of the canonical Hyperion model binary format.
 */

#include "model_format.h"
#include "../core/memory.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HYPERION_MODEL_IO_CHUNK 4096

static uint32_t crc32_table[256];
static int      crc32_initialized = 0;

static void crc32_init(void)
{
    if (crc32_initialized) {
        return;
    }

    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            }
            else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }

    crc32_initialized = 1;
}

static uint32_t crc32_begin(void)
{
    crc32_init();
    return 0xFFFFFFFFu;
}

static uint32_t crc32_update(uint32_t crc, const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    for (size_t i = 0; i < length; ++i) {
        crc = crc32_table[(crc ^ bytes[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc;
}

static uint32_t crc32_finish(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFu;
}

uint32_t hyperionModelChecksum(const HyperionModelHeader *header,
                               const HyperionModelMetadata *metadata, const void *weights,
                               size_t weightsLength)
{
    if (!header) {
        return 0;
    }

    uint32_t crc = crc32_begin();

    if (header->metadataLength > 0) {
        size_t metaLen    = header->metadataLength;
        size_t copyLength = 0;

        if (metadata) {
            copyLength = metaLen < sizeof(*metadata) ? metaLen : sizeof(*metadata);
            if (copyLength > 0) {
                crc = crc32_update(crc, metadata, copyLength);
            }
        }

        size_t remaining = metaLen - copyLength;
        if (remaining > 0) {
            uint8_t zeroBlock[64] = {0};
            while (remaining > 0) {
                size_t chunk = remaining < sizeof(zeroBlock) ? remaining : sizeof(zeroBlock);
                crc          = crc32_update(crc, zeroBlock, chunk);
                remaining -= chunk;
            }
        }
    }

    if (weights && weightsLength > 0) {
        crc = crc32_update(crc, weights, weightsLength);
    }

    return crc32_finish(crc);
}

static int write_block(FILE *file, const void *data, size_t length)
{
    if (length == 0 || !data) {
        return 0;
    }

    if (fwrite(data, 1, length, file) != length) {
        return -1;
    }
    return 0;
}

int hyperionModelWrite(const char *path, const HyperionModelHeader *header,
                       const HyperionModelMetadata *metadata, const void *weights,
                       size_t weightsLength)
{
    if (!path || !header) {
        return -1;
    }

    HyperionModelHeader tmp = *header;
    tmp.magic        = HYPERION_MODEL_FORMAT_MAGIC;
    tmp.versionMajor = header->versionMajor ? header->versionMajor :
                       HYPERION_MODEL_FORMAT_VERSION_MAJOR;
    tmp.versionMinor = header->versionMinor ? header->versionMinor :
                       HYPERION_MODEL_FORMAT_VERSION_MINOR;
    tmp.metadataLength = metadata ? (header->metadataLength ? header->metadataLength
                                                            : sizeof(*metadata))
                                  : 0u;
    tmp.weightsLength = weights ? weightsLength : 0u;

    size_t metaLen = tmp.metadataLength;
    if (metaLen > sizeof(HyperionModelMetadata)) {
        metaLen = sizeof(HyperionModelMetadata);
    }

    tmp.checksum = hyperionModelChecksum(&tmp, metadata, weights, tmp.weightsLength);

    FILE *file = fopen(path, "wb");
    if (!file) {
        return -1;
    }

    int rc = 0;
    if (fwrite(&tmp, sizeof(tmp), 1, file) != 1) {
        rc = -1;
    }

    if (rc == 0 && tmp.metadataLength > 0 && metadata) {
        rc = write_block(file, metadata, metaLen);
        if (rc == 0 && tmp.metadataLength > metaLen) {
            /* Zero-fill any remaining metadata bytes for forward compatibility */
            size_t remaining    = tmp.metadataLength - metaLen;
            uint8_t *zeroBuffer = (uint8_t *)calloc(remaining, 1);
            if (!zeroBuffer) {
                rc = -1;
            }
            else {
                rc = write_block(file, zeroBuffer, remaining);
                free(zeroBuffer);
            }
        }
    }

    if (rc == 0 && tmp.weightsLength > 0 && weights) {
        rc = write_block(file, weights, tmp.weightsLength);
    }

    if (rc != 0) {
        fclose(file);
        remove(path);
        return -1;
    }

    fclose(file);
    return 0;
}

static int read_metadata_block(FILE *file, const HyperionModelHeader *header,
                               HyperionModelMetadata *metadata)
{
    if (!header || header->metadataLength == 0) {
        if (metadata) {
            memset(metadata, 0, sizeof(*metadata));
        }
        return 0;
    }

    size_t toRead = header->metadataLength;
    uint8_t buffer[HYPERION_MODEL_IO_CHUNK];
    size_t  offset = 0;

    if (metadata) {
        memset(metadata, 0, sizeof(*metadata));
    }

    while (toRead > 0) {
        size_t chunk = toRead < sizeof(buffer) ? toRead : sizeof(buffer);
        if (fread(buffer, 1, chunk, file) != chunk) {
            return -1;
        }

        if (metadata && offset < sizeof(*metadata)) {
            size_t remaining     = sizeof(*metadata) - offset;
            size_t copyLength    = (chunk < remaining) ? chunk : remaining;
            memcpy(((uint8_t *)metadata) + offset, buffer, copyLength);
        }

        toRead -= chunk;
        offset += chunk;
    }

    return 0;
}

int hyperionModelRead(const char *path, HyperionModelInfo *info, void **weightsOut,
                      size_t *weightsLength)
{
    if (!path) {
        return -1;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return -1;
    }

    HyperionModelHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }

    if (header.magic != HYPERION_MODEL_FORMAT_MAGIC) {
        fclose(file);
        errno = EINVAL;
        return -1;
    }

    if (header.versionMajor > HYPERION_MODEL_FORMAT_VERSION_MAJOR) {
        fclose(file);
        errno = EINVAL;
        return -1;
    }

    if (info) {
        info->header = header;
    }

    if (read_metadata_block(file, &header, info ? &info->metadata : NULL) != 0) {
        fclose(file);
        return -1;
    }

    void  *weightsBuffer = NULL;
    size_t weightsSize   = (size_t)header.weightsLength;

    if (weightsSize > 0 && weightsOut) {
        weightsBuffer = HYPERION_MALLOC(weightsSize);
        if (!weightsBuffer) {
            fclose(file);
            return -1;
        }

        size_t totalRead = fread(weightsBuffer, 1, weightsSize, file);
        if (totalRead != weightsSize) {
            HYPERION_FREE(weightsBuffer);
            fclose(file);
            return -1;
        }
    }
    else if (weightsSize > 0) {
        /* Skip weight block */
        if (fseek(file, (long)weightsSize, SEEK_CUR) != 0) {
            fclose(file);
            return -1;
        }
    }

    fclose(file);

    if (weightsOut) {
        *weightsOut = weightsBuffer;
    }
    if (weightsLength) {
        *weightsLength = weightsSize;
    }

    return 0;
}

int hyperionModelVerify(const char *path, HyperionModelVerification *result)
{
    if (result) {
        memset(result, 0, sizeof(*result));
    }

    if (!path) {
        return -1;
    }

    FILE *file = fopen(path, "rb");
    if (!file) {
        return -1;
    }

    HyperionModelHeader header;
    if (fread(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return -1;
    }

    if (result && header.magic != HYPERION_MODEL_FORMAT_MAGIC) {
        result->invalidMagic = 1;
        fclose(file);
        return 0;
    }

    if (header.magic != HYPERION_MODEL_FORMAT_MAGIC) {
        fclose(file);
        return 0;
    }

    if (result && header.versionMajor > HYPERION_MODEL_FORMAT_VERSION_MAJOR) {
        result->versionMismatch = 1;
    }

    uint8_t  buffer[HYPERION_MODEL_IO_CHUNK];
    uint64_t remaining = header.metadataLength;
    uint32_t crc       = crc32_begin();

    while (remaining > 0) {
        size_t chunk = remaining < sizeof(buffer) ? (size_t)remaining : sizeof(buffer);
        if (fread(buffer, 1, chunk, file) != chunk) {
            fclose(file);
            return -1;
        }
        crc = crc32_update(crc, buffer, chunk);
        remaining -= chunk;
    }

    remaining = header.weightsLength;
    while (remaining > 0) {
        size_t chunk = remaining < sizeof(buffer) ? (size_t)remaining : sizeof(buffer);
        if (fread(buffer, 1, chunk, file) != chunk) {
            fclose(file);
            return -1;
        }
        crc = crc32_update(crc, buffer, chunk);
        remaining -= chunk;
    }

    crc = crc32_finish(crc);

    fclose(file);

    if (result) {
        result->checksumMismatch = (crc != header.checksum);
        result->success = (result->invalidMagic == 0 && result->versionMismatch == 0 &&
                           result->checksumMismatch == 0);
    }

    return 0;
}
