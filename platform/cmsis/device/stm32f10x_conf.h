/**
 * @file    stm32f10x_conf.h
 * @brief   Chọn các module của thư viện Standard Peripheral (SPL V3.5.0) mà project dùng.
 *          stm32f10x.h tự include file này khi có define USE_STDPERIPH_DRIVER.
 */
#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

#include "stm32f10x_dma.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rtc.h"     /* RTC trong chip (đồng hồ thời gian thực) */
#include "stm32f10x_bkp.h"     /* thanh ghi Backup: đánh dấu RTC đã cài */
#include "stm32f10x_pwr.h"     /* mở quyền ghi miền Backup */
#include "stm32f10x_flash.h"   /* xóa/ghi Flash: lưu cài đặt */
#include "misc.h"   /* NVIC_Init, NVIC_PriorityGroupConfig */

/* Bỏ comment dòng dưới để SPL kiểm tra tham số đầu vào (tốn thêm Flash, dùng khi gỡ lỗi) */
/* #define USE_FULL_ASSERT    1 */

#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif

#endif /* __STM32F10x_CONF_H */
