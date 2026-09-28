#ifndef UI_H
#define UI_H

/* Цвета */
#define UI_RESET   "\033[0m"
#define UI_BOLD    "\033[1m"
#define UI_DIM     "\033[2m"
#define UI_RED     "\033[31m"
#define UI_GREEN   "\033[32m"
#define UI_YELLOW  "\033[33m"
#define UI_CYAN    "\033[36m"

/* Очистка экрана + курсор в начало */
void ui_clear(void);

/* Приветствие */
void ui_banner(void);

/* Рамка меню. current_file — путь к текущему PNG. */
void ui_menu(const char *current_file);

/* Сообщения. Все пишут в соответствующий поток. */
void ui_error(const char *fmt, ...);   /* красный, stderr */
void ui_success(const char *fmt, ...); /* зелёный, stdout */
void ui_info(const char *fmt, ...);    /* cyan, stdout */
void ui_prompt(const char *fmt, ...);  /* жёлтый, stdout, без \n */

/* Пауза "нажмите Enter" */
void ui_pause(void);

#endif
