/**
 * Hyperion Tokenizer Tests
 */

#include "../models/text/tokenizer.h" // Include the tokenizer being tested
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic assertion helper (consistent with other test files)
#define ASSERT(condition, message)                                                                 \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Assertion Failed: %s (%s:%d)\n", message, __FILE__, __LINE__);        \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)

// Test creating and destroying tokenizer
void test_tokenizer_create_destroy()
{
    printf("  Testing tokenizer creation/destruction...\n");

    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    ASSERT(tokenizer != NULL, "hyperionCreateTokenizer() should return non-NULL");
    ASSERT(tokenizer->tokenCount >= 4, "Tokenizer should have at least special tokens");

    hyperionDestroyTokenizer(tokenizer);
    printf("    PASS\n");
}

// Test adding tokens to vocabulary
void test_add_tokens()
{
    printf("  Testing adding tokens to vocabulary...\n");

    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();

    // Add some test tokens
    int id1 = hyperionAddToken(tokenizer, "test", 100);
    int id2 = hyperionAddToken(tokenizer, "hello", 200);
    int id3 = hyperionAddToken(tokenizer, "world", 300);

    ASSERT(id1 >= 0, "hyperionAddToken should return valid ID");
    ASSERT(id2 >= 0, "hyperionAddToken should return valid ID");
    ASSERT(id3 >= 0, "hyperionAddToken should return valid ID");

    // Test getting token by string
    int lookup1       = hyperionGetTokenId(tokenizer, "test");
    int lookup2       = hyperionGetTokenId(tokenizer, "hello");
    int lookup3       = hyperionGetTokenId(tokenizer, "world");
    int lookupUnknown = hyperionGetTokenId(tokenizer, "unknown");

    ASSERT(lookup1 == id1, "Token lookup should return correct ID");
    ASSERT(lookup2 == id2, "Token lookup should return correct ID");
    ASSERT(lookup3 == id3, "Token lookup should return correct ID");
    ASSERT(lookupUnknown == TINYAI_TOKEN_UNKNOWN, "Unknown token should return UNKNOWN ID");

    // Test getting token by ID
    const char *str1 = hyperionGetTokenString(tokenizer, id1);
    const char *str2 = hyperionGetTokenString(tokenizer, id2);
    const char *str3 = hyperionGetTokenString(tokenizer, id3);

    ASSERT(strcmp(str1, "test") == 0, "Token string lookup should return correct string");
    ASSERT(strcmp(str2, "hello") == 0, "Token string lookup should return correct string");
    ASSERT(strcmp(str3, "world") == 0, "Token string lookup should return correct string");

    hyperionDestroyTokenizer(tokenizer);
    printf("    PASS\n");
}

// Test simple encoding/decoding
void test_encode_decode_simple()
{
    printf("  Testing simple text encoding/decoding...\n");

    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();

    // Add some test tokens
    hyperionAddToken(tokenizer, "Hello", 100);
    hyperionAddToken(tokenizer, "world", 90);
    hyperionAddToken(tokenizer, "!", 80);
    hyperionAddToken(tokenizer, "How", 70);
    hyperionAddToken(tokenizer, "are", 60);
    hyperionAddToken(tokenizer, "you", 50);

    // Test encoding
    const char *text       = "Hello world!";
    int         tokens[32] = {0};
    int         tokenCount = hyperionEncodeText(tokenizer, text, tokens, 32);

    ASSERT(tokenCount > 0, "Encoding should return token count > 0");
    ASSERT(tokenCount == 3, "Expected 3 tokens for 'Hello world!'");

    const char *token0 = hyperionGetTokenString(tokenizer, tokens[0]);
    const char *token1 = hyperionGetTokenString(tokenizer, tokens[1]);
    const char *token2 = hyperionGetTokenString(tokenizer, tokens[2]);

    ASSERT(strcmp(token0, "Hello") == 0, "First token should be 'Hello'");
    ASSERT(strcmp(token1, "world") == 0, "Second token should be 'world'");
    ASSERT(strcmp(token2, "!") == 0, "Third token should be '!'");

    // Test decoding
    char decoded[100] = {0};
    int decodedLength = hyperionDecodeTokens(tokenizer, tokens, tokenCount, decoded, sizeof(decoded));

    ASSERT(decodedLength > 0, "Decoding should return length > 0");
    ASSERT(strcmp(decoded, "Hello world!") == 0, "Decoded text should match original");

    hyperionDestroyTokenizer(tokenizer);
    printf("    PASS\n");
}

// Test encoding/decoding with unknown tokens
void test_encode_decode_unknown()
{
    printf("  Testing encoding/decoding with unknown tokens...\n");

    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();

    // Add only some tokens from the test text
    hyperionAddToken(tokenizer, "Hello", 100);
    hyperionAddToken(tokenizer, "!", 80);

    // Text with unknown token "world"
    const char *text       = "Hello world!";
    int         tokens[32] = {0};
    int         tokenCount = hyperionEncodeText(tokenizer, text, tokens, 32);

    ASSERT(tokenCount > 0, "Encoding should return token count > 0");
    ASSERT(tokens[1] == TINYAI_TOKEN_UNKNOWN, "Unknown token should be encoded as UNKNOWN");

    // Test decoding with unknown token
    char decoded[100] = {0};
    int decodedLength = hyperionDecodeTokens(tokenizer, tokens, tokenCount, decoded, sizeof(decoded));

    ASSERT(decodedLength > 0, "Decoding should return length > 0");

    // Check that <unk> replaces unknown token (or however it's handled in the tokenizer)
    // This depends on the specific implementation, so adjust assertion as needed
    printf("    Decoded with unknown token: '%s'\n", decoded);

    hyperionDestroyTokenizer(tokenizer);
    printf("    PASS\n");
}

// Test encoding with limited buffer
void test_encoding_buffer_limits()
{
    printf("  Testing encoding with limited buffer...\n");

    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();

    // Add tokens
    hyperionAddToken(tokenizer, "This", 100);
    hyperionAddToken(tokenizer, "is", 90);
    hyperionAddToken(tokenizer, "a", 80);
    hyperionAddToken(tokenizer, "test", 70);
    hyperionAddToken(tokenizer, "of", 60);
    hyperionAddToken(tokenizer, "buffer", 50);
    hyperionAddToken(tokenizer, "limits", 40);

    // Test text with more tokens than our small buffer can hold
    const char *text       = "This is a test of buffer limits";
    int         tokens[3]  = {0}; // Only space for 3 tokens
    int         tokenCount = hyperionEncodeText(tokenizer, text, tokens, 3);

    ASSERT(tokenCount == 3, "Encoding should respect buffer size limit");

    hyperionDestroyTokenizer(tokenizer);
    printf("    PASS\n");
}

// Test save and load vocabulary
void test_save_load_vocabulary()
{
    printf("  Testing save/load vocabulary...\n");

    const char *testVocabPath = "test_vocab.txt";

    // Create and populate tokenizer
    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    hyperionAddToken(tokenizer, "test", 100);
    hyperionAddToken(tokenizer, "vocabulary", 90);
    hyperionAddToken(tokenizer, "save", 80);
    hyperionAddToken(tokenizer, "load", 70);

    // Save vocabulary
    int saveResult = hyperionSaveVocabulary(tokenizer, testVocabPath);
    ASSERT(saveResult == 0, "Saving vocabulary should succeed");

    // Destroy original tokenizer
    hyperionDestroyTokenizer(tokenizer);

    // Create new tokenizer and load vocabulary
    HyperionTokenizer *newTokenizer = hyperionCreateTokenizer();
    int              loadResult   = hyperionLoadVocabulary(newTokenizer, testVocabPath);
    ASSERT(loadResult == 0, "Loading vocabulary should succeed");

    // Verify loaded vocabulary
    int id1 = hyperionGetTokenId(newTokenizer, "test");
    int id2 = hyperionGetTokenId(newTokenizer, "vocabulary");
    int id3 = hyperionGetTokenId(newTokenizer, "save");
    int id4 = hyperionGetTokenId(newTokenizer, "load");

    ASSERT(id1 != TINYAI_TOKEN_UNKNOWN, "Loaded tokenizer should contain 'test'");
    ASSERT(id2 != TINYAI_TOKEN_UNKNOWN, "Loaded tokenizer should contain 'vocabulary'");
    ASSERT(id3 != TINYAI_TOKEN_UNKNOWN, "Loaded tokenizer should contain 'save'");
    ASSERT(id4 != TINYAI_TOKEN_UNKNOWN, "Loaded tokenizer should contain 'load'");

    // Clean up
    hyperionDestroyTokenizer(newTokenizer);
    remove(testVocabPath); // Delete test file

    printf("    PASS\n");
}

// Helper function to create a simple test corpus
const char *get_test_corpus()
{
    // Static buffer for simplicity
    static char corpus[4096];
    snprintf(corpus, sizeof(corpus),
             "This is a simple test corpus for minimal BPE tokenization.\n"
             "It contains simple words and phrases to test the tokenizer.\n"
             "The quick brown fox jumps over the lazy dog.\n"
             "Hello world! How are you today?\n"
             "Hyperion is designed to be memory-efficient and run on minimal hardware.");
    return corpus;
}

// Test minimal BPE vocabulary creation
void test_minimal_vocabulary()
{
    printf("  Testing minimal vocabulary creation...\n");

    HyperionTokenizer *tokenizer = hyperionCreateTokenizer();
    const char      *corpus    = get_test_corpus();

    // Create vocabulary
    int vocabSize = 100;
    int result    = hyperionCreateMinimalVocabulary(tokenizer, corpus, vocabSize);
    ASSERT(result == 0, "Creating minimal vocabulary should succeed");

    // Verify size (should be less than or equal to requested size)
    ASSERT(tokenizer->tokenCount <= vocabSize, "Vocabulary size should not exceed requested size");

    // Encode the corpus and verify reasonable results
    int tokens[1000] = {0};
    int tokenCount   = hyperionEncodeText(tokenizer, corpus, tokens, 1000);

    ASSERT(tokenCount > 0, "Encoding corpus should produce tokens");
    printf("    Encoded corpus into %d tokens\n", tokenCount);

    // Decode back and check if output is readable
    char decoded[4096] = {0};
    int decodedLength = hyperionDecodeTokens(tokenizer, tokens, tokenCount, decoded, sizeof(decoded));

    ASSERT(decodedLength > 0, "Decoding should return length > 0");
    printf("    Decoded length: %d characters\n", decodedLength);

    hyperionDestroyTokenizer(tokenizer);
    printf("    PASS\n");
}

// Function to be called by test_main.c
void run_tokenizer_tests()
{
    printf("--- Running Tokenizer Tests ---\n");

    test_tokenizer_create_destroy();
    test_add_tokens();
    test_encode_decode_simple();
    test_encode_decode_unknown();
    test_encoding_buffer_limits();
    test_save_load_vocabulary();
    test_minimal_vocabulary();

    printf("--- Tokenizer Tests Finished ---\n");
}
