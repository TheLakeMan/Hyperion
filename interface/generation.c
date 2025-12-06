#include "generation.h"

void hyperionTokenIteratorInit(HyperionTokenIterator *iterator, const char *const *tokens, size_t tokenCount) {
    if (!iterator) {
        return;
    }

    iterator->tokens = tokens;
    iterator->tokenCount = tokenCount;
    iterator->currentIndex = 0;
}

const char *hyperionTokenIteratorNext(HyperionTokenIterator *iterator) {
    if (!iterator || !iterator->tokens || iterator->currentIndex >= iterator->tokenCount) {
        return NULL;
    }

    const char *token = iterator->tokens[iterator->currentIndex];
    iterator->currentIndex += 1;
    return token;
}

int hyperionStreamTokens(HyperionTokenIterator *iterator, HyperionTokenCallback callback, void *userData) {
    if (!iterator || !callback) {
        return 1;
    }

    const char *token = NULL;
    while ((token = hyperionTokenIteratorNext(iterator)) != NULL) {
        int status = callback(token, userData);
        if (status != 0) {
            return status;
        }
    }

    return 0;
}
