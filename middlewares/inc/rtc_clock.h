/**
 * @file    rtc_clock.h
 * @brief   [MIDDLEWARE] Đồng hồ giờ trong ngày dựa trên RTC trong chip (hal_rtc).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Bộ đếm RTC đếm giây; giờ trong ngày = bộ đếm mod 86400. Ngắt giây gọi callback (trong ISR).
 */
#ifndef RTC_CLOCK_H
#define RTC_CLOCK_H

#include <stdint.h>
#include "hal_rtc.h"

#define RTC_SECONDS_PER_DAY  86400UL

/** Khởi động RTC. @return chuỗi tên nguồn xung ("LSE" hoặc "LSI"). */
const char *rtc_clock_init(uint8_t irq_prio, rtc_second_cb_t on_second);

/** Số giây kể từ 00:00:00 (0..86399). */
uint32_t rtc_clock_now(void);

/** Đặt giờ: sec_of_day 0..86399. */
void rtc_clock_set(uint32_t sec_of_day);

#endif /* RTC_CLOCK_H */
