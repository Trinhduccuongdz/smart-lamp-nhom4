/**
 * @file    hal_i2c.h
 * @brief   [HAL] I2C1 master, chế độ Fast 400 kHz, chỉ ghi (đủ cho OLED SSD1306). Dùng SPL.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Chân: PB6 = SCL, PB7 = SDA (alternate function open-drain).
 * Mọi vòng chờ đều có timeout để task không bị treo nếu bus lỗi.
 */
#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>

typedef enum {
    I2C_OK = 0,
    I2C_ERR_BUSY,      /* bus bận quá lâu */
    I2C_ERR_START,     /* không tạo được điều kiện START */
    I2C_ERR_NACK,      /* slave không trả ACK (AF) */
    I2C_ERR_TIMEOUT
} i2c_status_t;

void i2c1_init(void);

/**
 * Ghi một gói: START, địa chỉ (7 bit) + W, `prefix` (thường là byte điều khiển),
 * rồi `len` byte `data`, cuối cùng STOP.
 */
i2c_status_t i2c1_write(uint8_t addr7, uint8_t prefix, const uint8_t *data, uint16_t len);

#endif /* HAL_I2C_H */
