#include <stdio.h>

void run_cli_tests(void);
void run_memory_tests(void);

int main(void) {
    run_cli_tests();
    run_memory_tests();
    printf("All tests completed.\n");
    return 0;
}
