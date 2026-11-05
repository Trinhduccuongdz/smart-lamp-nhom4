/**
 * @file    cpu_monitor.h
 * @brief   [MIDDLEWARES] Đo tải CPU và mức dùng stack của hệ thống FreeRTOS.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * Hai phương pháp đo, dùng để đối chiếu với nhau:
 *
 * (1) Idle hook (phương pháp thầy gợi ý)
 *     vApplicationIdleHook() tăng một biến đếm mỗi vòng lặp của Idle task.
 *     - Hiệu chuẩn: khi chưa task ứng dụng nào chạy, đếm được idle_max lần/giây (CPU rảnh 100%).
 *     - Khi chạy: đếm được idle_now lần/giây.
 *     - Tải CPU = 100% x (1 - idle_now / idle_max).
 *
 * (2) Run-time stats của FreeRTOS (configGENERATE_RUN_TIME_STATS = 1)
 *     Kernel cộng dồn thời gian chạy của TỪNG task theo một bộ đếm nhanh (TIM2, 20 kHz).
 *     Mỗi giây lấy hiệu số giữa hai lần đọc -> % CPU của từng task trong giây vừa qua.
 *     Tải CPU = 100% - % của Idle task.
 *
 * Khi bật chế độ ngủ (__WFI() trong idle hook), CPU dừng giữa hai ngắt nên biến đếm idle
 * tăng rất chậm -> phương pháp (1) không còn đúng; khi đó dùng kết quả phương pháp (2).
 */
#ifndef CPU_MONITOR_H
#define CPU_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"

#define CPU_MON_MAX_TASKS 12U

typedef struct {
    char     name[configMAX_TASK_NAME_LEN];
    uint8_t  priority;
    uint16_t cpu_x10;          /* % CPU trong cửa sổ đo vừa qua, nhân 10 (123 = 12.3%) */
    uint16_t stack_free_words; /* stack high water mark: số word còn trống ít nhất từng có */
} cpu_task_stat_t;

typedef struct {
    uint32_t        uptime_s;
    uint32_t        idle_count;        /* số lần idle hook trong cửa sổ vừa qua (quy về 1 s) */
    uint32_t        idle_max;          /* giá trị hiệu chuẩn (CPU rảnh 100%) */
    uint16_t        load_idlehook_x10; /* tải CPU theo phương pháp (1) */
    uint16_t        load_runtime_x10;  /* tải CPU theo phương pháp (2) */
    bool            sleep_enabled;
    uint32_t        heap_free;         /* byte heap FreeRTOS còn trống */
    uint32_t        heap_min_free;     /* byte heap trống ít nhất từng có */
    uint8_t         task_count;
    cpu_task_stat_t tasks[CPU_MON_MAX_TASKS];
} cpu_stats_t;

/** Tạo mutex bảo vệ bảng thống kê. Gọi trước vTaskStartScheduler(). */
void cpu_monitor_init(void);

/* Hai hàm dưới được FreeRTOS gọi qua macro trong FreeRTOSConfig.h */
void     cpu_monitor_timebase_init(void);   /* portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() */
uint32_t cpu_monitor_timebase_get(void);    /* portGET_RUN_TIME_COUNTER_VALUE() */

/** Hiệu chuẩn: gọi begin, để CPU rảnh hoàn toàn khoảng 1 s, rồi gọi end. Trả về idle_max. */
void     cpu_monitor_calibrate_begin(void);
uint32_t cpu_monitor_calibrate_end(void);

/** Lấy mẫu: tính tải CPU cho cửa sổ từ lần gọi trước đến giờ. Gọi định kỳ (1 s). */
void cpu_monitor_sample(void);

/** Sao chép bảng thống kê mới nhất (có mutex bảo vệ). */
bool cpu_monitor_get(cpu_stats_t *out, TickType_t wait);

/** Bật/tắt cho CPU ngủ (__WFI) trong idle hook. */
void cpu_monitor_set_sleep(bool enable);
bool cpu_monitor_get_sleep(void);

#endif /* CPU_MONITOR_H */
