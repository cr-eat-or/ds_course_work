/*
 * История полных снимков поля: поиск повтора (цикла) через grid_equal.
 * visit_seq / snap_visit — чтобы период цикла считался между соседними визитами одной конфигурации.
 */
#include "cycle.h"

#include <stddef.h>
#include <stdlib.h>

/* Контекст: динамический массив копий Grid и параллельно номер «обращения» push для каждого снимка. */
struct CycleCtx {
    Grid **snaps;
    size_t *snap_visit;
    size_t len;
    size_t cap;
    size_t max_snapshots;
    size_t visit_seq;
    int width;
    int height;
    bool have_size;
};

/* Удаляет все Grid-снимки и обнуляет len; массивы указателей и snap_visit остаются (ёмкость cap). */
static void free_all_snaps(CycleCtx *ctx)
{
    if (ctx == NULL || ctx->snaps == NULL) {
        return;
    }

    for (size_t snap_idx = 0; snap_idx < ctx->len; snap_idx++) {
        grid_free(ctx->snaps[snap_idx]);
        ctx->snaps[snap_idx] = NULL;
    }
    ctx->len = 0;
    /* Массивы snaps/snap_visit не free — переиспользуем после cycle_reset. */
}

/* max_snapshots == 0: без лимита длины истории (кроме памяти). */
CycleCtx *cycle_create(size_t max_snapshots)
{
    CycleCtx *ctx = malloc(sizeof(CycleCtx));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->snaps = NULL;
    ctx->snap_visit = NULL;
    ctx->len = 0;
    ctx->cap = 0;
    ctx->max_snapshots = max_snapshots;
    ctx->visit_seq = 0;
    ctx->width = 0;
    ctx->height = 0;
    ctx->have_size = false;
    return ctx;
}

/* Полное освобождение: снимки, массивы указателей и счётчиков. */
void cycle_free(CycleCtx *ctx)
{
    if (ctx == NULL) {
        return;
    }

    free_all_snaps(ctx);
    free(ctx->snaps);
    free(ctx->snap_visit);
    free(ctx);
}

/* Новая «сессия»: очистить снимки и забыть размер поля (следующий push задаёт width/height). */
void cycle_reset(CycleCtx *ctx)
{
    if (ctx == NULL) {
        return;
    }

    free_all_snaps(ctx);
    ctx->visit_seq = 0;
    ctx->have_size = false;
    ctx->width = 0;
    ctx->height = 0;
}

size_t cycle_history_len(const CycleCtx *ctx)
{
    if (ctx == NULL) {
        return 0;
    }

    return ctx->len;
}

/* Удвоение ёмкости массивов snaps и snap_visit. */
static bool grow_snaps(CycleCtx *ctx)
{
    size_t new_cap;

    if (ctx->cap == 0) {
        new_cap = 8;
    } else {
        new_cap = ctx->cap * 2;
    }

    Grid **resized = realloc(ctx->snaps, new_cap * sizeof(Grid *));
    if (resized == NULL) {
        return false;
    }

    ctx->snaps = resized;

    {
        size_t *vis = realloc(ctx->snap_visit, new_cap * sizeof(size_t));
        if (vis == NULL) {
            return false;
        }
        ctx->snap_visit = vis;
    }

    ctx->cap = new_cap;

    return true;
}

/* Глубокая копия поля для хранения в истории (отдельный Grid в куче). */
static Grid *snapshot_dup(const Grid *src) {
    Grid *copy = grid_create(src->width, src->height);
    if (copy == NULL) {
        return NULL;
    }

    if (!grid_copy(copy, src)) {
        grid_free(copy);
        return NULL;
    }

    return copy;
}

/*
 * Один логический «визит»: увеличить visit_seq, сравнить grid со всеми снимками.
 * Совпадение → CYCLE и период по разнице visit_seq; иначе при лимите/ошибке NEW или ERROR.
 */
CyclePushResult cycle_push(CycleCtx *ctx, const Grid *grid, size_t *period_out) {
    if (ctx == NULL || grid == NULL) {
        return CYCLE_PUSH_ERROR;
    }

    if (!ctx->have_size) {
        ctx->width = grid->width;
        ctx->height = grid->height;
        ctx->have_size = true;
    } else if (grid->width != ctx->width || grid->height != ctx->height) {
        return CYCLE_PUSH_ERROR;
    }

    ctx->visit_seq++;

    /* Поиск первого снимка с той же расстановкой (полное сравнение клеток). */
    for (size_t snap_idx = 0; snap_idx < ctx->len; snap_idx++) {
        if (grid_equal(grid, ctx->snaps[snap_idx])) {
            if (period_out != NULL) {
                *period_out = ctx->visit_seq - ctx->snap_visit[snap_idx];
            }
            /* Обновляем «якорь» времени для этой конфигурации — стабильный период при повторяющихся фигурах. */
            ctx->snap_visit[snap_idx] = ctx->visit_seq;

            return CYCLE_PUSH_CYCLE;
        }
    }

    /* Лимит числа хранимых снимков (0 = без лимита). */
    if (ctx->max_snapshots > 0 && ctx->len >= ctx->max_snapshots) {
        return CYCLE_PUSH_ERROR;
    }

    /* Расширить таблицу указателей при необходимости. */
    if (ctx->len >= ctx->cap && !grow_snaps(ctx)) {
        return CYCLE_PUSH_ERROR;
    }

    /* Новая уникальная конфигурация — дублируем поле в историю. */
    Grid *snap = snapshot_dup(grid);
    if (snap == NULL) {
        return CYCLE_PUSH_ERROR;
    }

    ctx->snaps[ctx->len] = snap;
    ctx->snap_visit[ctx->len] = ctx->visit_seq;
    ctx->len++;
    return CYCLE_PUSH_NEW;
}
