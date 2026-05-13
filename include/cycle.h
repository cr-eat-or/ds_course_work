#ifndef CYCLE_H
#define CYCLE_H

#include "grid.h"

#include <stddef.h>

/*
 * Модуль истории: каждый успешный push (NEW) хранит полную копию поля.
 * Повтор конфигурации — сравнение с любым ранее сохранённым снимком (grid_equal).
 *
 * max_snapshots == 0 — без ограничения числа снимков (ограничена только памятью).
 */

typedef struct CycleCtx CycleCtx;

/* Результат одной попытки записать текущее поле в историю. */
typedef enum {
    CYCLE_PUSH_NEW = 0,   /* новое состояние записано в историю */
    CYCLE_PUSH_CYCLE = 1, /* совпадение с более ранним снимком; *period_out задан */
    CYCLE_PUSH_ERROR = 2 /* NULL, несовпадение размера с первым снимком, лимит истории, malloc */
} CyclePushResult;

CycleCtx *cycle_create(size_t max_snapshots);
void cycle_free(CycleCtx *ctx);

/* Удалить все снимки; следующий push задаёт размер поля заново. */
void cycle_reset(CycleCtx *ctx);

/*
 * Сохраняет копию grid и сравнивает с предыдущими снимками.
 * При CYCLE_PUSH_CYCLE: *period_out — число обращений к cycle_push между
 * текущим совпадением и предыдущим учётом этого же снимка (включая текущее обращение).
 */
CyclePushResult cycle_push(CycleCtx *ctx, const Grid *grid, size_t *period_out);

/* Число снимков в истории (после успешных push с результатом NEW). */
size_t cycle_history_len(const CycleCtx *ctx);

#endif
