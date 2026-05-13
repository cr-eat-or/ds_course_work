/*
 * Тесты life_step: пустое поле, стабильный блок, мигалка, ошибки размеров и NULL.
 */
#include "grid.h"
#include "life.h"
#include "test_helpers.h"

#include <stdio.h>

int test_life_run(void) {
    Grid *grid_src;
    Grid *grid_dst;
    Grid *wrong_size;

    int failed = 0;

    /* Пустое поле остаётся пустым. */
    grid_src = grid_create(5, 5);
    grid_dst = grid_create(5, 5);
    failed += !expect_true(grid_src != NULL && grid_dst != NULL, "grid_create 5x5");
    if (grid_src == NULL || grid_dst == NULL) {
        grid_free(grid_src);
        grid_free(grid_dst);
        return failed;
    }

    failed += !expect_true(life_step(grid_src, grid_dst), "life_step пустое поле");
    failed += !expect_true(grid_equal(grid_src, grid_dst), "пустое не меняется");

    grid_free(grid_src);
    grid_free(grid_dst);

    /* Блок 2×2 — стабильный. */
    grid_src = grid_create(4, 4);
    grid_dst = grid_create(4, 4);
    failed += !expect_true(grid_src != NULL && grid_dst != NULL, "grid_create 4x4");
    if (grid_src == NULL || grid_dst == NULL) {
        grid_free(grid_src);
        grid_free(grid_dst);
        return failed;
    }

    grid_set(grid_src, 1, 1, true);
    grid_set(grid_src, 2, 1, true);
    grid_set(grid_src, 1, 2, true);
    grid_set(grid_src, 2, 2, true);

    failed += !expect_true(life_step(grid_src, grid_dst), "life_step блок");
    failed += !expect_true(grid_equal(grid_src, grid_dst), "блок стабилен");

    grid_free(grid_src);
    grid_free(grid_dst);

    /* Мигалка (период 2): горизонталь из трёх клеток -> вертикаль. */
    grid_src = grid_create(5, 5);
    grid_dst = grid_create(5, 5);
    failed += !expect_true(grid_src != NULL && grid_dst != NULL, "grid_create 5x5 мигалка");
    if (grid_src == NULL || grid_dst == NULL) {
        grid_free(grid_src);
        grid_free(grid_dst);
        return failed;
    }

    grid_set(grid_src, 1, 2, true);
    grid_set(grid_src, 2, 2, true);
    grid_set(grid_src, 3, 2, true);

    failed += !expect_true(life_step(grid_src, grid_dst), "life_step мигалка");
    failed += !expect_true(!grid_get(grid_dst, 2, 0) && grid_get(grid_dst, 2, 1)
                          && grid_get(grid_dst, 2, 2) && grid_get(grid_dst, 2, 3)
                          && !grid_get(grid_dst, 2, 4),
                          "мигалка: вертикаль из трёх по центру");
    failed += !expect_true(!grid_get(grid_dst, 1, 2) && !grid_get(grid_dst, 3, 2),
                          "мигалка: боковые клетки пустые");
    grid_free(grid_src);
    grid_free(grid_dst);

    /* Несовпадение размеров — ошибка. */
    grid_src = grid_create(3, 3);
    wrong_size = grid_create(4, 4);
    failed += !expect_true(grid_src != NULL && wrong_size != NULL, "grid_create для ошибки");
    if (grid_src == NULL || wrong_size == NULL) {
        grid_free(grid_src);
        grid_free(wrong_size);
        return failed;
    }

    failed += !expect_true(!life_step(grid_src, wrong_size), "life_step разные размеры");
    grid_free(grid_src);
    grid_free(wrong_size);

    /* NULL — ошибка. */
    failed += !expect_true(!life_step(NULL, NULL), "life_step NULL");

    if (failed == 0) {
        printf("test_life: OK\n");
    }
    return failed;
}
