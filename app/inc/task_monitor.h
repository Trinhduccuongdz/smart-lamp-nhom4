/**
 * @file    task_monitor.h
 * @brief   [APPLICATION] Hàm khởi tạo MonitorTask: đo tải CPU (idle hook + run-time stats), đặt bit READY khi khởi động.
 * @owner   Người 3
 */
#ifndef TASK_MONITOR_H
#define TASK_MONITOR_H

#include <stdbool.h>

void task_monitor_create(void);
void monitor_set_periodic_log(bool enable);   /* lệnh "log 0|1" của ConsoleTask */

#endif /* TASK_MONITOR_H */
