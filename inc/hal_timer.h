/**
 * @file    hal_timer.h
 * @brief   [HAL] TIM2 đếm tự do làm thang thời gian đo (run-time stats của FreeRTOS). Dùng SPL.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * TIM2 đếm lên 16 bit với tần số `count_hz` (PSC = 72 MHz / count_hz - 1, ARR = 0xFFFF).
 * Mỗi lần tràn (UIF) gọi callback để tầng trên mở rộng thành bộ đếm 32 bit.
 */
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include <stdint.h>

typedef void (*timer_overflow_cb_t)(void);

void     tim2_freerun_init(uint32_t count_hz, uint8_t irq_prio, timer_overflow_cb_t cb);
uint16_t tim2_get_count(void);
/** true nếu đang có cờ tràn chưa xử lý (dùng khi đọc bộ đếm 32 bit cho đúng). */
int      tim2_overflow_pending(void);

#endif /* HAL_TIMER_H */
