/**
 * @file    max7219.c
 * @brief   [BSP] Điều khiển MAX7219 dựa trên driver spi + gpio.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "max7219.h"
#include "board.h"
#include "hal_spi.h"
#include "hal_gpio.h"

/* Địa chỉ thanh ghi MAX7219 (bảng lệnh trong slide SPI) */
#define REG_DIGIT0       0x01U
#define REG_DECODE_MODE  0x09U
#define REG_INTENSITY    0x0AU
#define REG_SCAN_LIMIT   0x0BU
#define REG_SHUTDOWN     0x0CU
#define REG_DISPLAY_TEST 0x0FU


static inline void cs_low(void)  { gpio_reset(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN); }
static inline void cs_high(void) { spi1_wait_idle(); gpio_set(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN); }

/* Gửi cùng một lệnh tới tất cả module trong chuỗi */
static void send_all(uint8_t reg, uint8_t data)
{
    cs_low();
    for (uint8_t d = 0; d < MAX7219_NUM_DEVICES; d++) {
        spi1_write16((uint16_t)((uint16_t)reg << 8) | data);
    }
    cs_high();                         /* cạnh lên của CS chốt dữ liệu */
}

void max7219_init(void)
{
    gpio_config(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN, GPIO_MODE_OUT_PP);
    gpio_set(BOARD_MAX7219_CS_PORT, BOARD_MAX7219_CS_PIN);
    spi1_init();

    send_all(REG_DISPLAY_TEST, 0x00);  /* tắt chế độ test */
    send_all(REG_DECODE_MODE, 0x00);   /* không giải mã BCD: điều khiển từng LED */
    send_all(REG_SCAN_LIMIT, 0x07);    /* quét đủ 8 hàng */
    send_all(REG_INTENSITY, 0x00);
    for (uint8_t r = 0; r < 8U; r++) {
        send_all((uint8_t)(REG_DIGIT0 + r), 0x00);
    }
    send_all(REG_SHUTDOWN, 0x00);      /* bắt đầu ở trạng thái tắt */
}

void max7219_set_intensity(uint8_t level)
{
    send_all(REG_INTENSITY, (uint8_t)(level & 0x0FU));
}

void max7219_set_shutdown(bool shutdown)
{
    send_all(REG_SHUTDOWN, shutdown ? 0x00U : 0x01U);
}

/* Xoay ảnh 8x8 theo hướng lắp module (bit 7 = cột trái, rows[0] = hàng trên) */
static void rotate8(const uint8_t *in, uint8_t *out)
{
    for (uint8_t y = 0; y < 8U; y++) {
        uint8_t v = 0;
        for (uint8_t x = 0; x < 8U; x++) {
#if MAX7219_ROTATE == 1U
            uint8_t sy = (uint8_t)(7U - x), sx = y;                 /* 90° thuận chiều kim đồng hồ */
#elif MAX7219_ROTATE == 2U
            uint8_t sy = (uint8_t)(7U - y), sx = (uint8_t)(7U - x); /* 180° */
#elif MAX7219_ROTATE == 3U
            uint8_t sy = x, sx = (uint8_t)(7U - y);                 /* 90° ngược chiều kim đồng hồ */
#else
            uint8_t sy = y, sx = x;
#endif
            v = (uint8_t)(v | (((in[sy] >> (7U - sx)) & 1U) << (7U - x)));
        }
        out[y] = v;
    }
}

void max7219_write_frame(const uint8_t *rows)
{
    uint8_t buf[8U * MAX7219_NUM_DEVICES];
    for (uint8_t d = 0; d < MAX7219_NUM_DEVICES; d++) {
        rotate8(&rows[d * 8U], &buf[d * 8U]);
    }
    for (uint8_t r = 0; r < 8U; r++) {
        cs_low();
        /* Khung đầu tiên đẩy vào sẽ đi xa nhất -> gửi module cuối trước */
        for (int d = (int)MAX7219_NUM_DEVICES - 1; d >= 0; d--) {
            spi1_write16((uint16_t)((uint16_t)(REG_DIGIT0 + r) << 8) | buf[d * 8 + r]);
        }
        cs_high();
    }
}
