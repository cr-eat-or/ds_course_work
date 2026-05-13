/*
 * Описание шаблонов: смещения (d_col, d_row) и bbox для центрирования.
 */
#include "templates.h"

#include <stddef.h>

typedef struct {
    int d_col;
    int d_row;
} TemplateCell;

typedef struct {
    const TemplateCell *cells;
    size_t cells_count;
    int bbox_w;
    int bbox_h;
    const char *name;
} TemplateDef;

/* Блок из 4 клеток */
static const TemplateCell pat_block[] = {
    {0, 0}, {1, 0}, {0, 1}, {1, 1},
};

/* Мигалка */
static const TemplateCell pat_blinker_h[] = {
    {0, 0}, {1, 0}, {2, 0},
};

/* Глайдер */
static const TemplateCell pat_glider[] = {
    {0, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2},
};

/* Крест из 5 клеток */
static const TemplateCell pat_cross[] = {
    {0, 1}, {1, 0}, {1, 1}, {1, 2}, {2, 1},
};

/* Вселенная  Кока */
static const TemplateCell pat_frame_box[] = {
    {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {1, 0}, {1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5},
    {7, 3}, {7, 4}, {7, 5}, {7, 6}, {7, 7}, {7, 8}, {8, 3}, {8, 4}, {8, 5}, {8, 6}, {8, 7}, {8, 8},
    {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {8, 0}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1},
    {0, 7}, {1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7}, {0, 8}, {1, 8}, {2, 8}, {3, 8}, {4, 8}, {5, 8},
};

/* Королевскя пчела */
static const TemplateCell pat_meta_glider[] = {
    {0, 3}, {0, 4}, {1, 3}, {1, 4}, {9, 3}, {10, 3}, {10, 2}, {10, 4}, {11, 3}, {11, 2}, {11, 1},
    {11, 4}, {11, 5}, {12, 1}, {12, 0}, {12, 5}, {12, 6}, {16, 3}, {16, 2}, {16, 4}, {17, 3}, {17, 2},
    {17, 4}, {20, 3}, {20, 4}, {21, 3}, {21, 4},
};

/* Жаба (период 2) */
static const TemplateCell pat_toad[] = {
    {1, 0}, {2, 0}, {3, 0}, {0, 1}, {1, 1}, {2, 1},
};

/* Маяк (период 2) */
static const TemplateCell pat_beacon[] = {
    {0, 0}, {1, 0}, {2, 2}, {3, 2},
};

/* R-пентомино */
static const TemplateCell pat_rpent[] = {
    {1, 0}, {2, 0}, {0, 1}, {1, 1}, {2, 2},
};

static const TemplateDef defs[TEMPLATE_COUNT] = {
    {pat_block, sizeof(pat_block) / sizeof(pat_block[0]), 2, 2, "блок"},
    {pat_blinker_h, sizeof(pat_blinker_h) / sizeof(pat_blinker_h[0]), 3, 1, "мигалка"},
    {pat_glider, sizeof(pat_glider) / sizeof(pat_glider[0]), 3, 3, "глайдер"},
    {pat_cross, sizeof(pat_cross) / sizeof(pat_cross[0]), 3, 3, "крест"},
    {pat_frame_box, sizeof(pat_frame_box) / sizeof(pat_frame_box[0]), 9, 9, "рамка"},
    {pat_meta_glider, sizeof(pat_meta_glider) / sizeof(pat_meta_glider[0]), 22, 7, "метаглайдеры"},
    {pat_toad, sizeof(pat_toad) / sizeof(pat_toad[0]), 4, 2, "жаба"},
    {pat_beacon, sizeof(pat_beacon) / sizeof(pat_beacon[0]), 4, 3, "маяк"},
    {pat_rpent, sizeof(pat_rpent) / sizeof(pat_rpent[0]), 3, 3, "R-пентомино"},
};

static bool valid_id(TemplateId template_id) {
    return (int)template_id >= 0 && (int)template_id < (int)TEMPLATE_COUNT;
}

const char *template_name(TemplateId template_id) {
    if (!valid_id(template_id)) {
        return NULL;
    }

    return defs[template_id].name;
}

bool template_fits(const Grid *grid, TemplateId template_id, int anchor_col, int anchor_row) {
    const TemplateDef *def;

    if (grid == NULL || !valid_id(template_id)) {
        return false;
    }

    def = &defs[template_id];
    for (size_t idx = 0; idx < def->cells_count; idx++) {
        int col = anchor_col + def->cells[idx].d_col;
        int row = anchor_row + def->cells[idx].d_row;
        if (col < 0 || row < 0 || col >= grid->width || row >= grid->height) {
            return false;
        }
    }

    return true;
}

bool template_apply(Grid *grid, TemplateId template_id, int anchor_col, int anchor_row) {
    const TemplateDef *def;

    if (grid == NULL || !valid_id(template_id)) {
        return false;
    }

    if (!template_fits(grid, template_id, anchor_col, anchor_row)) {
        return false;
    }

    def = &defs[template_id];
    for (size_t idx = 0; idx < def->cells_count; idx++) {
        grid_set(grid, anchor_col + def->cells[idx].d_col, anchor_row + def->cells[idx].d_row, true);
    }
    return true;
}

bool template_apply_center(Grid *grid, TemplateId template_id) {
    int anchor_col;
    int anchor_row;

    if (grid == NULL || !valid_id(template_id)) {
        return false;
    }

    anchor_col = (grid->width - defs[template_id].bbox_w) / 2;
    anchor_row = (grid->height - defs[template_id].bbox_h) / 2;

    return template_apply(grid, template_id, anchor_col, anchor_row);
}
