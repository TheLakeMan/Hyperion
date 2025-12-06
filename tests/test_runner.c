#include <stdio.h>

void run_cli_tests(void);
void run_memory_tests(void);
void run_text_model_tests(void);

int main(void) {
    run_cli_tests();
    run_memory_tests();
    run_text_model_tests();
    printf("All tests completed.\n");
    return 0;
}
