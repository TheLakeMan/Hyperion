#include "test_framework.h"
#include <stdio.h>
#include <string.h>

int hyperionRunTests(const HyperionTestCase *cases, size_t count, const char *filter)
{
    if (!cases || count == 0) {
        fprintf(stderr, "No tests registered.\n");
        return 1;
    }

    size_t executed = 0;
    size_t failed   = 0;

    for (size_t i = 0; i < count; ++i) {
        const HyperionTestCase *test = &cases[i];

        if (filter && filter[0] != '\0' && test->category &&
            strcmp(filter, test->category) != 0 && strcmp(filter, test->name) != 0) {
            continue;
        }

        executed++;
        int result = test->func();
        if (result != 0) {
            failed++;
            fprintf(stderr, "[FAIL] %s (%s)\n", test->name,
                    test->category ? test->category : "uncategorized");
        }
        else {
            printf("[PASS] %s\n", test->name);
        }
    }

    if (executed == 0) {
        fprintf(stderr, "No tests matched filter '%s'.\n", filter ? filter : "");
        return 1;
    }

    if (failed > 0) {
        fprintf(stderr, "%zu/%zu tests failed.\n", failed, executed);
        return 1;
    }

    printf("All %zu tests passed.\n", executed);
    return 0;
}
