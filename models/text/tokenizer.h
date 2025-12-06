#ifndef HYPERION_TOKENIZER_H
#define HYPERION_TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HYPERION_MAX_TOKEN_LENGTH 256

typedef struct {
    size_t left;
    size_t right;
    int rank;
} HyperionMergeRule;

typedef struct {
    char **vocab;
    size_t vocabSize;
    HyperionMergeRule *merges;
    size_t mergeCount;
} HyperionTokenizer;

int hyperionTokenizerInit(HyperionTokenizer *tokenizer,
                          const char **vocab,
                          size_t vocabSize,
                          const HyperionMergeRule *merges,
                          size_t mergeCount);

void hyperionTokenizerCleanup(HyperionTokenizer *tokenizer);

#ifdef __cplusplus
}
#endif

#endif // HYPERION_TOKENIZER_H
