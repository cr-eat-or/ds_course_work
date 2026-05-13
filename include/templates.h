#ifndef TEMPLATES_H
#define TEMPLATES_H

#include "grid.h"

/*
 * Паттерны как смещения относительно якоря (левый верх bbox).
 * template_apply только включает клетки; остальное поле не трогает.
 */

typedef enum TemplateId {
    TEMPLATE_BLOCK = 0,
    TEMPLATE_BLINKER_H,
    TEMPLATE_GLIDER,
    TEMPLATE_CROSS,
    TEMPLATE_FRAME_BOX,
    TEMPLATE_META_GLIDER,
    TEMPLATE_TOAD,
    TEMPLATE_BEACON,
    TEMPLATE_RPENTOMINO,
    TEMPLATE_COUNT
} TemplateId;

const char *template_name(TemplateId template_id);

bool template_fits(const Grid *grid, TemplateId template_id, int anchor_col, int anchor_row);

bool template_apply(Grid *grid, TemplateId template_id, int anchor_col, int anchor_row);

bool template_apply_center(Grid *grid, TemplateId template_id);

#endif
