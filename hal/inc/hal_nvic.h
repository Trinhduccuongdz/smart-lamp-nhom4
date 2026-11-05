/**
 * @file    hal_nvic.h
 * @brief   [HAL] Cấu hình chung của bộ điều khiển ngắt NVIC (dùng SPL, misc.c).
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 */
#ifndef HAL_NVIC_H
#define HAL_NVIC_H

/**
 * Dành toàn bộ 4 bit ưu tiên cho preemption, không dùng sub-priority
 * (khuyến nghị của FreeRTOS cho Cortex-M, slide "Pre-emption priority và sub-priority").
 * Phải gọi trước khi cấu hình bất kỳ ngắt nào.
 */
void nvic_init_priority_group(void);

#endif /* HAL_NVIC_H */
