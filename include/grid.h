#ifndef GRID_H
#define GRID_H

#include <stdbool.h>

/*
 * Прямоугольное поле клеток для игры «Жизнь» Конвея.
 * Состояние клетки: false — мёртвая, true — живая.
 * Хранение по строкам: индекс = row * width + col; в памяти — 0 или 1 в unsigned int.
 *
 * Чтение вне поля даёт false (мёртвая); запись вне поля игнорируется.
 * Так же ведёт себя life_step при подсчёте соседей у края (мёртвый внешний мир).
 */
typedef struct Grid {
    int width;
    int height;
    unsigned int *cells;
} Grid;

/* --- Создание / освобождение --- */
Grid *grid_create(int width, int height);
void grid_free(Grid *grid);

/* --- Чтение и запись клетки (с учётом границ поля) --- */
bool grid_get(const Grid *grid, int col, int row);
void grid_set(Grid *grid, int col, int row, bool alive);

/* Все клетки в мёртвое состояние. */
void grid_clear(Grid *grid);

/* --- Сравнение и копирование (для тестов и модуля cycle) --- */
/* Копирование содержимого; ширина и высота должны совпадать. true — успех, false — ошибка. */
bool grid_copy(Grid *dst, const Grid *src);

/* true, если размеры и все клетки совпадают. */
bool grid_equal(const Grid *first, const Grid *second);

#endif
