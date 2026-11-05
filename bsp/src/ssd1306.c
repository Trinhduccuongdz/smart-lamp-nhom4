/**
 * @file    ssd1306.c
 * @brief   [BSP] OLED SSD1306 dựa trên driver i2c.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "ssd1306.h"
#include "font5x7.h"
#include "hal_i2c.h"
#include <string.h>

#define CTRL_CMD   0x00U   /* byte điều khiển: các byte sau là lệnh */
#define CTRL_DATA  0x40U   /* byte điều khiển: các byte sau là dữ liệu GDDRAM */

static uint8_t  s_fb[SSD1306_WIDTH * SSD1306_HEIGHT / 8U];   /* 1024 byte, mỗi byte = 8 pixel dọc */
static uint32_t s_last_sum = 0xFFFFFFFFUL;
static bool     s_ok;
static uint8_t  s_addr = SSD1306_I2C_ADDR;
static i2c_status_t s_err = I2C_OK;

static bool cmd_list(const uint8_t *cmds, uint16_t n)
{
    /* Gửi mỗi lệnh một gói [0x00][cmd] cho đơn giản và chắc chắn */
    for (uint16_t i = 0; i < n; i++) {
        s_err = i2c1_write(s_addr, CTRL_CMD, &cmds[i], 1);
        if (s_err != I2C_OK) {
            return false;
        }
    }
    return true;
}

bool ssd1306_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,             /* tắt màn */
        0xD5, 0x80,       /* tần số dao động / bộ chia clock */
        0xA8, 0x3F,       /* multiplex = 64 dòng */
        0xD3, 0x00,       /* offset = 0 */
        0x40,             /* dòng bắt đầu = 0 */
        0x8D, 0x14,       /* bật bơm điện áp nội (charge pump) */
        0x20, 0x00,       /* chế độ địa chỉ ngang */
        0xA1,             /* lật cột (segment remap) */
        0xC8,             /* quét COM từ dưới lên */
        0xDA, 0x12,       /* cấu hình chân COM cho 128x64 */
        0x81, 0xCF,       /* độ tương phản */
        0xD9, 0xF1,       /* pre-charge */
        0xDB, 0x40,       /* VCOMH */
        0xA4,             /* hiển thị theo RAM */
        0xA6,             /* không đảo màu */
        0xAF              /* bật màn */
    };
    static const uint8_t probe[] = { 0xAE };   /* lệnh "tắt màn": dùng để dò địa chỉ */
    i2c1_init();

    /* Dò địa chỉ: thử 0x3C trước, không ACK thì thử 0x3D */
    s_addr = SSD1306_I2C_ADDR;
    if (!cmd_list(probe, sizeof(probe))) {
        if (s_err != I2C_ERR_NACK) {          /* lỗi bus, không phải sai địa chỉ: báo luôn */
            s_ok = false;
            return false;
        }
        s_addr = SSD1306_I2C_ADDR_ALT;        /* 0x3C không trả lời: thử 0x3D */
        if (!cmd_list(probe, sizeof(probe))) {
            s_addr = SSD1306_I2C_ADDR;
            s_ok = false;
            return false;
        }
    }
    s_ok = cmd_list(init_seq, sizeof(init_seq));
    ssd1306_clear();
    s_last_sum = 0xFFFFFFFFUL;
    return s_ok;
}

void ssd1306_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

void ssd1306_set_pixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) { return; }
    uint16_t idx = (uint16_t)x + (uint16_t)(y / 8U) * SSD1306_WIDTH;
    uint8_t  bit = (uint8_t)(1U << (y & 7U));
    if (on) { s_fb[idx] |= bit; } else { s_fb[idx] &= (uint8_t)~bit; }
}

void ssd1306_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool on)
{
    for (uint8_t i = 0; i < w; i++) {
        for (uint8_t j = 0; j < h; j++) {
            ssd1306_set_pixel((uint8_t)(x + i), (uint8_t)(y + j), on);
        }
    }
}

void ssd1306_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    if (w == 0U || h == 0U) { return; }
    for (uint8_t i = 0; i < w; i++) {
        ssd1306_set_pixel((uint8_t)(x + i), y, true);
        ssd1306_set_pixel((uint8_t)(x + i), (uint8_t)(y + h - 1U), true);
    }
    for (uint8_t j = 0; j < h; j++) {
        ssd1306_set_pixel(x, (uint8_t)(y + j), true);
        ssd1306_set_pixel((uint8_t)(x + w - 1U), (uint8_t)(y + j), true);
    }
}

void ssd1306_draw_bitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bits)
{
    uint8_t stride = (uint8_t)((w + 7U) / 8U);
    for (uint8_t j = 0; j < h; j++) {
        for (uint8_t i = 0; i < w; i++) {
            if ((bits[j * stride + i / 8U] & (0x80U >> (i & 7U))) != 0U) {
                ssd1306_set_pixel((uint8_t)(x + i), (uint8_t)(y + j), true);
            }
        }
    }
}

void ssd1306_draw_string(uint8_t x, uint8_t line, const char *s, bool invert)
{
    if (line >= SSD1306_HEIGHT / 8U) { return; }
    uint16_t idx = (uint16_t)line * SSD1306_WIDTH + x;
    while (*s != '\0' && idx + 6U <= (uint16_t)(line + 1U) * SSD1306_WIDTH) {
        uint8_t c = (uint8_t)*s++;
        if (c < FONT5X7_FIRST || c >= FONT5X7_FIRST + FONT5X7_COUNT) { c = '?'; }
        const uint8_t *g = g_font5x7[c - FONT5X7_FIRST];
        for (uint8_t col = 0; col < 6U; col++) {
            uint8_t bits = (col < 5U) ? g[col] : 0x00U;   /* cột thứ 6 là khoảng cách */
            s_fb[idx++] = invert ? (uint8_t)~bits : bits;
        }
    }
}

bool ssd1306_update(void)
{
    /* Tổng kiểm tra đơn giản: nếu khung không đổi thì khỏi gửi (tiết kiệm ~23 ms bus I2C) */
    uint32_t sum = 0;
    for (uint16_t i = 0; i < sizeof(s_fb); i++) {
        sum = (sum << 1 | sum >> 31) ^ s_fb[i];
    }
    if (sum == s_last_sum) {
        return false;
    }

    static const uint8_t window[] = { 0x21, 0x00, 0x7F,    /* cột 0..127 */
                                      0x22, 0x00, 0x07 };  /* trang 0..7 */
    if (!cmd_list(window, sizeof(window))) {
        s_ok = false;
        return false;
    }
    /* Gửi 1024 byte thành từng gói 128 byte (mỗi gói bắt đầu bằng byte điều khiển 0x40) */
    for (uint16_t off = 0; off < sizeof(s_fb); off += 128U) {
        s_err = i2c1_write(s_addr, CTRL_DATA, &s_fb[off], 128U);
        if (s_err != I2C_OK) {
            s_ok = false;
            return false;
        }
    }
    s_ok = true;
    s_last_sum = sum;
    return true;
}

uint8_t ssd1306_address(void)
{
    return s_addr;
}

const char *ssd1306_error_text(void)
{
    switch (s_err) {
    case I2C_OK:          return "OK";
    case I2C_ERR_BUSY:    return "BUSY";
    case I2C_ERR_START:   return "START";
    case I2C_ERR_NACK:    return "NACK";
    case I2C_ERR_TIMEOUT: return "TIMEOUT";
    default:              return "?";
    }
}
