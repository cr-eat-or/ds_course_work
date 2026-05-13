/*
 * Тесты модуля grid: создание, границы get/set, копия, равенство, clear, ошибки размера.
 */
#include "grid.h"
#include "test_helpers.h"

#include <stddef.h>

int test_grid_run(void) {
    Grid *grid_one;
    Grid *grid_two;
    int failed;

    failed = 0;

    /* Базовые операции на поле 3×3. */
    grid_one = grid_create(3, 3);
    failed += !expect_true(grid_one != NULL, "grid_create");
    if (grid_one == NULL) {
        return failed;
    }

    failed += !expect_true(grid_one->width == 3 && grid_one->height == 3, "размеры поля");

    failed += !expect_true(!grid_get(grid_one, 1, 1), "новая клетка мёртвая");

    grid_set(grid_one, 1, 1, true);
    failed += !expect_true(grid_get(grid_one, 1, 1), "set alive");

    grid_set(grid_one, 1, 1, false);
    failed += !expect_true(!grid_get(grid_one, 1, 1), "set dead");

    failed += !expect_true(!grid_get(grid_one, -1, 0) && !grid_get(grid_one, 3, 0),
                          "чтение вне поля — мёртвая клетка");

    grid_set(grid_one, 5, 5, true);
    failed += !expect_true(!grid_get(grid_one, 2, 2), "запись вне поля игнорируется");

    grid_two = grid_create(3, 3);
    failed += !expect_true(grid_two != NULL, "grid_create b");
    if (grid_two == NULL) {
        grid_free(grid_one);
        return failed;
    }

    grid_set(grid_one, 1, 1, true);
    failed += !expect_true(grid_copy(grid_two, grid_one), "grid_copy");

    failed += !expect_true(grid_equal(grid_one, grid_two), "равенство после копирования");

    grid_clear(grid_one);
    failed += !expect_true(!grid_get(grid_one, 1, 1), "clear");

    failed += !expect_true(!grid_equal(grid_one, grid_two), "после clear сетки различаются");

    grid_free(grid_one);
    grid_free(grid_two);

    /* Нулевой размер — отказ без поля. */
    failed += !expect_true(grid_create(0, 5) == NULL && grid_create(5, 0) == NULL,
                          "некорректный размер даёт NULL");

    if (failed == 0) {
        printf("test_grid: OK\n");
    }

    return failed;
}
