/**
 * @file    main.c
 * @brief   [APPLICATION] Điểm vào: khởi tạo bo mạch, tạo kênh giao tiếp, tạo task, chạy scheduler.
 *          main.c KHÔNG chứa logic chức năng và KHÔNG truy cập thanh ghi.
 *
 * Đề tài: Thiết kế đèn thông minh tự động theo ánh sáng trên nền FreeRTOS và giám sát tải CPU.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "app_ipc.h"
#include "task_input.h"
#include "task_lamp.h"
#include "task_display.h"
#include "task_console.h"
#include "task_log.h"
#include "task_monitor.h"

int main(void)
{
    board_init();          /* 72 MHz, clock GPIO, nhóm ưu tiên NVIC, LED PC13 */
    app_ipc_init();        /* qEvents, qLampView, qCpuStat, qLog, egSystem */

    /* Người 2: tạo trước để kênh log sẵn sàng */
    task_log_create();
    task_display_create();
    /* Người 3 */
    task_console_create();
    task_monitor_create();
    /* Người 1 */
    task_input_create();
    task_lamp_create();

    vTaskStartScheduler(); /* tạo Idle task + Timer daemon, không bao giờ trả về nếu đủ heap */

    for (;;) { }           /* chỉ tới đây khi thiếu heap cho Idle/Timer task */
}
