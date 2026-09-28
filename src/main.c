#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "png_io.h"
#include "stego.h"
#include "ui.h"

#define MAX_LINE 8192
#define MAX_PATH 1024

/* ---- Ввод ---- */

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

/* ---- Действия ---- */

static int do_encode(Image *img, const char *current_path) {
    char *line = read_line("  Введите строку (формат: <текст>\\0 <ключ>) > ");
    if (!line) { ui_error("не удалось прочитать строку"); return -1; }

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
    ui_prompt("  Сохранить в текущий файл (1) или в новый (2)? > ");
    fflush(stdout);
    char choice[16];
    if (!fgets(choice, sizeof(choice), stdin)) {
        ui_error("не удалось прочитать выбор");
        return -1;
    }

    if (choice[0] == '1') {
        if (image_save(current_path, img) < 0) return -1;
        ui_success("сохранено в '%s'", current_path);
        return 0;
    }

    char *name = read_line("  Введите имя нового файла > ");
    if (!name || *name == '\0') {
        ui_error("пустое имя файла");
        return -1;
    }

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
    char *key = read_line("  Введите ключ > ");
    if (!key || *key == '\0') {
        ui_error("пустой ключ");
        return -1;
    }
    char *text = stego_decode(img, key);
    if (!text) return -1;

    printf("\n");
    printf(UI_GREEN UI_BOLD "  Расшифрованная строка:" UI_RESET "\n");
    printf(UI_BOLD "  \"%s\"" UI_RESET "\n\n", text);
    free(text);
    return 0;
}

/* Меняет текущий файл. Обновляет *img и current_path. */
static int do_change_file(Image *img, char *current_path, size_t cap) {
    char *name = read_line("  Введите путь к PNG > ");
    if (!name || *name == '\0') {
        ui_error("пустой путь");
        return -1;
    }

    /* Проверяем, что файл открывается для чтения. */
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

/* ---- main ---- */

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Использование: %s [файл.png]\n", argv[0]);
        return 1;
    }
    srand((unsigned)time(NULL));

    char current_path[MAX_PATH] = {0};
    Image img = {0};

    /* Если аргумент передан — пытаемся загрузить сразу. */
    if (argc == 2) {
        if (image_load(argv[1], &img) == 0) {
            snprintf(current_path, sizeof(current_path), "%s", argv[1]);
        } else {
            /* Не падаем — просто стартуем без файла. */
            ui_error("не удалось загрузить '%s', начнём без файла", argv[1]);
            ui_pause();
        }
    }

    int running = 1;
    while (running) {
        ui_clear();
        ui_banner();

        int has_file = (img.data != NULL);
        if (has_file) {
            char title[MAX_PATH + 64];
            snprintf(title, sizeof(title), "%s (%dx%d)",
                     current_path, img.width, img.height);
            ui_menu(title);
        } else {
            ui_menu("(файл не выбран)");
        }

        ui_prompt("  Выбор > ");
        char buf[32];
        if (!fgets(buf, sizeof(buf), stdin)) break;

        printf("\n");

        switch (buf[0]) {
        case '0':
            running = 0;
            break;

        case '1':
            if (!has_file) {
                ui_error("сначала выберите файл (пункт 3)");
                ui_pause();
                break;
            }
            do_decode(&img);
            ui_pause();
            break;

        case '2':
            if (!has_file) {
                ui_error("сначала выберите файл (пункт 3)");
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

        default:
            ui_error("неизвестный пункт меню: '%c'", buf[0]);
            ui_pause();
            break;
        }
    }

    ui_clear();
    ui_banner();
    printf(UI_CYAN "  До встречи!\n" UI_RESET);
    image_free(&img);
    return 0;
}