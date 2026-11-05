/**
 * @file    hal_rtc.h
 * @brief   [HAL] RTC trong chip STM32F103: bộ đếm giây 32 bit, ngắt mỗi giây (dùng SPL).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Nguồn xung: ưu tiên thạch anh LSE 32.768 kHz (PC14/PC15). Nếu LSE không khởi động
 * được (bo không gắn thạch anh) thì tự chuyển sang LSI ~40 kHz (sai số vài %).
 * Thanh ghi Backup DR1 giữ dấu hiệu "đã cài" để reset mềm không làm mất giờ.
 * Driver KHÔNG phụ thuộc FreeRTOS: callback chạy trong ISR.
 */
#ifndef HAL_RTC_H
#define HAL_RTC_H

#include <stdint.h>

typedef void (*rtc_second_cb_t)(void);

typedef enum { RTC_SRC_LSE = 0, RTC_SRC_LSI = 1 } rtc_source_t;

/**
 * Khởi động RTC, bật ngắt giây với mức ưu tiên irq_prio (>= 5 nếu callback gọi API FreeRTOS).
 * @return nguồn xung đang dùng.
 */
rtc_source_t rtc_hw_init(uint8_t irq_prio, rtc_second_cb_t cb);

uint32_t rtc_hw_get_counter(void);
void     rtc_hw_set_counter(uint32_t value);

#endif /* HAL_RTC_H */
