#include "png_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int image_load(const char *path, Image *out) {
    int w, h, channels;
    /* Просим 3 канала — RGB. stb сам конвертирует grayscale/RGBA. */
    unsigned char *data = stbi_load(path, &w, &h, &channels, 3);
    if (!data) {
        fprintf(stderr, "Ошибка: не удалось загрузить PNG '%s': %s\n",
                path, stbi_failure_reason());
        return -1;
    }
    out->data   = data;
    out->width  = w;
    out->height = h;
    return 0;
}

int image_save(const char *path, const Image *img) {
    int ok = stbi_write_png(path, img->width, img->height, 3,
                            img->data, img->width * 3);
    if (!ok) {
        fprintf(stderr, "Ошибка: не удалось сохранить PNG '%s'\n", path);
        return -1;
    }
    return 0;
}

void image_free(Image *img) {
    if (img && img->data) {
        stbi_image_free(img->data);
        img->data = NULL;
    }
}

int image_copy(const Image *src, Image *dst) {
    size_t sz = (size_t)src->width * (size_t)src->height * 3u;
    unsigned char *buf = malloc(sz);
    if (!buf) return -1;
    memcpy(buf, src->data, sz);
    dst->data = buf;
    dst->width = src->width;
    dst->height = src->height;
    return 0;
}