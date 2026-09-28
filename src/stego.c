#include "stego.h"
#include "codec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Позиция в битовом потоке --- */

typedef struct {
    Image *img;
    int x, y;        /* текущий пиксель */
    int channel;     /* 0=R, 1=G, 2=B */
} BitWriter;

typedef struct {
    const Image *img;
    int x, y;
    int channel;
} BitReader;

static size_t total_bytes(const Image *img) {
    return (size_t)img->width * (size_t)img->height * 3u;
}

static size_t pixel_index(const Image *img, int x, int y) {
    return ((size_t)y * (size_t)img->width + (size_t)x) * 3u;
}

/* Записывает один бит в текущую позицию и сдвигает позицию на следующий байт. */
static int bw_write_bit(BitWriter *bw, int bit) {
    if (bw->y >= bw->img->height) return -1;
    size_t base = pixel_index(bw->img, bw->x, bw->y);
    unsigned char *p = &bw->img->data[base + bw->channel];
    *p = (unsigned char)((*p & 0xFE) | (bit & 1));

    /* сдвиг */
    bw->channel++;
    if (bw->channel == 3) {
        bw->channel = 0;
        bw->x++;
        if (bw->x >= bw->img->width) {
            bw->x = 0;
            bw->y++;
        }
    }
    return 0;
}

static int br_read_bit(BitReader *br, int *bit) {
    if (br->y >= br->img->height) return -1;
    size_t base = pixel_index(br->img, br->x, br->y);
    unsigned char v = br->img->data[base + br->channel];
    *bit = v & 1;

    br->channel++;
    if (br->channel == 3) {
        br->channel = 0;
        br->x++;
        if (br->x >= br->img->width) {
            br->x = 0;
            br->y++;
        }
    }
    return 0;
}

/* Позиционирует writer/reader на заданный пиксель (channel = 0). */
static void bw_goto(BitWriter *bw, int x, int y) {
    bw->x = x; bw->y = y; bw->channel = 0;
}
static void br_goto(BitReader *br, int x, int y) {
    br->x = x; br->y = y; br->channel = 0;
}

/* Записывает n бит значения value (старшие биты идут первыми? — нам всё равно,
   главное чтобы чтение шло в том же порядке). Пишем от младшего к старшему. */
static int bw_write_bits(BitWriter *bw, unsigned value, int n) {
    for (int i = 0; i < n; ++i) {
        if (bw_write_bit(bw, (value >> i) & 1) < 0) return -1;
    }
    return 0;
}

static int br_read_bits(BitReader *br, int n, unsigned *out) {
    unsigned v = 0;
    for (int i = 0; i < n; ++i) {
        int bit;
        if (br_read_bit(br, &bit) < 0) return -1;
        v |= (unsigned)bit << i;
    }
    *out = v;
    return 0;
}

/* Проверка: хватит ли байтов на весь поток. Считаем worst-case:
   на каждый символ 14 бит + терминатор 9 бит + запас. */
static int check_capacity(const Image *img, size_t msg_len) {
    size_t total_bits = (msg_len + 1) * 14u + 16u;
    size_t total = total_bytes(img) * 8u;
    return total >= total_bits;
}

int stego_encode(Image *img, const char *msg, const char *key) {
    KeyParams kp;
    if (codec_key_params(key, img->width, img->height, &kp) < 0) return -1;

    size_t msg_len = strlen(msg);
    if (!check_capacity(img, msg_len)) {
        fprintf(stderr, "Ошибка: в изображении недостаточно места для строки\n");
        return -1;
    }

    BitWriter bw;
    bw.img = img;
    bw.channel = 0;
    bw_goto(&bw, kp.x0, kp.y0);

    /* Кодируем строку + терминатор. */
    size_t n = msg_len + 1; /* +1 — терминатор */
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = (i < msg_len) ? (unsigned char)msg[i] : 0;

        int carry = 0;
        unsigned char e = codec_encode_char(c, kp.shift, &carry);
        if (bw_write_bits(&bw, e, 8) < 0) goto no_space;
        if (bw_write_bits(&bw, (unsigned)carry, 1) < 0) goto no_space;

        if (c == 0) break; /* терминатор — без dx/dy */

        /* dx ∈ [15,25], храним как есть (5 бит) */
        int dx = 15 + (rand() % 11);   /* 15..25 */
        int dy = 1  + (rand() % 2);    /* 1..2  */
        if (bw_write_bits(&bw, (unsigned)dx, 5) < 0) goto no_space;
        if (bw_write_bits(&bw, (unsigned)(dy - 1), 1) < 0) goto no_space;

        /* Переход от пикселя, с которого начался символ. */
        int nx = (kp.x0 + dx) % img->width;
        int ny = (kp.y0 + dy) % img->height;
        kp.x0 = nx;
        kp.y0 = ny;
        bw_goto(&bw, nx, ny);
    }
    return 0;

no_space:
    fprintf(stderr, "Ошибка: закончилось место в изображении\n");
    return -1;
}

char *stego_decode(const Image *img, const char *key) {
    KeyParams kp;
    if (codec_key_params(key, img->width, img->height, &kp) < 0) return NULL;

    BitReader br;
    br.img = img;
    br.channel = 0;
    br_goto(&br, kp.x0, kp.y0);

    size_t cap = 64;
    size_t len = 0;
    char *out = malloc(cap);
    if (!out) { fprintf(stderr, "Ошибка: malloc\n"); return NULL; }

    for (;;) {
        unsigned e_u;
        unsigned carry_u;
        if (br_read_bits(&br, 8, &e_u) < 0) goto no_data;
        if (br_read_bits(&br, 1, &carry_u) < 0) goto no_data;

        unsigned char c = codec_decode_char((unsigned char)e_u, kp.shift,
                                            (int)carry_u);
        if (c == 0) break; /* терминатор */

        if (len + 1 >= cap) {
            cap *= 2;
            char *tmp = realloc(out, cap);
            if (!tmp) { free(out); fprintf(stderr, "Ошибка: realloc\n"); return NULL; }
            out = tmp;
        }
        out[len++] = (char)c;

        unsigned dx_u, dy_u;
        if (br_read_bits(&br, 5, &dx_u) < 0) goto no_data;
        if (br_read_bits(&br, 1, &dy_u) < 0) goto no_data;

        int dx = (int)dx_u;
        int dy = (int)dy_u + 1;
        if (dx < 15 || dx > 25 || dy < 1 || dy > 2) {
            fprintf(stderr, "Ошибка: некорректный отступ (dx=%d, dy=%d)\n", dx, dy);
            free(out);
            return NULL;
        }

        int nx = (kp.x0 + dx) % img->width;
        int ny = (kp.y0 + dy) % img->height;
        kp.x0 = nx;
        kp.y0 = ny;
        br_goto(&br, nx, ny);
    }

    out[len] = '\0';
    return out;

no_data:
    fprintf(stderr, "Ошибка: данные закончились раньше терминатора\n");
    free(out);
    return NULL;
}
