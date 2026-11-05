/**
 * @file    board.h
 * @brief   [BSP] Bản đồ chân của bo mạch (MỘT chỗ duy nhất định nghĩa chân)
 *          và LED báo hiệu PC13.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống) - cả nhóm cùng thống nhất bảng chân
 *
 *  Chức năng            Chân        Ngoại vi
 *  ------------------   ---------   -------------------------------
 *  Cảm biến quang (DO)  PA0         GPIO input pull-up + EXTI0
 *  Nút NEXT             PB12        GPIO input pull-up + EXTI12
 *  Nút OK               PB13        GPIO input pull-up + EXTI13
 *  Nút UP               PB14        GPIO input pull-up + EXTI14
 *  Nút DOWN             PB15        GPIO input pull-up + EXTI15
 *  Thạch anh RTC (LSE)  PC14/PC15   32.768 kHz (không có thì tự dùng LSI ~40 kHz)
 *  MAX7219 CS           PA4         GPIO output
 *  MAX7219 CLK          PA5         SPI1 SCK
 *  MAX7219 DIN          PA7         SPI1 MOSI
 *  OLED SCL             PB6         I2C1 SCL
 *  OLED SDA             PB7         I2C1 SDA
 *  USB-TTL RX           PA9         USART1 TX (DMA1 kênh 4)
 *  USB-TTL TX           PA10        USART1 RX
 *  LED trên bo          PC13        GPIO output (sáng khi mức 0)
 */
#ifndef BOARD_H
#define BOARD_H

#include "stm32f10x.h"

#define BOARD_LIGHT_PORT      GPIOA
#define BOARD_LIGHT_PIN       0U

#define BOARD_BTN_NEXT_PORT   GPIOB
#define BOARD_BTN_NEXT_PIN    12U
#define BOARD_BTN_OK_PORT     GPIOB
#define BOARD_BTN_OK_PIN      13U
#define BOARD_BTN_UP_PORT     GPIOB
#define BOARD_BTN_UP_PIN      14U
#define BOARD_BTN_DOWN_PORT   GPIOB
#define BOARD_BTN_DOWN_PIN    15U

#define BOARD_MAX7219_CS_PORT GPIOA
#define BOARD_MAX7219_CS_PIN  4U

#define BOARD_LED_PORT        GPIOC
#define BOARD_LED_PIN         13U

/* Ưu tiên ngắt NVIC (4 bit, 0 = cao nhất). FreeRTOS: configMAX_SYSCALL_INTERRUPT_PRIORITY = 5.
 * ISR có gọi API ...FromISR phải có số ưu tiên >= 5. */
#define BOARD_IRQ_PRIO_TIM2   4U   /* chỉ đếm, KHÔNG gọi API -> được phép cao hơn ngưỡng */
#define BOARD_IRQ_PRIO_UART   6U
#define BOARD_IRQ_PRIO_RTC    6U   /* ngắt mỗi giây, gọi xQueueSendFromISR */
#define BOARD_IRQ_PRIO_EXTI   7U

void board_init(void);        /* clock 72 MHz, clock GPIO, nhóm ưu tiên NVIC, LED */
void board_led_toggle(void);
void board_led_set(int on);

#endif /* BOARD_H */
