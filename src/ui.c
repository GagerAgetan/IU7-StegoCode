#include "ui.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>   /* isatty */

static int ui_is_tty(void) {
    return isatty(fileno(stdout));
}

void ui_clear(void) {
    if (!ui_is_tty()) return;
    printf("\033[2J\033[3J\033[H");
    fflush(stdout);
}

void ui_banner(void) {
    printf(UI_CYAN UI_BOLD);
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║        LSB Steganography Tool (PNG)          ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
    printf(UI_RESET);
}

void ui_menu(const char *current_file) {
    printf("\n");
    printf(UI_CYAN "  Текущий файл: " UI_RESET UI_BOLD "%s" UI_RESET "\n",
           current_file ? current_file : "(нет)");
    printf(UI_CYAN "  ──────────────────────────────────────────\n" UI_RESET);
    printf("   " UI_BOLD "1" UI_RESET " — Считать данные из файла\n");
    printf("   " UI_BOLD "2" UI_RESET " — Записать строку\n");
    printf("   " UI_BOLD "3" UI_RESET " — Сменить файл\n");
    printf("   " UI_BOLD "0" UI_RESET " — Выход\n");
    printf(UI_CYAN "  ──────────────────────────────────────────\n" UI_RESET);
}

static void vprint_colored(const char *color, const char *prefix,
                           const char *fmt, va_list ap, FILE *stream) {
    fprintf(stream, "%s%s", color, prefix ? prefix : "");
    vfprintf(stream, fmt, ap);
    fprintf(stream, UI_RESET "\n");
    fflush(stream);
}

void ui_error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint_colored(UI_RED UI_BOLD, "[ОШИБКА] ", fmt, ap, stdout);  /* stdout! */
    va_end(ap);
}

void ui_success(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint_colored(UI_GREEN UI_BOLD, "[OK] ", fmt, ap, stdout);
    va_end(ap);
}

void ui_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprint_colored(UI_CYAN, "[i] ", fmt, ap, stdout);
    va_end(ap);
}

void ui_prompt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf(UI_YELLOW);
    vprintf(fmt, ap);
    printf(UI_RESET);
    fflush(stdout);
    va_end(ap);
}

void ui_pause(void) {
    printf(UI_DIM "\n  Нажмите Enter, чтобы продолжить..." UI_RESET);
    fflush(stdout);
    char buf[16];
    if (!fgets(buf, sizeof(buf), stdin)) return;
    if (!strchr(buf, '\n')) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
    }
}