#include <stdio.h>

void run_cli_tests(void);
void run_memory_tests(void);
void run_tokenizer_tests(void);

int main(void) {
    run_cli_tests();
    run_memory_tests();
    run_tokenizer_tests();
    printf("All tests completed.\n");
    return 0;
}
