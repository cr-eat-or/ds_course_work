/*
 * Один шаг симуляции: правило Конвея B3/S23 на основе grid_get (в т.ч. мёртвый край).
 */
#include "life.h"

#include <stddef.h>

/* Обход 8 соседних клеток; grid_get за границей даёт false. */
static int count_neighbors(const Grid *src, int col, int row) {
    int neighbor_count = 0;

    for (int off_row = -1; off_row <= 1; off_row++) {
        for (int off_col = -1; off_col <= 1; off_col++) {
            if (off_col == 0 && off_row == 0) {
                continue;
            }

            if (grid_get(src, col + off_col, row + off_row)) {
                neighbor_count++;
            }
        }
    }
    return neighbor_count;
}

/*
 * Двойной буфер: только читаем src, пишем в dst. Выживание 2-3 соседа, рождение ровно при 3.
 */
bool life_step(const Grid *src, Grid *dst) {
    if (src == NULL || dst == NULL) {
        return false;
    }

    if (src->width != dst->width || src->height != dst->height) {
        return false;
    }

    if (src->cells == NULL || dst->cells == NULL) {
        return false;
    }

    for (int row = 0; row < src->height; row++) {
        for (int col = 0; col < src->width; col++) {
            int neighbors = count_neighbors(src, col, row);
            bool alive = grid_get(src, col, row);

            /* S23: живая клетка выживает только при 2 или 3 соседях. */
            if (alive) {
                if (neighbors == 2 || neighbors == 3) {
                    grid_set(dst, col, row, true);
                } else {
                    grid_set(dst, col, row, false);
                }
            } else {
                /* B3: мёртвая рождается ровно при трёх соседях. */
                if (neighbors == 3) {
                    grid_set(dst, col, row, true);
                } else {
                    grid_set(dst, col, row, false);
                }
            }
        }
    }

    return true;
}
