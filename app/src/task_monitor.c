/**
 * @file    task_monitor.c
 * @brief   [APPLICATION] MonitorTask: hiệu chuẩn idle hook, rồi mỗi giây đo tải CPU,
 *          công bố kết quả (qCpuStat), nháy LED PC13 báo hệ thống còn sống.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * Trình tự khởi động:
 *   1. Mọi task khác đang chờ bit READY của egSystem -> chỉ còn Idle task chạy.
 *   2. MonitorTask ngủ 1 s; trong lúc đó idle hook đếm được idle_max (CPU rảnh 100%).
 *   3. Đặt bit READY -> các task khác bắt đầu làm việc.
 */
#include "app_ipc.h"
#include "task_monitor.h"
#include "app_cfg.h"
#include "cpu_monitor.h"
#include "board.h"
#include "task.h"

static volatile bool s_periodic_log;

void monitor_set_periodic_log(bool enable)
{
    s_periodic_log = enable;
}

static void monitor_task(void *arg)
{
    (void)arg;

    /* --- Hiệu chuẩn --- */
    vTaskDelay(pdMS_TO_TICKS(10));
    cpu_monitor_calibrate_begin();
    vTaskDelay(pdMS_TO_TICKS(MONITOR_CALIB_MS));
    uint32_t idle_max = cpu_monitor_calibrate_end();
    log_printf("[MON] idle_max %lu lan/s", (unsigned long)idle_max);

    (void)xEventGroupSetBits(g_egSystem, EVT_SYSTEM_READY);

    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MONITOR_PERIOD_MS));

        cpu_monitor_sample();

        cpu_stats_t st;
        if (cpu_monitor_get(&st, pdMS_TO_TICKS(10))) {
            cpu_summary_t sum = {
                .load_idlehook_x10 = st.load_idlehook_x10,
                .load_runtime_x10  = st.load_runtime_x10,
                .sleep_enabled     = st.sleep_enabled,
                .uptime_s          = st.uptime_s,
            };
            (void)xQueueOverwrite(g_qCpuStat, &sum);

            if (s_periodic_log) {
                /* Định dạng CSV gọn để dán vào Excel / vẽ đồ thị */
                log_printf("CPU,%lu,%u.%u,%u.%u,%u", (unsigned long)st.uptime_s,
                           (unsigned)(st.load_idlehook_x10 / 10U), (unsigned)(st.load_idlehook_x10 % 10U),
                           (unsigned)(st.load_runtime_x10 / 10U), (unsigned)(st.load_runtime_x10 % 10U),
                           (unsigned)st.sleep_enabled);
            }
        }
        board_led_toggle();
    }
}

void task_monitor_create(void)
{
    cpu_monitor_init();
    BaseType_t ok = xTaskCreate(monitor_task, "Monitor", STACK_MONITOR, NULL, PRIO_MONITOR, NULL);
    configASSERT(ok == pdPASS);
}
