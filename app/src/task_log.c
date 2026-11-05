/**
 * @file    task_log.c
 * @brief   [APPLICATION] LogTask: gatekeeper của UART TX. Là task DUY NHẤT gọi console_write();
 *          các task khác chỉ gửi chuỗi vào qLog (slide FreeRTOS: Ví dụ 16 - Gatekeeper).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "app_ipc.h"
#include "task_log.h"
#include "app_cfg.h"
#include "console.h"
#include "task.h"
#include <string.h>

static void log_task(void *arg)
{
    (void)arg;
    app_wait_system_ready();

    for (;;) {
        log_msg_t msg;
        if (xQueueReceive(g_qLog, &msg, portMAX_DELAY) == pdPASS) {
            console_write(msg.text, strlen(msg.text));   /* DMA + chờ binary semaphore */
        }
    }
}

void task_log_create(void)
{
    BaseType_t ok = xTaskCreate(log_task, "Log", STACK_LOG, NULL, PRIO_LOG, NULL);
    configASSERT(ok == pdPASS);
}
