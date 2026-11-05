/**
 * @file    hal_exti.h
 * @brief   [HAL] Ngắt ngoài EXTI: chọn chân, chọn cạnh, đăng ký hàm callback (dùng SPL).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Driver KHÔNG phụ thuộc FreeRTOS. Tầng trên đăng ký callback; callback chạy
 * trong ngữ cảnh ISR nên chỉ được gọi API FreeRTOS có hậu tố FromISR.
 */
#ifndef HAL_EXTI_H
#define HAL_EXTI_H

#include <stdint.h>
#include "stm32f10x.h"

typedef enum {
    EXTI_EDGE_RISING  = 1,
    EXTI_EDGE_FALLING = 2,
    EXTI_EDGE_BOTH    = 3
} exti_edge_t;

/** Callback gọi từ ISR. Tham số: số line (0..15). */
typedef void (*exti_callback_t)(uint8_t line);

/**
 * Cấu hình EXTI cho chân `pin` của `port`, bật ngắt trong NVIC.
 * @param irq_prio  mức ưu tiên preemption (0..15, số càng lớn ưu tiên càng thấp).
 *                  Nếu callback gọi API FreeRTOS thì phải >= configMAX_SYSCALL (5).
 */
void exti_config(GPIO_TypeDef *port, uint8_t pin, exti_edge_t edge,
                 uint8_t irq_prio, exti_callback_t cb);

#endif /* HAL_EXTI_H */
