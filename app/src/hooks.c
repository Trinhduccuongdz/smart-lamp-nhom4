/**
 * @file    hooks.c
 * @brief   [APPLICATION] Các hook báo lỗi của FreeRTOS: tràn stack, hết heap, configASSERT.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * Khi lỗi: chặn ngắt, in thông báo qua UART kiểu polling (không dựa vào scheduler),
 * rồi nháy nhanh LED PC13 mãi mãi để dễ nhận biết. Khi debug, đặt breakpoint tại fatal_loop().
 */
#include "FreeRTOS.h"
#include "task.h"
#include "console.h"
#include "board.h"
#include "xprintf.h"

static void fatal_loop(void)
{
    for (;;) {
        board_led_toggle();
        for (volatile uint32_t i = 0; i < 600000UL; i++) { }
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    char buf[48];
    taskDISABLE_INTERRUPTS();
    xsnprintf(buf, sizeof(buf), "\r\n!!! TRAN STACK: task %s\r\n", name);
    console_panic_write(buf);
    fatal_loop();
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    console_panic_write("\r\n!!! HET HEAP FreeRTOS (tang configTOTAL_HEAP_SIZE)\r\n");
    fatal_loop();
}

void app_assert_failed(const char *file, int line)
{
    char buf[80];
    taskDISABLE_INTERRUPTS();
    xsnprintf(buf, sizeof(buf), "\r\n!!! ASSERT %s:%d\r\n", file, line);
    console_panic_write(buf);
    fatal_loop();
}
