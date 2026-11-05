/**
 * @file    board.c
 * @brief   [BSP] Khởi tạo bo mạch Blue Pill.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 */
#include "board.h"
#include "hal_clock.h"
#include "hal_gpio.h"
#include "hal_nvic.h"

void board_init(void)
{
    (void)clock_init_72mhz();   /* SystemInit() của SPL đã chạy 72 MHz trước main(); ở đây chỉ kiểm tra */
    gpio_clock_enable_all();

    nvic_init_priority_group();   /* 4 bit ưu tiên preemption, không dùng sub-priority */

    gpio_config(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_MODE_OUT_PP);
    board_led_set(0);
}

void board_led_toggle(void)
{
    gpio_toggle(BOARD_LED_PORT, BOARD_LED_PIN);
}

void board_led_set(int on)
{
    /* LED PC13 trên Blue Pill nối lên VCC: mức 0 = sáng */
    gpio_write(BOARD_LED_PORT, BOARD_LED_PIN, on == 0);
}
