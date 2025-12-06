#include "tokenizer.h"

#include <stdlib.h>
#include <string.h>

#include "core/logging.h"

static char *hyperionTokenizerDup(const char *source) {
    if (!source) {
        return NULL;
    }
    size_t length = strlen(source);
    char *copy = (char *)malloc(length + 1);
    if (copy) {
        memcpy(copy, source, length + 1);
    }
    return copy;
}

static bool hyperionTokenizerValidateVocab(const char **vocab, size_t vocabSize) {
    if (!vocab || vocabSize == 0) {
        hyperionLogError("Tokenizer initialization failed: vocabulary is missing or empty.");
        return false;
    }

    for (size_t i = 0; i < vocabSize; ++i) {
        const char *token = vocab[i];
        if (!token) {
            hyperionLogError("Tokenizer initialization failed: token at index %zu is NULL.", i);
            return false;
        }

        size_t length = strlen(token);
        if (length == 0) {
            hyperionLogError("Tokenizer initialization failed: token at index %zu is empty.", i);
            return false;
        }

        if (length > HYPERION_MAX_TOKEN_LENGTH) {
            hyperionLogError("Tokenizer initialization failed: token '%s' exceeds maximum length %d.",
                             token,
                             HYPERION_MAX_TOKEN_LENGTH);
            return false;
        }

        for (size_t j = i + 1; j < vocabSize; ++j) {
            const char *other = vocab[j];
            if (!other) {
                hyperionLogError("Tokenizer initialization failed: token at index %zu is NULL.", j);
                return false;
            }

            if (strcmp(token, other) == 0) {
                hyperionLogError("Tokenizer initialization failed: duplicate token '%s' found at indices %zu and %zu.",
                                 token,
                                 i,
                                 j);
                return false;
            }
        }
    }

    return true;
}

static bool hyperionTokenizerValidateMerges(const HyperionMergeRule *merges,
                                            size_t mergeCount,
                                            size_t vocabSize) {
    if (mergeCount == 0) {
        return true;
    }

    if (!merges) {
        hyperionLogError("Tokenizer initialization failed: merge rules are missing while merge count is %zu.", mergeCount);
        return false;
    }

    for (size_t i = 0; i < mergeCount; ++i) {
        const HyperionMergeRule *rule = &merges[i];
        if (rule->rank < 0) {
            hyperionLogError("Tokenizer initialization failed: merge rule %zu has negative rank %d.", i, rule->rank);
            return false;
        }

        if (rule->left >= vocabSize || rule->right >= vocabSize) {
            hyperionLogError(
                "Tokenizer initialization failed: merge rule %zu references tokens outside vocabulary bounds (left=%zu, right=%zu, vocabSize=%zu).",
                i,
                rule->left,
                rule->right,
                vocabSize);
            return false;
        }

        for (size_t j = i + 1; j < mergeCount; ++j) {
            if (rule->rank == merges[j].rank) {
                hyperionLogError(
                    "Tokenizer initialization failed: duplicate merge rank %d found for rules %zu and %zu.",
                    rule->rank,
                    i,
                    j);
                return false;
            }
        }
    }

    return true;
}

int hyperionTokenizerInit(HyperionTokenizer *tokenizer,
                          const char **vocab,
                          size_t vocabSize,
                          const HyperionMergeRule *merges,
                          size_t mergeCount) {
    if (!tokenizer) {
        return 1;
    }

    tokenizer->vocab = NULL;
    tokenizer->vocabSize = 0;
    tokenizer->merges = NULL;
    tokenizer->mergeCount = 0;

    if (!hyperionTokenizerValidateVocab(vocab, vocabSize)) {
        return 1;
    }

    if (!hyperionTokenizerValidateMerges(merges, mergeCount, vocabSize)) {
        return 1;
    }

    tokenizer->vocab = (char **)calloc(vocabSize, sizeof(char *));
    if (!tokenizer->vocab) {
        hyperionLogError("Tokenizer initialization failed: unable to allocate vocabulary buffer.");
        return 1;
    }

    for (size_t i = 0; i < vocabSize; ++i) {
        tokenizer->vocab[i] = hyperionTokenizerDup(vocab[i]);
        if (!tokenizer->vocab[i]) {
            hyperionLogError("Tokenizer initialization failed: memory allocation error while copying token '%s'.", vocab[i]);
            hyperionTokenizerCleanup(tokenizer);
            return 1;
        }
    }

    tokenizer->vocabSize = vocabSize;

    if (mergeCount > 0) {
        tokenizer->merges = (HyperionMergeRule *)calloc(mergeCount, sizeof(HyperionMergeRule));
        if (!tokenizer->merges) {
            hyperionLogError("Tokenizer initialization failed: unable to allocate merge rules buffer.");
            hyperionTokenizerCleanup(tokenizer);
            return 1;
        }

        memcpy(tokenizer->merges, merges, mergeCount * sizeof(HyperionMergeRule));
    }

    tokenizer->mergeCount = mergeCount;
    return 0;
}

void hyperionTokenizerCleanup(HyperionTokenizer *tokenizer) {
    if (!tokenizer) {
        return;
    }

    if (tokenizer->vocab) {
        for (size_t i = 0; i < tokenizer->vocabSize; ++i) {
            free(tokenizer->vocab[i]);
        }
        free(tokenizer->vocab);
    }

    if (tokenizer->merges) {
        free(tokenizer->merges);
    }

    tokenizer->vocab = NULL;
    tokenizer->merges = NULL;
    tokenizer->vocabSize = 0;
    tokenizer->mergeCount = 0;
}
