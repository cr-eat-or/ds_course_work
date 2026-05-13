#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdbool.h>
#include <stdio.h>

/*
 * Общий макрос-паттерн для юнит-тестов без внешней библиотеки.
 *
 * Условие должно быть истинным.
 * Возвращает true, если проверка прошла; false — если условие ложно (печать FAILED).
 * Счётчик ошибок: failed += !expect_true(...).
 */
static bool expect_true(bool condition, const char *name) {
    if (!condition) {
        printf("FAILED: %s\n", name);
        return false;
    }

    return true;
}

#endif
