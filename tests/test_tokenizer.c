#include "models/text/tokenizer.h"

#include <assert.h>
#include <string.h>

static void test_duplicate_tokens_fail(void) {
    const char *vocab[] = {"hello", "world", "hello"};
    HyperionMergeRule merges[] = {{0, 1, 0}};

    HyperionTokenizer tokenizer;
    int result = hyperionTokenizerInit(&tokenizer, vocab, 3, merges, 1);
    assert(result != 0);
}

static void test_invalid_merge_rank_fail(void) {
    const char *vocab[] = {"a", "b", "c"};
    HyperionMergeRule merges[] = {{0, 1, 1}, {1, 2, 1}}; /* Duplicate rank */

    HyperionTokenizer tokenizer;
    int result = hyperionTokenizerInit(&tokenizer, vocab, 3, merges, 2);
    assert(result != 0);
}

static void test_token_length_limit_fail(void) {
    char longToken[HYPERION_MAX_TOKEN_LENGTH + 2];
    memset(longToken, 'x', sizeof(longToken));
    longToken[HYPERION_MAX_TOKEN_LENGTH + 1] = '\0';

    const char *vocab[] = {"short", longToken};
    HyperionMergeRule merges[] = {{0, 1, 0}};

    HyperionTokenizer tokenizer;
    int result = hyperionTokenizerInit(&tokenizer, vocab, 2, merges, 1);
    assert(result != 0);
}

static void test_valid_tokenizer_initialization(void) {
    const char *vocab[] = {"hello", "world", "!"};
    HyperionMergeRule merges[] = {{0, 1, 0}, {1, 2, 1}};

    HyperionTokenizer tokenizer;
    assert(hyperionTokenizerInit(&tokenizer, vocab, 3, merges, 2) == 0);

    assert(tokenizer.vocabSize == 3);
    assert(tokenizer.mergeCount == 2);
    assert(strcmp(tokenizer.vocab[0], "hello") == 0);

    hyperionTokenizerCleanup(&tokenizer);
}

void run_tokenizer_tests(void) {
    test_duplicate_tokens_fail();
    test_invalid_merge_rank_fail();
    test_token_length_limit_fail();
    test_valid_tokenizer_initialization();
}
