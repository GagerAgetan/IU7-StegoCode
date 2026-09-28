#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "png_io.h"
#include "stego.h"
#include "ui.h"
#include "files.h"

#define MAX_LINE 8192
#define MAX_PATH 1024

/* ---------- Обработка сигналов ---------- */

static void on_signal(int sig) {
    (void)sig;
    ui_leave();     /* вернуть терминал в нормальное состояние */
    _exit(1);
}

/* ---------- Ввод ---------- */

/* Проверяет, является ли строка ровно "q" (одна буква). */
static int is_quit(const char *s) {
    return s && s[0] == 'q' && s[1] == '\0';
}

/* Читает строку. Возвращает NULL при EOF. */
static char *read_line(const char *prompt) {
    static char buf[MAX_LINE];
    ui_prompt("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin)) return NULL;
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';
    return buf;
}

/* Разбирает "<строка>\0 <ключ>". Возвращает 0 при успехе. */
static int parse_input(const char *line, char **msg_out, char **key_out) {
    const char *sep = strstr(line, "\\0");
    if (!sep) {
        ui_error("во вводе нет разделителя \\0");
        return -1;
    }

    size_t msg_len = (size_t)(sep - line);
    const char *after = sep + 2;
    if (*after != ' ') {
        ui_error("после \\0 должен быть пробел");
        return -1;
    }
    after++;
    if (*after == '\0') {
        ui_error("пустой ключ");
        return -1;
    }

    char *msg = malloc(msg_len + 1);
    if (!msg) { ui_error("malloc"); return -1; }
    memcpy(msg, line, msg_len);
    msg[msg_len] = '\0';

    size_t klen = strlen(after);
    char *key = malloc(klen + 1);
    if (!key) { free(msg); ui_error("malloc"); return -1; }
    memcpy(key, after, klen + 1);

    *msg_out = msg;
    *key_out = key;
    return 0;
}

/* ---------- Действия ---------- */

static int do_encode(Image *img, const char *current_path) {
    char *line = read_line("  Введите строку (формат: <текст>\\0 <ключ>, q — назад) > ");
    if (!line) { ui_info("отменено"); return 0; }
    if (is_quit(line)) { ui_info("возврат в меню"); return 0; }

    char *msg = NULL, *key = NULL;
    if (parse_input(line, &msg, &key) < 0) return -1;

    if (stego_encode(img, msg, key) < 0) {
        free(msg);
        free(key);
        return -1;
    }
    free(msg);
    free(key);
    ui_success("строка закодирована в изображение");

    printf("\n");
    ui_prompt("  Сохранить в текущий файл (1) или в новый (2), q — назад? > ");
    fflush(stdout);
    char choice[16];
    if (!fgets(choice, sizeof(choice), stdin)) {
        ui_error("не удалось прочитать выбор");
        return -1;
    }
    size_t n = strlen(choice);
    if (n > 0 && choice[n - 1] == '\n') choice[n - 1] = '\0';
    if (is_quit(choice)) { ui_info("возврат в меню"); return 0; }

    if (choice[0] == '1') {
        if (image_save(current_path, img) < 0) return -1;
        ui_success("сохранено в '%s'", current_path);
        return 0;
    }

    char *name = read_line("  Введите имя нового файла (q — назад) > ");
    if (!name) { ui_info("отменено"); return 0; }
    if (is_quit(name)) { ui_info("возврат в меню"); return 0; }

    char path[MAX_PATH];
    size_t nlen = strlen(name);
    if (nlen >= 4 && strcmp(name + nlen - 4, ".png") == 0) {
        snprintf(path, sizeof(path), "%s", name);
    } else {
        snprintf(path, sizeof(path), "%s.png", name);
    }

    if (image_save(path, img) < 0) return -1;
    ui_success("сохранено в '%s'", path);
    return 0;
}

static int do_decode(const Image *img) {
    char *key = read_line("  Введите ключ (q — назад) > ");
    if (!key) { ui_info("отменено"); return 0; }
    if (is_quit(key)) { ui_info("возврат в меню"); return 0; }

    char *text = stego_decode(img, key);
    if (!text) return -1;

    printf("\n");
    printf(UI_GREEN UI_BOLD "  Расшифрованная строка:" UI_RESET "\n");
    printf(UI_BOLD "  \"%s\"" UI_RESET "\n\n", text);
    free(text);
    return 0;
}

static int do_change_file(Image *img, char *current_path, size_t cap) {
    char *name = read_line("  Введите путь к PNG (q — назад) > ");
    if (!name) { ui_info("отменено"); return 0; }
    if (is_quit(name)) { ui_info("возврат в меню"); return 0; }

    FILE *probe = fopen(name, "rb");
    if (!probe) {
        ui_error("файл '%s' не найден или недоступен для чтения", name);
        return -1;
    }
    fclose(probe);

    Image new_img;
    if (image_load(name, &new_img) < 0) return -1;

    image_free(img);
    *img = new_img;
    snprintf(current_path, cap, "%s", name);

    ui_success("файл загружен: %s (%dx%d)",
               current_path, img->width, img->height);
    return 0;
}

/* Показывает список PNG в текущем каталоге и даёт выбрать файл. */
static int do_list_files(Image *img, char *current_path, size_t cap) {
    size_t count = 0;
    char **list = files_list_png(&count);
    if (!list && count == 0) {
        ui_error("не удалось прочитать содержимое каталога");
        return -1;
    }

    ui_file_list(list, count, current_path);

    if (count == 0) {
        files_free_list(list, count);
        return 0;
    }

    printf("\n");
    ui_prompt("  Выбрать файл по номеру (Enter — назад) > ");
    fflush(stdout);

    char buf[32];
    if (!fgets(buf, sizeof(buf), stdin)) {
        files_free_list(list, count);
        return -1;
    }
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';

    if (buf[0] == '\0') {
        files_free_list(list, count);
        ui_info("возврат в меню");
        return 0;
    }

    char *end = NULL;
    long idx = strtol(buf, &end, 10);
    if (*end != '\0' || idx < 1 || (size_t)idx > count) {
        files_free_list(list, count);
        ui_error("'%s' — не номер из списка", buf);
        return -1;
    }

    const char *chosen = list[(size_t)idx - 1];

    Image new_img;
    if (image_load(chosen, &new_img) < 0) {
        files_free_list(list, count);
        return -1;
    }

    image_free(img);
    *img = new_img;
    snprintf(current_path, cap, "%s", chosen);

    ui_success("файл загружен: %s (%dx%d)",
               current_path, img->width, img->height);

    files_free_list(list, count);
    return 0;
}

/* ---------- main ---------- */

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Использование: %s [файл.png]\n", argv[0]);
        return 1;
    }
    srand((unsigned)time(NULL));

    /* Восстановление терминала при любом завершении. */
    atexit(ui_leave);
    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    /* Загружаем файл, если передан аргументом. */
    char current_path[MAX_PATH] = {0};
    Image img = {0};
    int early_error = 0;

    if (argc == 2) {
        if (image_load(argv[1], &img) == 0) {
            snprintf(current_path, sizeof(current_path), "%s", argv[1]);
        } else {
            early_error = 1;
        }
    }

    /* Входим в alt screen. */
    ui_enter();

    if (early_error) {
        ui_error("не удалось загрузить '%s'", argv[1]);
        ui_pause();
    }

    ui_clear();
    ui_banner();

    int running = 1;
    while (running) {
        int has_file = (img.data != NULL);
        char title[MAX_PATH + 64];
        if (has_file) {
            snprintf(title, sizeof(title), "%s (%dx%d)",
                     current_path, img.width, img.height);
        } else {
            snprintf(title, sizeof(title), "(файл не выбран)");
        }

        ui_menu(title);
        ui_prompt("  Выбор (0-4, q — выход) > ");
        char buf[32];
        if (!fgets(buf, sizeof(buf), stdin)) break;

        size_t n = strlen(buf);
        if (n > 0 && buf[n - 1] == '\n') buf[n - 1] = '\0';

        /* Дренаж на случай, если строка не влезла в буфер. */
        if (!strchr(buf, '\n') && n >= sizeof(buf) - 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
        }

        printf("\n");

        if (is_quit(buf)) { running = 0; break; }

        switch (buf[0]) {
        case '0':
            running = 0;
            break;

        case '1':
            if (!has_file) {
                ui_error("сначала выберите файл (пункт 3 или 4)");
                ui_pause();
                break;
            }
            do_decode(&img);
            ui_pause();
            break;

        case '2':
            if (!has_file) {
                ui_error("сначала выберите файл (пункт 3 или 4)");
                ui_pause();
                break;
            }
            do_encode(&img, current_path);
            ui_pause();
            break;

        case '3':
            do_change_file(&img, current_path, sizeof(current_path));
            ui_pause();
            break;

        case '4':
            do_list_files(&img, current_path, sizeof(current_path));
            ui_pause();
            break;

        default:
            ui_error("'%s' — не команда. Введите 0-4 или q", buf);
            ui_pause();
            break;
        }

        ui_clear();   /* стираем alt screen — меню перерисуется в начале цикла */
    }

    image_free(&img);
    /* ui_leave вызовется через atexit — терминал вернётся в исходное состояние */
    return 0;
}