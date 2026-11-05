/**
 * @file    xprintf.h
 * @brief   [MIDDLEWARES] snprintf thu gọn, an toàn khi dùng trong nhiều task.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * Lý do không dùng printf/sprintf của thư viện C (slide FreeRTOS "Rủi ro khi dùng printf()"):
 * tốn nhiều Flash, cần stack lớn, có thể gọi malloc() và không thread-safe.
 * Hàm này không dùng biến toàn cục, không cấp phát động -> tái nhập (reentrant).
 *
 * Hỗ trợ: %d %i %u %x %X %c %s %%, độ rộng (ví dụ %5d), đệm số 0 (%03u), căn trái (%-8s).
 */
#ifndef XPRINTF_H
#define XPRINTF_H

#include <stdarg.h>
#include <stddef.h>

/* Cho trình biên dịch kiểm tra chuỗi định dạng như printf. Chỉ GCC và ARM Compiler 6 (clang)
 * hỗ trợ; ARM Compiler 5 (armcc, định nghĩa __CC_ARM) thì bỏ qua. */
#if defined(__clang__) || (defined(__GNUC__) && !defined(__CC_ARM))
#define XPRINTF_FORMAT(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define XPRINTF_FORMAT(fmt_idx, arg_idx)
#endif

int xvsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int xsnprintf(char *buf, size_t size, const char *fmt, ...) XPRINTF_FORMAT(3, 4);

#endif /* XPRINTF_H */
