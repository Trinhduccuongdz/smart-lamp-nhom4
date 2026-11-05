/**
 * @file    console.h
 * @brief   [MIDDLEWARES] Console nối tiếp qua USART1: gom ký tự thành dòng lệnh, phát chuỗi bằng DMA.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Nhận: ISR gom ký tự vào bộ đệm RAM; khi gặp '\r' hoặc '\n' thì báo "đủ một dòng"
 *       qua callback (theo gợi ý slide: "Ghi các ký tự nhận được vào buffer RAM, sau đó
 *       dùng semaphore/notification đánh thức task xử lý" thay vì đẩy từng ký tự vào queue).
 * Phát: console_write() khởi động DMA rồi chờ binary semaphore do ISR DMA trao.
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CONSOLE_LINE_MAX  48U

/** Callback chạy trong ISR khi có một dòng hoàn chỉnh. Chỉ được gọi API ...FromISR. */
typedef void (*console_line_cb_t)(void);

/** Gọi trước vTaskStartScheduler() (tạo semaphore, khởi tạo UART). */
void console_init(uint32_t baud, console_line_cb_t on_line);

/** Lấy một dòng đã nhận (không chờ). Trả về false nếu không còn dòng nào. */
bool console_readline(char *out, size_t size);

/** Phát chuỗi qua DMA và chờ phát xong. Chỉ MỘT task (gatekeeper LogTask) được gọi. */
void console_write(const char *s, size_t len);

/** Phát kiểu polling, dùng trong hook báo lỗi khi scheduler có thể đã hỏng. */
void console_panic_write(const char *s);

#endif /* CONSOLE_H */
