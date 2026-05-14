/*
 * Консольное приложение Жизнь: два буфера поля, пошаговая эволюция, ввод команд.
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "cycle.h"
#include "grid.h"
#include "life.h"
#include "templates.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <errno.h>
#include <time.h>
#endif

/* Размер поля по умолчанию */
#define WIDTH 32
#define HEIGHT 32
#define LINE_MAX 1024
#define K_CMD_MAX_PAIRS 256

/* Пауза между кадрами в командах a и z (миллисекунды), если не указано явно. */
#define UI_DEFAULT_DELAY_MS 100u
/* В z: максимум шагов без обнаружения цикла, чтобы не зависнуть на длинном транзиенте. */
#define RUN_UNTIL_CYCLE_STEP_LIMIT 10000000u

/* Буфер одного кадра: один fwrite вместо множества printf. */
#define FRAME_BUF_CAP (64 * 1024)

/* Пауза между анимированными кадрами (Windows: Sleep; POSIX: nanosleep). */
static void sleep_ms(unsigned ms) {
    if (ms == 0u) {
        return;
    }
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    {
        struct timespec ts;
        ts.tv_sec = (time_t)(ms / 1000u);
        ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
        while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
            /* остаток в ts; повтор при сигнале */
        }
    }
#endif
}

static size_t frame_put(char *buf, size_t cap, size_t pos, const char *fmt, ...) {
    va_list ap;
    int n;

    if (pos >= cap) {
        return pos;
    }
    va_start(ap, fmt);
    n = vsnprintf(buf + pos, cap - pos, fmt, ap);
    va_end(ap);
    if (n < 0) {
        return pos;
    }
    if ((size_t)n >= cap - pos) {
        return cap - 1;
    }
    return pos + (size_t)n;
}

/*
 * Сетка в буфер (по строкам поля).
 * Живая клетка — «██», мёртвая — «[]».
 * По x только первый и последний столбец (0 и w−1); по y — только первая и последняя строка (остальные строки - пробелы у ║).
 * Границы — Box Drawing (UTF-8); суффикс строки копировать целиком по байтам.
 */
#define GRID_ROW_SUFFIX "║\n"
#define GRID_ROW_SUFFIX_LEN (sizeof(GRID_ROW_SUFFIX) - 1u)

#define CELL_ALIVE "██"
#define CELL_ALIVE_LEN (sizeof(CELL_ALIVE) - 1u)
#define CELL_DEAD "[]"
#define CELL_DEAD_LEN (sizeof(CELL_DEAD) - 1u)

static size_t append_grid_rows(char *buf, size_t cap, size_t pos, const Grid *grid) {
    int row;
    int col;
    char rowbuf[1024];
    size_t rlen;
    int w;

    pos = frame_put(buf, cap, pos, "%s", "    ");
    {
        /* Ширина поля клеток в символах (██ / [] по 2 знака). */
        size_t inner_w = (size_t)(2 * grid->width);
        char lo_x[16];
        char hi_x[16];
        size_t lo_len;
        size_t hi_len;

        snprintf(lo_x, sizeof lo_x, "%d", 0);
        snprintf(hi_x, sizeof hi_x, "%d", grid->width - 1);
        lo_len = strlen(lo_x);
        hi_len = strlen(hi_x);

        if (strcmp(lo_x, hi_x) == 0u) {
            /* Один столбец — одна подпись по центру поля клеток. */
            size_t pad_l = (inner_w - lo_len) / 2u;
            size_t pad_r = inner_w - lo_len - pad_l;
            size_t i;

            for (i = 0; i < pad_l; i++) {
                pos = frame_put(buf, cap, pos, "%s", " ");
            }
            pos = frame_put(buf, cap, pos, "%s", lo_x);
            for (i = 0; i < pad_r; i++) {
                pos = frame_put(buf, cap, pos, "%s", " ");
            }
        } else if (lo_len + hi_len + 1u <= inner_w) {
            size_t pad = inner_w - lo_len - hi_len;
            size_t i;

            pos = frame_put(buf, cap, pos, "%s", lo_x);
            for (i = 0; i < pad; i++) {
                pos = frame_put(buf, cap, pos, "%s", " ");
            }
            pos = frame_put(buf, cap, pos, "%s", hi_x);
        } else {
            pos = frame_put(buf, cap, pos, "%s%s", lo_x, hi_x);
        }
    }
    pos = frame_put(buf, cap, pos, "\n");

    pos = frame_put(buf, cap, pos, "%s", "   ╔");
    for (col = 0; col < grid->width; col++) {
        pos = frame_put(buf, cap, pos, "%s", "══");
    }
    pos = frame_put(buf, cap, pos, "%s", "╗\n");

    for (row = 0; row < grid->height; row++) {
        if (row == 0 || row == grid->height - 1) {
            w = snprintf(rowbuf, sizeof rowbuf, "%3d║", row);
        } else {
            w = snprintf(rowbuf, sizeof rowbuf, "%s", "   ║");
        }
        if (w < 0 || (size_t)w >= sizeof rowbuf) {
            continue;
        }
        rlen = (size_t)w;
        for (col = 0; col < grid->width; col++) {
            if (grid_get(grid, col, row)) {
                if (rlen + CELL_ALIVE_LEN >= sizeof rowbuf) {
                    break;
                }
                memcpy(rowbuf + rlen, CELL_ALIVE, CELL_ALIVE_LEN);
                rlen += CELL_ALIVE_LEN;
            } else {
                if (rlen + CELL_DEAD_LEN >= sizeof rowbuf) {
                    break;
                }
                memcpy(rowbuf + rlen, CELL_DEAD, CELL_DEAD_LEN);
                rlen += CELL_DEAD_LEN;
            }
        }
        if (rlen + GRID_ROW_SUFFIX_LEN >= sizeof rowbuf) {
            continue;
        }
        memcpy(rowbuf + rlen, GRID_ROW_SUFFIX, GRID_ROW_SUFFIX_LEN);
        rlen += GRID_ROW_SUFFIX_LEN;
        rowbuf[rlen] = '\0';
        pos = frame_put(buf, cap, pos, "%s", rowbuf);
    }

    pos = frame_put(buf, cap, pos, "%s", "   ╚");
    for (col = 0; col < grid->width; col++) {
        pos = frame_put(buf, cap, pos, "%s", "══");
    }
    pos = frame_put(buf, cap, pos, "╝\n");
    return pos;
}

/* Подсчёт живых клеток для строки статуса и приглашения. */
static int population(const Grid *grid) {
    int n = 0;

    for (int row = 0; row < grid->height; row++) {
        for (int col = 0; col < grid->width; col++) {
            if (grid_get(grid, col, row)) {
                n++;
            }
        }
    }

    return n;
}

static void draw_scene(unsigned generation, const Grid *grid, size_t cycle_period, const char *info) {
    static char frame[FRAME_BUF_CAP];
    size_t pos = 0;
    int pop = population(grid);

    pos = frame_put(
        frame,
        FRAME_BUF_CAP,
        pos,
        "--- Игра «Жизнь» (B3/S23, край поля — мёртвые соседи) ---\n");
    pos = frame_put(
        frame,
        FRAME_BUF_CAP,
        pos,
        "Поле %dx%d  |  Поколение %u  |  Живых: %d\n",
        grid->width,
        grid->height,
        generation,
        pop);
    if (info != NULL && info[0] != '\0') {
        pos = frame_put(frame, FRAME_BUF_CAP, pos, "%s\n", info);
    }
    if (cycle_period > 0u) {
        pos = frame_put(
            frame,
            FRAME_BUF_CAP,
            pos,
            ">>> Повтор конфигурации: период %zu <<<\n",
            cycle_period);
    }
    pos = frame_put(frame, FRAME_BUF_CAP, pos, "\n");
    pos = append_grid_rows(frame, FRAME_BUF_CAP, pos, grid);
    pos = frame_put(
        frame,
        FRAME_BUF_CAP,
        pos,
        "\n██ — живая, [] — мёртвая   |   Справка:   ?\n");

    fwrite(frame, 1, pos, stdout);
    fflush(stdout);
}

static void print_help(void) {
    printf(
        "Команды:\n"
        "  Enter      — одно поколение\n"
        "  n          — то же (без лишнего текста в строке)\n"
        "  a N [мс]   — N поколений с паузой между кадрами (мс; по умол. 100; 0 — без паузы)\n"
        "  z [мс]     — шагать, пока не обнаружится цикл (пауза между кадрами; лимит шагов в коде)\n"
        "  1–9        — шаблоны 1..9 по центру (список имён — ниже)\n"
        "  t N c r    — шаблон N (1..9), якорь в точке (c,r)\n"
        "  k x y ...  — живые клетки: пары «столбец строка» (от 0)\n"
        "  p          — перерисовать поле\n"
        "  r          — новая партия\n"
        "  ?          — эта справка\n"
        "  q          — выход\n"
        "\n"
        "Шаблоны N: 1 блок, 2 мигалка, 3 глайдер, 4 крест, 5 вселенная Кока,\n"
        "            6 королевская пчела, 7 жаба, 8 маяк, 9 R-пентомино.\n");
}

/*
 * Логика одного поколения симуляции:
 *  1) эволюция life_step из *cur в *nxt;
 *  2) обмен указателей (новое поколение в *cur);
 *  3) учёт в истории cycle_push(*cur) - при повторе период в *cycle_note для отображения в кадре.
 */
static int game_step(CycleCtx *cycle, Grid **cur, Grid **nxt, unsigned *generation, size_t *cycle_note) {
    Grid *tmp;
    size_t period;
    CyclePushResult pr;

    if (cycle_note != NULL) {
        *cycle_note = 0;
    }

    if (!life_step(*cur, *nxt)) {
        fprintf(stderr, "Ошибка life_step.\n");
        return -1;
    }

    tmp = *cur;
    *cur = *nxt;
    *nxt = tmp;
    (*generation)++;

    pr = cycle_push(cycle, *cur, &period);
    if (pr == CYCLE_PUSH_ERROR) {
        fprintf(stderr, "Ошибка cycle_push (память или лимит истории).\n");
        return -1;
    }

    if (pr == CYCLE_PUSH_CYCLE) {
        if (cycle_note != NULL) {
            *cycle_note = period;
        } else {
            printf(
                "Текущее поле уже было в истории (период повтора %zu поколений).\n",
                period);
        }
    }

    return 0;
}

/* Пропуск пробелов/табуляции/CR при разборе строки команды. */
static void skip_spaces(char **p) {
    while (**p == ' ' || **p == '\t' || **p == '\r') {
        (*p)++;
    }
}

/*
 * После смены поля вручную (k), шаблоном по центру (1–9) или по якорю (t): сброс истории и снимок.
 */
static int cycle_reseed(CycleCtx *cycle, Grid *grid) {
    cycle_reset(cycle);
    return cycle_push(cycle, grid, NULL) == CYCLE_PUSH_NEW;
}

/* Приглашение ввода с кратким контекстом (поколение и число живых). */
static void print_prompt(unsigned gen, const Grid *grid) {
    printf("\n> [покол. %u | ж. %d] ", gen, population(grid));
}

int main(void) {
    /* cur - текущее отображаемое поколение, nxt - буфер для следующего life_step. */
    Grid *cur;
    Grid *nxt;
    CycleCtx *cycle;
    char line[LINE_MAX];
    char info_buf[160];

    unsigned gen = 0;
    int exit_code = 0;
    /* После game_step: ненулевой период, если конфигурация уже была в истории cycle. */
    size_t cycle_note = 0;

    cur = grid_create(WIDTH, HEIGHT);
    nxt = grid_create(WIDTH, HEIGHT);
    cycle = cycle_create(0);
    if (cur == NULL || nxt == NULL || cycle == NULL) {
        fprintf(stderr, "Не удалось выделить поле или историю.\n");
        grid_free(cur);
        grid_free(nxt);
        cycle_free(cycle);
        return 1;
    }

    draw_scene(gen, cur, 0u, "Ввод: Enter — шаг, ? — справка.");
    print_prompt(gen, cur);

    /* --- Главный цикл: чтение строки, разбор команды, действие, снова приглашение --- */
    while (1) {
        fflush(stdout);
        if (fgets(line, (int)sizeof line, stdin) == NULL) {
            printf("\nКонец ввода — выход.\n");
            break;
        }

        size_t ln = strlen(line);
        while (ln > 0 && (line[ln - 1] == '\n' || line[ln - 1] == '\r')) {
            line[--ln] = '\0';
        }

        char *p = line;
        /* UTF-8 BOM в начале строки. */
        if ((unsigned char)p[0] == 0xEFu && (unsigned char)p[1] == 0xBBu && (unsigned char)p[2] == 0xBFu) {
            p += 3;
        }

        skip_spaces(&p);
        /* Пустая строка или только пробелы — один шаг, как Enter. */
        if (*p == '\n' || *p == '\0') {
            if (game_step(cycle, &cur, &nxt, &gen, &cycle_note) != 0) {
                exit_code = 1;
                break;
            }

            draw_scene(gen, cur, cycle_note, NULL);
            print_prompt(gen, cur);
            continue;
        }

        /* Выход из программы. */
        if (*p == 'q') {
            printf("\nДо свидания.\n");
            break;
        }

        /* Справка по командам. */
        if (*p == '?') {
            print_help();
            print_prompt(gen, cur);
            continue;
        }

        /* Принудительная перерисовка без смены поколения. */
        if (*p == 'p') {
            draw_scene(gen, cur, 0u, NULL);
            print_prompt(gen, cur);
            continue;
        }

        /* Очистить оба буфера, сбросить историю циклов и счётчик поколений. */
        if (*p == 'r') {
            grid_clear(cur);
            grid_clear(nxt);
            cycle_reset(cycle);
            gen = 0;
            draw_scene(gen, cur, 0u, "Новая партия.");
            print_prompt(gen, cur);
            continue;
        }

        /* Явная команда «шаг» (только буква n без хвоста). */
        if (*p == 'n') {
            char *rest = p + 1;

            skip_spaces(&rest);
            if (*rest != '\n' && *rest != '\0') {
                printf("Лишний текст после «n». Для шага нажмите Enter или одну букву n.\n");
                print_prompt(gen, cur);
                continue;
            }

            if (game_step(cycle, &cur, &nxt, &gen, &cycle_note) != 0) {
                exit_code = 1;
                break;
            }

            draw_scene(gen, cur, cycle_note, NULL);
            print_prompt(gen, cur);
            continue;
        }

        /* Несколько поколений подряд с анимацией и паузой delay_ms между кадрами. */
        if (*p == 'a') {
            int advance_ok = 1;
            int n_adv;
            unsigned delay_ms = UI_DEFAULT_DELAY_MS;
            int nread = 0;
            char *r;

            p++;
            skip_spaces(&p);
            if (sscanf(p, "%d%n", &n_adv, &nread) != 1 || n_adv < 1) {
                printf("Формат: a N [задержка_мс]   пример: a 20 150\n");
                print_prompt(gen, cur);
                continue;
            }
            r = p + nread;
            skip_spaces(&r);
            if (*r != '\0') {
                int dread = 0;
                if (sscanf(r, "%u%n", &delay_ms, &dread) != 1) {
                    printf("Неверная задержка (ожидается целое число мс).\n");
                    print_prompt(gen, cur);
                    continue;
                }
                r += dread;
                skip_spaces(&r);
                if (*r != '\0') {
                    printf("Лишний текст после команды: «%s»\n", r);
                    print_prompt(gen, cur);
                    continue;
                }
            }

            for (int i = 0; i < n_adv; i++) {
                cycle_note = 0;
                if (game_step(cycle, &cur, &nxt, &gen, &cycle_note) != 0) {
                    exit_code = 1;
                    advance_ok = 0;
                    break;
                }
                snprintf(
                    info_buf,
                    sizeof info_buf,
                    "a: кадр %d/%d, пауза %u мс",
                    i + 1,
                    n_adv,
                    delay_ms);
                draw_scene(gen, cur, cycle_note, info_buf);
                sleep_ms(delay_ms);
            }

            if (!advance_ok) {
                break;
            }

            snprintf(info_buf, sizeof info_buf, "Прокручено поколений: %d.", n_adv);
            draw_scene(gen, cur, 0u, info_buf);
            print_prompt(gen, cur);
            continue;
        }

        /* Автоматический прогон до первого обнаруженного цикла (или до лимита шагов). */
        if (*p == 'z') {
            unsigned delay_ms = UI_DEFAULT_DELAY_MS;
            int consumed = 0;
            unsigned long steps_done = 0;

            p++;
            skip_spaces(&p);
            if (*p != '\0') {
                if (sscanf(p, "%u%n", &delay_ms, &consumed) != 1) {
                    printf("Формат: z [задержка_мс]\n");
                    print_prompt(gen, cur);
                    continue;
                }
                {
                    char *tail = p + consumed;
                    skip_spaces(&tail);
                    if (*tail != '\0') {
                        printf("Лишний текст: «%s»\n", tail);
                        print_prompt(gen, cur);
                        continue;
                    }
                }
            }

            while (steps_done < RUN_UNTIL_CYCLE_STEP_LIMIT) {
                cycle_note = 0;
                if (game_step(cycle, &cur, &nxt, &gen, &cycle_note) != 0) {
                    exit_code = 1;
                    break;
                }
                steps_done++;

                snprintf(
                    info_buf,
                    sizeof info_buf,
                    "z: шаг %lu, пауза %u мс (до первого повтора в истории)",
                    (unsigned long)steps_done,
                    delay_ms);
                draw_scene(gen, cur, cycle_note, info_buf);

                if (cycle_note > 0u) {
                    snprintf(
                        info_buf,
                        sizeof info_buf,
                        "Цикл найден за %lu шаг(ов).",
                        (unsigned long)steps_done);
                    draw_scene(gen, cur, cycle_note, info_buf);
                    break;
                }

                sleep_ms(delay_ms);
            }

            if (exit_code != 0) {
                break;
            }

            if (steps_done >= RUN_UNTIL_CYCLE_STEP_LIMIT && cycle_note == 0u) {
                snprintf(
                    info_buf,
                    sizeof info_buf,
                    "Лимит шагов %u: совпадение с историей не встретилось.",
                    (unsigned)RUN_UNTIL_CYCLE_STEP_LIMIT);
                draw_scene(gen, cur, 0u, info_buf);
            }

            print_prompt(gen, cur);
            continue;
        }

        /* Шаблон с якорем (левый верх bbox): t N столбец строка — как 1/2/3, но в заданной точке. */
        if (*p == 't' || *p == 'T') {
            int tnum;
            int ac;
            int ar;
            int nread;
            char *q = p + 1;
            TemplateId tid;

            skip_spaces(&q);
            if (sscanf(q, "%d %d %d%n", &tnum, &ac, &ar, &nread) != 3) {
                printf(
                    "Формат: t N столбец строка   — N от 1 до 9 (см. ?); координаты якоря от 0.\n");
                print_prompt(gen, cur);
                continue;
            }
            q += nread;
            skip_spaces(&q);
            if (*q != '\0') {
                printf("Лишний текст в конце команды: «%s»\n", q);
                print_prompt(gen, cur);
                continue;
            }
            if (tnum < 1 || tnum > (int)TEMPLATE_COUNT) {
                printf("N должно быть от 1 до %d.\n", (int)TEMPLATE_COUNT);
                print_prompt(gen, cur);
                continue;
            }

            tid = (TemplateId)(tnum - 1);
            if (!template_fits(cur, tid, ac, ar)) {
                printf(
                    "Шаблон «%s» с якорем (%d, %d) не помещается в поле %dx%d.\n",
                    template_name(tid) != NULL ? template_name(tid) : "?",
                    ac,
                    ar,
                    cur->width,
                    cur->height);
                print_prompt(gen, cur);
                continue;
            }
            if (!template_apply(cur, tid, ac, ar)) {
                fprintf(stderr, "Ошибка template_apply.\n");
                exit_code = 1;
                break;
            }
            if (!cycle_reseed(cycle, cur)) {
                fprintf(stderr, "Не удалось записать начальный снимок в историю.\n");
                exit_code = 1;
                break;
            }

            snprintf(
                info_buf,
                sizeof info_buf,
                "Шаблон «%s», якорь (%d, %d).",
                template_name(tid) != NULL ? template_name(tid) : "?",
                ac,
                ar);
            draw_scene(gen, cur, 0u, info_buf);
            print_prompt(gen, cur);
            continue;
        }

        /* Быстрая вставка шаблона по центру: одна цифра 1–9. */
        {
            TemplateId tid_quick = TEMPLATE_COUNT;
            char *tail;

            if (*p >= '1' && *p <= '9'
                && (p[1] == '\0' || p[1] == ' ' || p[1] == '\t')) {
                tid_quick = (TemplateId)(*p - '1');
                tail = p + 1;
            }

            if (tid_quick != TEMPLATE_COUNT) {
                skip_spaces(&tail);
                if (*tail != '\0') {
                    printf("Лишний текст после номера шаблона.\n");
                    print_prompt(gen, cur);
                    continue;
                }
                if (!template_apply_center(cur, tid_quick)) {
                    printf("Шаблон «%s» не помещается по центру.\n",
                           template_name(tid_quick) != NULL ? template_name(tid_quick) : "?");
                    print_prompt(gen, cur);
                } else {
                    if (!cycle_reseed(cycle, cur)) {
                        fprintf(stderr, "Не удалось записать начальный снимок в историю.\n");
                        exit_code = 1;
                        break;
                    }

                    snprintf(
                        info_buf,
                        sizeof info_buf,
                        "Шаблон: %s.",
                        template_name(tid_quick) != NULL ? template_name(tid_quick) : "?");
                    draw_scene(gen, cur, 0u, info_buf);
                    print_prompt(gen, cur);
                }
                continue;
            }
        }

        /* Ручная расстановка: пары координат (столбец, строка), затем cycle_reseed. */
        if (*p == 'k') {
            int cols[K_CMD_MAX_PAIRS];
            int rows[K_CMD_MAX_PAIRS];
            int col;
            int row;
            int n_chars;

            char *q = p + 1;
            skip_spaces(&q);
            int pairs_count = 0;

            while (pairs_count < K_CMD_MAX_PAIRS) {
                if (sscanf(q, "%d %d%n", &col, &row, &n_chars) != 2) {
                    break;
                }

                q += n_chars;
                skip_spaces(&q);

                if (col < 0 || col >= cur->width || row < 0 || row >= cur->height) {
                    printf(
                        "Вне поля: (%d, %d). Допустимо: столбец 0..%d, строка 0..%d.\n",
                        col,
                        row,
                        cur->width - 1,
                        cur->height - 1);
                    pairs_count = -1;
                    break;
                }

                cols[pairs_count] = col;
                rows[pairs_count] = row;
                pairs_count++;
            }

            if (pairs_count < 0) {
                print_prompt(gen, cur);
                continue;
            }

            skip_spaces(&q);
            if (*q != '\0') {
                printf("Лишний текст в конце команды: «%s»\n", q);
                print_prompt(gen, cur);
                continue;
            }

            if (pairs_count == 0) {
                printf("Укажите пары чисел: k столбец строка [столбец строка ...]\n");
                print_prompt(gen, cur);
                continue;
            }

            for (int i = 0; i < pairs_count; i++) {
                grid_set(cur, cols[i], rows[i], true);
            }

            if (!cycle_reseed(cycle, cur)) {
                fprintf(stderr, "Не удалось записать начальный снимок в историю.\n");
                exit_code = 1;
                break;
            }

            snprintf(info_buf, sizeof info_buf, "Отмечено живыми клеток: %d.", pairs_count);
            draw_scene(gen, cur, 0u, info_buf);
            print_prompt(gen, cur);
            continue;
        }

        printf("Неизвестная команда. Нажмите ? для справки.\n");
        print_prompt(gen, cur);
    }

    /* Завершение: освободить историю и оба буфера поля. */
    cycle_free(cycle);
    grid_free(cur);
    grid_free(nxt);
    return exit_code;
}
