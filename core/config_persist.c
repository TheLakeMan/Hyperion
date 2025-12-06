#include "config_persist.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char *trim(char *text) {
    if (!text) {
        return text;
    }

    while (isspace((unsigned char)*text)) {
        ++text;
    }

    size_t len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }
    return text;
}

int hyperionConfigParseKeyValues(const char *path, HyperionKeyValueCallback callback, void *user_data) {
    if (!callback || !path) {
        return 1;
    }

    FILE *file = fopen(path, "r");
    if (!file) {
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        char *trimmed = trim(line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        char *separator = strchr(trimmed, '=');
        if (!separator) {
            continue;
        }

        *separator = '\0';
        char *key = trim(trimmed);
        char *value = trim(separator + 1);
        if (callback(key, value, user_data) != 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int hyperionConfigWriteKeyValues(const char *path, const HyperionKeyValue *pairs, size_t pair_count) {
    if (!path || (!pairs && pair_count > 0)) {
        return 1;
    }

    FILE *file = fopen(path, "w");
    if (!file) {
        return 1;
    }

    for (size_t i = 0; i < pair_count; ++i) {
        if (!pairs[i].key || !pairs[i].value) {
            fclose(file);
            return 1;
        }
        fprintf(file, "%s=%s\n", pairs[i].key, pairs[i].value);
    }

    fclose(file);
    return 0;
}
