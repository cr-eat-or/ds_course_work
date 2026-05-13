/*
 * Реализация поля: выделение памяти, доступ по (col,row), очистка, memcpy/memcmp для снимков.
 */
#include "grid.h"

#include <stdlib.h>
#include <string.h>

/* Проверка, что (col,row) лежит внутри поля - для безопасного get/set. */
static bool in_bounds(const Grid *grid, int col, int row) {
    return col >= 0 && row >= 0 && col < grid->width && row < grid->height;
}

/* Выделяет структуру и массив клеток (calloc -> все мёртвые). Некорректный размер -> NULL. */
Grid *grid_create(int width, int height) {
    Grid *grid;
    size_t cell_count;

    if (width <= 0 || height <= 0) {
        return NULL;
    }

    grid = malloc(sizeof(Grid));
    if (grid == NULL) {
        return NULL;
    }

    /* size_t при умножении, чтобы избежать переполнения int на больших полях. */
    cell_count = (size_t)width * (size_t)height;
    grid->cells = calloc(cell_count, sizeof(unsigned int));
    if (grid->cells == NULL) {
        free(grid);
        return NULL;
    }

    grid->width = width;
    grid->height = height;
    return grid;
}

/* Освобождает массив клеток и саму структуру. */
void grid_free(Grid *grid) {
    if (grid == NULL) {
        return;
    }

    free(grid->cells);
    free(grid);
}

/*
 * Чтение клетки. Вне границ - false (согласовано с подсчётом соседей у края в life.c).
 */
bool grid_get(const Grid *grid, int col, int row) {
    if (!in_bounds(grid, col, row)) {
        return false;
    }

    return grid->cells[(size_t)row * (size_t)grid->width + (size_t)col] != 0;
}

/* Запись: вне поля - тихий no-op. */
void grid_set(Grid *grid, int col, int row, bool alive) {
    if (!in_bounds(grid, col, row)) {
        return;
    }

    grid->cells[(size_t)row * (size_t)grid->width + (size_t)col] = alive ? 1 : 0;
}

/* Обнуление всего массива клеток одним memset. */
void grid_clear(Grid *grid) {
    size_t cell_count;

    if (grid == NULL || grid->cells == NULL) {
        return;
    }

    cell_count = (size_t)grid->width * (size_t)grid->height;
    memset(grid->cells, 0, cell_count * sizeof(unsigned int));
}

/* Полное копирование буфера клеток при совпадении размеров (для снимков в cycle). */
bool grid_copy(Grid *dst, const Grid *src) {
    size_t cell_count;

    if (dst == NULL || src == NULL) {
        return false;
    }

    if (dst->width != src->width || dst->height != src->height) {
        return false;
    }

    cell_count = (size_t)src->width * (size_t)src->height;
    memcpy(dst->cells, src->cells, cell_count * sizeof(unsigned int));
    return true;
}

/* Побайтовое сравнение массивов клеток при совпадении размеров. */
bool grid_equal(const Grid *first, const Grid *second) {
    size_t cell_count;

    if (first == NULL || second == NULL) {
        return false;
    }

    if (first->width != second->width || first->height != second->height) {
        return false;
    }

    cell_count = (size_t)first->width * (size_t)first->height;
    return memcmp(first->cells, second->cells, cell_count * sizeof(unsigned int)) == 0;
}
