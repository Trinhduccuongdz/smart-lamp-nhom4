/**
 * @file    hal_spi.h
 * @brief   [HAL] SPI1 master, chỉ truyền (MOSI + SCK), khung 16 bit (dùng SPL).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Chân: PA5 = SCK, PA7 = MOSI (alternate function push-pull). Chân CS do tầng
 * trên tự điều khiển bằng GPIO (slide SPI: "Consider a GPIO pin for the CE pin").
 */
#ifndef HAL_SPI_H
#define HAL_SPI_H

#include <stdint.h>

/** Khởi tạo SPI1: master, CPOL=0, CPHA=0, MSB trước, 16 bit, PCLK2/8 = 9 MHz. */
void spi1_init(void);

/** Gửi một khung 16 bit, chờ đến khi truyền xong. */
void spi1_write16(uint16_t data);

/** Chờ SPI rảnh (dùng trước khi kéo CS lên). */
void spi1_wait_idle(void);

#endif /* HAL_SPI_H */
