/**
 * @file    hal_gpio.h
 * @brief   [HAL] Cấu hình và đọc/ghi chân GPIO (dùng thư viện Standard Peripheral).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Tầng trên chỉ dùng số chân 0..15 và các chế độ dưới đây, không cần biết SPL.
 * Mỗi chế độ ứng với mã 4 bit CNF[1:0]MODE[1:0] trong thanh ghi CRL/CRH (slide "GPIO Ports"):
 *   OUT_PP  = 0x3 (output push-pull 50 MHz)      AF_PP = 0xB (alternate function push-pull)
 *   IN_FLOATING = 0x4                            AF_OD = 0xF (alternate function open-drain)
 *   IN_PULL = 0x8 (ODR = 1 kéo lên, ODR = 0 kéo xuống)    ANALOG = 0x0
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f10x.h"

typedef enum {
    GPIO_MODE_ANALOG = 0,
    GPIO_MODE_IN_FLOATING,
    GPIO_MODE_OUT_PP,
    GPIO_MODE_OUT_OD,
    GPIO_MODE_AF_PP,
    GPIO_MODE_AF_OD
} gpio_mode_t;

typedef enum { GPIO_PULL_DOWN = 0, GPIO_PULL_UP = 1 } gpio_pull_t;

/** Bật clock cho các port A, B, C và khối AFIO (cần cho EXTI). */
void gpio_clock_enable_all(void);

/** Cấu hình một chân. pin = 0..15. */
void gpio_config(GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode);

/** Cấu hình chân input có điện trở kéo lên/xuống bên trong. */
void gpio_config_input_pull(GPIO_TypeDef *port, uint8_t pin, gpio_pull_t pull);

void gpio_set(GPIO_TypeDef *port, uint8_t pin);
void gpio_reset(GPIO_TypeDef *port, uint8_t pin);
void gpio_write(GPIO_TypeDef *port, uint8_t pin, bool level);
bool gpio_read(GPIO_TypeDef *port, uint8_t pin);
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);

#endif /* HAL_GPIO_H */
