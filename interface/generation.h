#ifndef HYPERION_GENERATION_H
#define HYPERION_GENERATION_H

#include <stddef.h>

typedef struct {
    const char *const *tokens;
    size_t tokenCount;
    size_t currentIndex;
} HyperionTokenIterator;

typedef int (*HyperionTokenCallback)(const char *token, void *userData);

void hyperionTokenIteratorInit(HyperionTokenIterator *iterator, const char *const *tokens, size_t tokenCount);
const char *hyperionTokenIteratorNext(HyperionTokenIterator *iterator);
int hyperionStreamTokens(HyperionTokenIterator *iterator, HyperionTokenCallback callback, void *userData);

#endif // HYPERION_GENERATION_H
