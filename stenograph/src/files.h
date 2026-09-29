#ifndef FILES_H
#define FILES_H

#include <stddef.h>
/* Возвращает malloc'нутый массив строк (имён PNG-файлов в текущем каталоге).
   *count — количество найденных файлов.
   Возвращает NULL при ошибке (и *count = 0). */
char **files_list_png(size_t *count);

/* Освобождает массив, полученный из files_list_png. */
void files_free_list(char **list, size_t count);

#endif
