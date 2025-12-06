#ifndef HYPERION_CONFIG_PERSIST_H
#define HYPERION_CONFIG_PERSIST_H

#include <stddef.h>

typedef struct {
    const char *key;
    const char *value;
} HyperionKeyValue;

typedef int (*HyperionKeyValueCallback)(const char *key, const char *value, void *user_data);

int hyperionConfigParseKeyValues(const char *path, HyperionKeyValueCallback callback, void *user_data);
int hyperionConfigWriteKeyValues(const char *path, const HyperionKeyValue *pairs, size_t pair_count);

#endif // HYPERION_CONFIG_PERSIST_H
