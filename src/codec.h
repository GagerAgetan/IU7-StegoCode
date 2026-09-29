#ifndef CODEC_H
#define CODEC_H

#include <stddef.h>

/* Параметры, вычисленные из ключа. */
typedef struct {
    long draft;
    int  shift;      /* switch */
    int  x0, y0;     /* стартовый пиксель (уже по модулю размеров) */
} KeyParams;

/* Вычисляет draft/switch/x0/y0. Возвращает 0 при успехе, -1 при ошибке. */
int codec_key_params(const char *key, int width, int height, KeyParams *out);

/* Кодирует символ c с учётом switch. carry = 1, если был перенос. */
unsigned char codec_encode_char(unsigned char c, int shift, int *carry);

/* Декодирует символ. carry — прочитанный флаг (для проверки/отладки). */
unsigned char codec_decode_char(unsigned char e, int shift, int carry);

#endif
