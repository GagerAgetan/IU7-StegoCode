#ifndef PNG_IO_H
#define PNG_IO_H

#include <stddef.h>

typedef struct {
    unsigned char *data;   /* RGB, 3 байта на пиксель */
    int width;
    int height;
} Image;

/* Загружает PNG. Возвращает 0 при успехе, -1 при ошибке (пишет в stderr). */
int  image_load(const char *path, Image *out);

/* Сохраняет PNG. Возвращает 0 при успехе, -1 при ошибке. */
int  image_save(const char *path, const Image *img);

void image_free(Image *img);

int image_copy(const Image *src, Image *dst);

#endif
