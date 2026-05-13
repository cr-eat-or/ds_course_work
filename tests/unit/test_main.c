/*
 * Запуск всех наборов тестов: суммирует число провалов (0 = успех).
 */
#include <stdio.h>

int test_grid_run(void);
int test_life_run(void);
int test_cycle_run(void);
int test_templates_run(void);

int main(void) {
    int failed;

    /* Каждый test_*_run возвращает количество неудачных expect_true. */
    failed = 0;
    failed += test_grid_run();
    failed += test_life_run();
    failed += test_cycle_run();
    failed += test_templates_run();

    if (failed == 0) {
        printf("All tests passed.\n");
        return 0;
    }

    printf("Tests failed: %d\n", failed);
    return 1;
}
