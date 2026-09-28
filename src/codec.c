#include "codec.h"

#include <stdio.h>
#include <string.h>

int codec_key_params(const char *key, int width, int height, KeyParams *out) {
    size_t len = strlen(key);
    if (len == 0) {
        fprintf(stderr, "Ошибка: пустой ключ\n");
        return -1;
    }
    if ((unsigned char)key[0] == 0 || (unsigned char)key[len - 1] == 0) {
        fprintf(stderr, "Ошибка: первый или последний символ ключа — нуль\n");
        return -1;
    }

    long draft = 0;
    for (size_t i = 0; i < len; ++i) {
        draft += (unsigned char)key[i];
    }

    out->draft = draft;
    out->shift = (int)(draft % 128);
    out->x0    = (int)((draft / (unsigned char)key[0]) % width);
    out->y0    = (int)((draft / (unsigned char)key[len - 1]) % height);
    return 0;
}

unsigned char codec_encode_char(unsigned char c, int shift, int *carry) {
    int sum = (int)c + shift;
    if (sum >= 128) {
        *carry = 1;
        sum -= 128;
    } else {
        *carry = 0;
    }
    return (unsigned char)sum;
}

unsigned char codec_decode_char(unsigned char e, int shift, int carry) {
    (void)carry; /* carry — избыточность, но проверяем в stego.c */
    int c = (int)e - shift;
    if (c < 0) c += 128;
    return (unsigned char)c;
}
