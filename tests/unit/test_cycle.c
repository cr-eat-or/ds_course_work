/*
 * Тесты cycle_push: NEW/CYCLE/ERROR, reset, смена размера поля, лимит снимков, связка с life (мигалка).
 */
#include "cycle.h"
#include "grid.h"
#include "life.h"
#include "test_helpers.h"

#include <stdio.h>

int test_cycle_run(void) {
    CycleCtx *ctx;
    Grid *field;
    Grid *grid_small;
    Grid *grid_large;
    Grid *grid_cur;
    Grid *grid_nxt;
    Grid *swap_buf;
    size_t period;

    int     failed = 0;

    /* Некорректные аргументы. */
    failed += !expect_true(cycle_push(NULL, NULL, &period) == CYCLE_PUSH_ERROR,
                          "cycle_push NULL");

    ctx = cycle_create(0);
    failed += !expect_true(ctx != NULL, "cycle_create");
    if (ctx == NULL) {
        return failed;
    }

    field = grid_create(3, 3);
    failed += !expect_true(field != NULL, "grid_create 3x3");
    if (field == NULL) {
        cycle_free(ctx);
        return failed;
    }

    failed += !expect_true(cycle_push(ctx, field, NULL) == CYCLE_PUSH_NEW, "первый push");
    failed += !expect_true(cycle_history_len(ctx) == 1, "история длина 1");

    failed += !expect_true(cycle_push(ctx, field, &period) == CYCLE_PUSH_CYCLE,
                          "то же состояние — цикл");
    failed += !expect_true(period == 1, "период 1");

    cycle_reset(ctx);
    failed += !expect_true(cycle_history_len(ctx) == 0, "reset очищает");

    grid_set(field, 0, 0, true);
    failed += !expect_true(cycle_push(ctx, field, NULL) == CYCLE_PUSH_NEW, "push A");

    grid_clear(field);
    failed += !expect_true(cycle_push(ctx, field, NULL) == CYCLE_PUSH_NEW, "push B");

    grid_set(field, 0, 0, true);
    failed += !expect_true(cycle_push(ctx, field, &period) == CYCLE_PUSH_CYCLE, 
                          "снова A — цикл");

    failed += !expect_true(period == 2, "период 2 (A-B-A)");

    cycle_free(ctx);
    grid_free(field);

    /* После первого снимка размер поля фиксируется — другой размер даёт ERROR. */
    ctx = cycle_create(0);
    grid_small = grid_create(2, 2);
    grid_large = grid_create(3, 3);
    failed += !expect_true(ctx != NULL && grid_small != NULL && grid_large != NULL,
                          "create для ошибки размера");
    if (ctx == NULL || grid_small == NULL || grid_large == NULL) {
        cycle_free(ctx);
        grid_free(grid_small);
        grid_free(grid_large);
        return failed;
    }

    failed += !expect_true(cycle_push(ctx, grid_small, NULL) == CYCLE_PUSH_NEW, 
                          "первый размер 2x2");
    failed += !expect_true(cycle_push(ctx, grid_large, NULL) == CYCLE_PUSH_ERROR, 
                          "другой размер — ошибка");

    cycle_free(ctx);
    grid_free(grid_small);
    grid_free(grid_large);

    /* max_snapshots: третий уникальный снимок запрещён. */
    ctx = cycle_create(2);
    field = grid_create(2, 2);
    failed += !expect_true(ctx != NULL && field != NULL, "create лимит");
    if (ctx == NULL || field == NULL) {
        cycle_free(ctx);
        grid_free(field);
        return failed;
    }

    failed += !expect_true(cycle_push(ctx, field, NULL) == CYCLE_PUSH_NEW, "лимит: 1");
    grid_set(field, 0, 0, true);
    failed += !expect_true(cycle_push(ctx, field, NULL) == CYCLE_PUSH_NEW, "лимит: 2");
    grid_clear(field);
    grid_set(field, 1, 1, true);
    failed += !expect_true(cycle_push(ctx, field, NULL) == CYCLE_PUSH_ERROR,
                          "лимит: третий новый снимок запрещён");
    cycle_free(ctx);
    grid_free(field);

    /* Мигалка + life: период 2. */
    ctx = cycle_create(0);
    grid_cur = grid_create(5, 5);
    grid_nxt = grid_create(5, 5);
    failed += !expect_true(ctx != NULL && grid_cur != NULL && grid_nxt != NULL, "create мигалка");
    if (ctx == NULL || grid_cur == NULL || grid_nxt == NULL) {
        cycle_free(ctx);
        grid_free(grid_cur);
        grid_free(grid_nxt);
        return failed;
    }
    
    grid_set(grid_cur, 1, 2, true);
    grid_set(grid_cur, 2, 2, true);
    grid_set(grid_cur, 3, 2, true);

    failed += !expect_true(cycle_push(ctx, grid_cur, NULL) == CYCLE_PUSH_NEW, "мигалка S0");
    failed += !expect_true(life_step(grid_cur, grid_nxt), "life_step 1");
    swap_buf = grid_cur;
    grid_cur = grid_nxt;
    grid_nxt = swap_buf;
    failed += !expect_true(cycle_push(ctx, grid_cur, NULL) == CYCLE_PUSH_NEW, "мигалка S1");
    failed += !expect_true(life_step(grid_cur, grid_nxt), "life_step 2");
    swap_buf = grid_cur;
    grid_cur = grid_nxt;
    grid_nxt = swap_buf;
    failed += !expect_true(cycle_push(ctx, grid_cur, &period) == CYCLE_PUSH_CYCLE,
                          "мигалка цикл");
    failed += !expect_true(period == 2, "мигалка период 2");

    cycle_free(ctx);
    grid_free(grid_cur);
    grid_free(grid_nxt);

    if (failed == 0) {
        printf("test_cycle: OK\n");
    }
    return failed;
}
