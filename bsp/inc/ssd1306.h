/**
 * @file    ssd1306.h
 * @brief   [BSP] Thư viện OLED SSD1306 128x64 qua I2C1 (địa chỉ 0x3C).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Vẽ vào bộ đệm khung hình 1 KB trong RAM, sau đó ssd1306_update() đẩy cả khung ra OLED.
 * Chữ dùng font 5x7: 21 ký tự x 8 dòng.
 */
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>

#define SSD1306_WIDTH    128U
#define SSD1306_HEIGHT   64U
/* Địa chỉ 7 bit: đa số module là 0x3C (mặt sau ghi 0x78), một số là 0x3D (ghi 0x7A).
 * ssd1306_init() tự thử cả hai và dùng địa chỉ nào trả lời. */
#define SSD1306_I2C_ADDR     0x3CU
#define SSD1306_I2C_ADDR_ALT 0x3DU

bool ssd1306_init(void);                 /* false nếu OLED không phản hồi ở cả hai địa chỉ */
/** Địa chỉ 7 bit đang dùng (sau ssd1306_init). */
uint8_t ssd1306_address(void);
/** Mô tả lỗi I2C lần gần nhất để in log chẩn đoán: "OK", "BUSY", "START", "NACK", "TIMEOUT". */
const char *ssd1306_error_text(void);
void ssd1306_clear(void);
void ssd1306_set_pixel(uint8_t x, uint8_t y, bool on);
void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on);
void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
/** Vẽ chuỗi tại cột pixel x, dòng chữ `line` (0..7). invert = chữ trắng nền đen ngược lại. */
void ssd1306_draw_string(uint8_t x, uint8_t line, const char *s, bool invert);
/** Vẽ ảnh 1 bit w x h tại (x, y). Dữ liệu theo hàng, MSB trước, mỗi hàng (w + 7) / 8 byte. */
void ssd1306_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bits);
/** Gửi bộ đệm ra màn hình. Bỏ qua nếu nội dung không đổi so với lần trước. Trả về true nếu có gửi. */
bool ssd1306_update(void);

#endif /* SSD1306_H */
