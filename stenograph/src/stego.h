#ifndef STEGO_H
#define STEGO_H

#include "png_io.h"

/* Кодирует строку msg (без '\0', он добавится автоматически) в изображение.
   key — ключ. Возвращает 0 при успехе, -1 при ошибке. */
int stego_encode(Image *img, const char *msg, const char *key);

/* Декодирует строку. Результат malloc'нут, вызывающий free().
   Возвращает NULL при ошибке. */
char *stego_decode(const Image *img, const char *key);

#endif
