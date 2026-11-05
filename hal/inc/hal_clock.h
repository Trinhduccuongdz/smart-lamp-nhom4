/**
 * @file    hal_clock.h
 * @brief   [HAL] Xung nhịp hệ thống.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * SystemInit() trong system_stm32f10x.c (SPL) chạy trước main() và đã cấu hình:
 * thạch anh 8 MHz (HSE) -> PLL x9 -> SYSCLK = 72 MHz
 * AHB = 72 MHz, APB2 (PCLK2) = 72 MHz, APB1 (PCLK1) = 36 MHz
 * Timer trên APB1 (TIM2..4) nhận 72 MHz vì bộ chia APB1 khác 1 (slide Timer).
 */
#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include <stdint.h>

#define CLOCK_SYSCLK_HZ   72000000UL
#define CLOCK_PCLK1_HZ    36000000UL
#define CLOCK_PCLK2_HZ    72000000UL
#define CLOCK_TIMCLK1_HZ  72000000UL   /* clock cấp cho TIM2..TIM4 */

/**
 * Kiểm tra SYSCLK đã đạt 72 MHz. Trả về 0 nếu đúng; -1 nếu không (thạch anh HSE hỏng,
 * chip đang chạy HSI 8 MHz -> mọi thời gian của FreeRTOS sẽ sai 9 lần).
 */
int clock_init_72mhz(void);

#endif /* HAL_CLOCK_H */
