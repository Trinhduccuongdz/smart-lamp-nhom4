/**
 * @file    xprintf.c
 * @brief   [MIDDLEWARES] Cài đặt snprintf thu gọn.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 */
#include "xprintf.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    char  *buf;
    size_t size;
    size_t pos;
} out_t;

static void put(out_t *o, char c)
{
    if (o->pos + 1U < o->size) {
        o->buf[o->pos] = c;
    }
    o->pos++;
}

static void put_padded(out_t *o, const char *s, size_t len, int width, bool left, char pad)
{
    int fill = width - (int)len;
    if (!left) { while (fill-- > 0) { put(o, pad); } }
    for (size_t i = 0; i < len; i++) { put(o, s[i]); }
    if (left)  { while (fill-- > 0) { put(o, ' '); } }
}

int xvsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    out_t o = { buf, size, 0 };
    char tmp[12];

    while (*fmt != '\0') {
        if (*fmt != '%') { put(&o, *fmt++); continue; }
        fmt++;

        bool left = false;
        char pad = ' ';
        int width = 0;
        if (*fmt == '-') { left = true; fmt++; }
        if (*fmt == '0') { pad = '0'; fmt++; }
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt++ - '0'); }
        if (*fmt == 'l') { fmt++; }   /* long = int trên Cortex-M3 (32 bit) */

        char spec = *fmt;
        if (spec == '\0') { break; }
        fmt++;

        switch (spec) {
        case 'd': case 'i': case 'u': case 'x': case 'X': {
            uint32_t v;
            bool neg = false;
            if (spec == 'd' || spec == 'i') {
                int32_t sv = va_arg(ap, int32_t);
                neg = sv < 0;
                v = neg ? (uint32_t)(-(sv + 1)) + 1U : (uint32_t)sv;
            } else {
                v = va_arg(ap, uint32_t);
            }
            uint32_t base = (spec == 'x' || spec == 'X') ? 16U : 10U;
            const char *digits = (spec == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            size_t n = 0;
            do { tmp[n++] = digits[v % base]; v /= base; } while (v != 0U && n < sizeof(tmp));
            if (neg) {
                if (pad == '0') { put(&o, '-'); width--; } else { tmp[n++] = '-'; }
            }
            char rev[12];
            for (size_t i = 0; i < n; i++) { rev[i] = tmp[n - 1U - i]; }
            put_padded(&o, rev, n, width, left, pad);
            break;
        }
        case 'c':
            tmp[0] = (char)va_arg(ap, int);
            put_padded(&o, tmp, 1, width, left, ' ');
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (s == NULL) { s = "(null)"; }
            size_t n = 0;
            while (s[n] != '\0') { n++; }
            put_padded(&o, s, n, width, left, ' ');
            break;
        }
        case '%':
            put(&o, '%');
            break;
        default:
            put(&o, '%');
            put(&o, spec);
            break;
        }
    }
    if (size > 0U) {
        buf[(o.pos < size) ? o.pos : size - 1U] = '\0';
    }
    return (int)o.pos;
}

int xsnprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = xvsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}
