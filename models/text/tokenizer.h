/**
 * Hyperion Tokenizer Header
 * 
 * This header defines the tokenizer API for text processing in Hyperion.
 */

#ifndef HYPERION_TOKENIZER_H
#define HYPERION_TOKENIZER_H

#include <stdint.h>

/* ----------------- Constants ----------------- */

/* Maximum vocabulary size */
#define HYPERION_MAX_VOCAB_SIZE 65536

/* Maximum token length */
#define HYPERION_MAX_TOKEN_LENGTH 256

/* Special token IDs */
#define HYPERION_TOKEN_UNKNOWN 0
#define HYPERION_TOKEN_BOS     1
#define HYPERION_TOKEN_EOS     2
#define HYPERION_TOKEN_PAD     3

/* ----------------- Types ----------------- */

/**
 * Tokenizer structure
 */
typedef struct {
    char *tokens[HYPERION_MAX_VOCAB_SIZE];    /* Token strings */
    uint32_t tokenCount;                    /* Number of tokens in vocabulary */
    uint32_t *frequencies;                  /* Token frequencies (for training) */
    int caseSensitive;                      /* Whether tokenization is case-sensitive */
} HyperionTokenizer;

/* ----------------- API Functions ----------------- */

/**
 * Create a new tokenizer
 * 
 * @return New tokenizer or NULL on error
 */
HyperionTokenizer* hyperionCreateTokenizer();

/**
 * Destroy a tokenizer
 * 
 * @param tokenizer Tokenizer to destroy
 */
void hyperionDestroyTokenizer(HyperionTokenizer *tokenizer);

/**
 * Load a vocabulary from a file
 * 
 * @param tokenizer Tokenizer to load into
 * @param path File path
 * @return 0 on success, non-zero on error
 */
int hyperionLoadVocabulary(HyperionTokenizer *tokenizer, const char *path);

/**
 * Add a token to the vocabulary
 * 
 * @param tokenizer Tokenizer to add to
 * @param token Token string
 * @param frequency Token frequency
 * @return Token ID or -1 on error
 */
int hyperionAddToken(HyperionTokenizer *tokenizer, const char *token, uint32_t frequency);

/**
 * Get a token ID by string
 * 
 * @param tokenizer Tokenizer to use
 * @param token Token string
 * @return Token ID or HYPERION_TOKEN_UNKNOWN if not found
 */
int hyperionGetTokenId(const HyperionTokenizer *tokenizer, const char *token);

/**
 * Get a token string by ID
 * 
 * @param tokenizer Tokenizer to use
 * @param id Token ID
 * @return Token string or NULL if not found
 */
const char* hyperionGetTokenString(const HyperionTokenizer *tokenizer, int id);

/**
 * Encode a text string into token IDs
 * 
 * @param tokenizer Tokenizer to use
 * @param text Input text
 * @param tokens Output token array
 * @param maxTokens Maximum number of tokens to output
 * @return Number of tokens encoded
 */
int hyperionEncodeText(const HyperionTokenizer *tokenizer, const char *text, 
                   int *tokens, int maxTokens);

/**
 * Decode token IDs into a text string
 * 
 * @param tokenizer Tokenizer to use
 * @param tokens Input token array
 * @param tokenCount Number of tokens to decode
 * @param text Output text buffer
 * @param maxLength Maximum output length
 * @return Length of decoded text
 */
int hyperionDecodeTokens(const HyperionTokenizer *tokenizer, const int *tokens, 
                     int tokenCount, char *text, int maxLength);

/**
 * Create a minimal BPE tokenizer vocabulary from text corpus
 * 
 * @param tokenizer Tokenizer to use
 * @param corpus Input text corpus
 * @param maxVocabSize Maximum vocabulary size
 * @return 0 on success, non-zero on error
 */
int hyperionCreateMinimalVocabulary(HyperionTokenizer *tokenizer, 
                                const char *corpus, int maxVocabSize);

/**
 * Save a vocabulary to a file
 * 
 * @param tokenizer Tokenizer to save
 * @param path File path
 * @return 0 on success, non-zero on error
 */
int hyperionSaveVocabulary(const HyperionTokenizer *tokenizer, const char *path);

#endif /* HYPERION_TOKENIZER_H */