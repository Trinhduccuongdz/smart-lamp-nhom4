/**
 * @file    max7219.h
 * @brief   [BSP] Thư viện ma trận LED 8x8 dùng IC MAX7219 (qua SPI1).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Hỗ trợ nối tiếp nhiều module (loại 4 module FC-16): đặt MAX7219_NUM_DEVICES.
 * Lệnh MAX7219 là gói 16 bit: [địa chỉ thanh ghi 8 bit][dữ liệu 8 bit] (slide SPI).
 */
#ifndef MAX7219_H
#define MAX7219_H

#include <stdint.h>
#include <stdbool.h>

#ifndef MAX7219_NUM_DEVICES
#define MAX7219_NUM_DEVICES 1U
#endif

/* Hướng lắp module: số lần xoay khung hình 90° theo chiều kim đồng hồ trước khi gửi.
 *  0: chân IN bên trái, OUT bên phải (nằm ngang)
 *  1: chân IN ở dưới, OUT ở trên (module dựng đứng trên breadboard)
 *  2, 3: xoay thêm 90° nếu hình bị ngược / lệch */
#ifndef MAX7219_ROTATE
#define MAX7219_ROTATE 3U
#endif

void max7219_init(void);
/** Độ sáng 0..15 (thanh ghi Intensity 0x0A). */
void max7219_set_intensity(uint8_t level);
/** true = tắt màn (Shutdown mode, dòng tiêu thụ rất nhỏ). */
void max7219_set_shutdown(bool shutdown);
/**
 * Ghi toàn bộ khung hình. rows[d * 8 + r] là hàng r của module d, bit 7 = cột trái.
 * Mảng phải có 8 * MAX7219_NUM_DEVICES phần tử.
 */
void max7219_write_frame(const uint8_t *rows);

#endif /* MAX7219_H */
