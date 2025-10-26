#ifndef HYPERION_TEST_FRAMEWORK_H
#define HYPERION_TEST_FRAMEWORK_H

#include <stddef.h>
#include <stdio.h>

typedef int (*HyperionTestFunc)(void);

typedef struct {
    const char      *name;
    const char      *category;
    HyperionTestFunc func;
} HyperionTestCase;

int hyperionRunTests(const HyperionTestCase *cases, size_t count, const char *filter);

#define HYPERION_TEST(name) static int name(void)

#define HYPERION_ASSERT(condition, message)                                                    \
    do {                                                                                       \
        if (!(condition)) {                                                                    \
            fprintf(stderr, "Assertion failed: %s (%s:%d)\n", (message), __FILE__, __LINE__); \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

#endif /* HYPERION_TEST_FRAMEWORK_H */
