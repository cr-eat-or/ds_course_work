/*
 * Тесты шаблонов: имена, границы, apply/fits/center, согласованность с life (мигалка).
 */
#include "grid.h"
#include "life.h"
#include "templates.h"
#include "test_helpers.h"

#include <stddef.h>
#include <stdio.h>

int test_templates_run(void) {
    Grid *grid;
    Grid *nxt;
    int failed = 0;

    failed += !expect_true(template_name((TemplateId)(-1)) == NULL, "template_name неверный id");
    failed += !expect_true(template_name(TEMPLATE_COUNT) == NULL, "template_name за диапазоном");
    failed += !expect_true(template_name(TEMPLATE_BLOCK) != NULL, "template_name блок");

    /* Защита от NULL и неверных идентификаторов. */
    failed += !expect_true(!template_apply(NULL, TEMPLATE_BLOCK, 0, 0), "apply NULL grid");
    failed += !expect_true(!template_apply_center(NULL, TEMPLATE_BLOCK), "apply_center NULL");

    grid = grid_create(3, 3);
    failed += !expect_true(grid != NULL, "grid_create 3x3 для id");
    if (grid == NULL) {
        return failed;
    }

    failed += !expect_true(!template_apply(grid, (TemplateId)999, 0, 0), "apply неверный id");
    grid_free(grid);

    grid = grid_create(2, 2);
    failed += !expect_true(grid != NULL, "grid_create 2x2");
    if (grid == NULL) {
        return failed;
    }

    failed += !expect_true(template_fits(grid, TEMPLATE_BLOCK, 0, 0), "блок помещается");
    failed += !expect_true(!template_fits(grid, TEMPLATE_GLIDER, 0, 0), "глайдер не помещается 2x2");
    failed += !expect_true(template_apply(grid, TEMPLATE_BLOCK, 0, 0), "apply блок");
    failed += !expect_true(grid_get(grid, 0, 0) && grid_get(grid, 1, 0)
                          && grid_get(grid, 0, 1) && grid_get(grid, 1, 1),
                          "все клетки блока живые");
    grid_free(grid);

    grid = grid_create(1, 1);
    failed += !expect_true(grid != NULL, "grid_create 1x1");
    if (grid == NULL) {
        return failed;
    }

    failed += !expect_true(!template_apply(grid, TEMPLATE_BLOCK, 0, 0), "блок не влезает");
    failed += !expect_true(!grid_get(grid, 0, 0), "поле не менялось при ошибке");
    grid_free(grid);

    grid = grid_create(7, 7);
    failed += !expect_true(grid != NULL, "grid_create 7x7");
    if (grid == NULL) {
        return failed;
    }

    failed += !expect_true(template_apply_center(grid, TEMPLATE_GLIDER), "глайдер по центру");
    failed += !expect_true(grid_get(grid, 2, 2), "глайдер: клетка угла");
    grid_free(grid);

    /* Мигалка по центру + один шаг life -> вертикаль. */
    grid = grid_create(5, 5);
    failed += !expect_true(grid != NULL, "grid_create 5x5 мигалка");
    if (grid == NULL) {
        return failed;
    }
    
    failed += !expect_true(template_apply_center(grid, TEMPLATE_BLINKER_H), "мигалка центр");

    nxt = grid_create(5, 5);
    failed += !expect_true(nxt != NULL, "grid_create nxt");
    if (nxt == NULL) {
        grid_free(grid);
        return failed;
    }
    
    failed += !expect_true(life_step(grid, nxt), "life_step после шаблона");
    failed += !expect_true(!grid_get(nxt, 2, 0) && grid_get(nxt, 2, 1)
                          && grid_get(nxt, 2, 2) && grid_get(nxt, 2, 3)
                          && !grid_get(nxt, 2, 4),
                          "мигалка стала вертикалью");
    grid_free(nxt);
    grid_free(grid);

    if (failed == 0) {
        printf("test_templates: OK\n");
    }
    return failed;
}
