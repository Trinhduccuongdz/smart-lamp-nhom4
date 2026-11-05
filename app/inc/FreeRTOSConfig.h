/**
 * @file    FreeRTOSConfig.h
 * @brief   Cấu hình FreeRTOS cho đề tài đèn thông minh (STM32F103C8, Cortex-M3, 72 MHz).
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống) - thay đổi phải báo cả nhóm
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* ---------------- Lập lịch ---------------- */
#define configUSE_PREEMPTION                    1   /* lập lịch chiếm quyền ưu tiên cố định */
#define configUSE_TIME_SLICING                  1   /* task cùng ưu tiên chia CPU theo tick */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1   /* dùng lệnh CLZ của Cortex-M3 */
#define configUSE_TICKLESS_IDLE                 0
#define configCPU_CLOCK_HZ                      72000000UL
#define configTICK_RATE_HZ                      1000U   /* tick 1 ms */
#define configMAX_PRIORITIES                    6
#define configMINIMAL_STACK_SIZE                128     /* word (= 512 byte), dùng cho Idle task */
#define configMAX_TASK_NAME_LEN                 10
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configUSE_NEWLIB_REENTRANT              0

/* ---------------- Bộ nhớ ---------------- */
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configTOTAL_HEAP_SIZE                   (12U * 1024U)   /* heap_4.c (first-fit, gộp khối trống) */
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ---------------- Hook ---------------- */
#define configUSE_IDLE_HOOK                     1   /* cpu_monitor.c: đếm idle + __WFI() */
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2   /* phương pháp 2: kiểm tra mẫu ở cuối stack */
#define configUSE_MALLOC_FAILED_HOOK            1
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

/* ---------------- Thống kê thời gian chạy (đo % CPU từng task) ---------------- */
#define configGENERATE_RUN_TIME_STATS           1
#define configUSE_TRACE_FACILITY                1   /* cần cho uxTaskGetSystemState() */
#define configUSE_STATS_FORMATTING_FUNCTIONS    0   /* tự định dạng, không dùng sprintf */
void     cpu_monitor_timebase_init(void);
uint32_t cpu_monitor_timebase_get(void);
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() cpu_monitor_timebase_init()
#define portGET_RUN_TIME_COUNTER_VALUE()         cpu_monitor_timebase_get()

/* ---------------- Software timer ---------------- */
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            160

/* ---------------- Ngắt (Cortex-M3, STM32F1 có 4 bit ưu tiên) ----------------
 * Số càng NHỎ ưu tiên logic càng CAO.
 *  - Tick (SysTick) và PendSV chạy ở mức thấp nhất: 15.
 *  - ISR gọi API ...FromISR phải có số ưu tiên >= 5 (không cao hơn ngưỡng).
 *  - ISR có số < 5 (ví dụ TIM2 = 4) không bị kernel che nhưng KHÔNG được gọi API FreeRTOS. */
#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ---------------- Gỡ lỗi ---------------- */
void app_assert_failed(const char *file, int line);
/* Keil ARMCC: __MODULE__ chỉ là tên file (không kèm đường dẫn) -> chuỗi ngắn, tiết kiệm Flash */
#if defined(__CC_ARM)
#define APP_ASSERT_FILE __MODULE__
#else
#define APP_ASSERT_FILE __FILE__
#endif
#define configASSERT(x) do { if ((x) == 0) { app_assert_failed(APP_ASSERT_FILE, __LINE__); } } while (0)

/* ---------------- API được biên dịch ---------------- */
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_vTaskDelete                     0
#define INCLUDE_vTaskSuspend                    1   /* portMAX_DELAY = chờ vô hạn */
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTimerPendFunctionCall          0

/* Ánh xạ handler của port FreeRTOS vào tên vector trong startup_stm32f103xb.s */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
