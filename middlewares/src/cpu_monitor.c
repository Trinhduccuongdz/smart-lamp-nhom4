/**
 * @file    cpu_monitor.c
 * @brief   [MIDDLEWARES] Đo tải CPU: idle hook + run-time stats (TIM2), mức dùng stack, heap.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 */
#include "cpu_monitor.h"
#include "hal_timer.h"
#include "board.h"
#include "semphr.h"
#include <string.h>

#define TIMEBASE_HZ  20000UL   /* gấp 20 lần tick 1 kHz, theo khuyến nghị 10..100 lần */

/* ---------- Phương pháp (1): Idle hook ---------- */
static volatile uint32_t s_idle_counter;   /* chỉ idle hook tăng, MonitorTask đọc + xóa */
static volatile bool     s_sleep_enabled;

void vApplicationIdleHook(void)
{
    s_idle_counter++;
    if (s_sleep_enabled) {
        /* Ngủ (Sleep mode) đến ngắt kế tiếp (tick 1 ms hoặc ngắt ngoại vi).
         * Ngoại vi và SysTick vẫn chạy, chỉ lõi CPU ngừng nhận clock -> giảm dòng tiêu thụ. */
        __DSB();
        __WFI();
    }
}

/* ---------- Thang thời gian cho phương pháp (2): TIM2 16 bit + phần cao 16 bit ---------- */
static volatile uint16_t s_tb_high;

static void on_tim2_overflow(void)
{
    s_tb_high++;
}

void cpu_monitor_timebase_init(void)
{
    tim2_freerun_init(TIMEBASE_HZ, BOARD_IRQ_PRIO_TIM2, on_tim2_overflow);
}

uint32_t cpu_monitor_timebase_get(void)
{
    /* Đọc 32 bit nhất quán: phần cao có thể đổi giữa hai lần đọc nếu TIM2 vừa tràn */
    uint16_t hi1, hi2, lo;
    do {
        hi1 = s_tb_high;
        lo  = tim2_get_count();
        hi2 = s_tb_high;
    } while (hi1 != hi2);
    uint32_t hi = hi1;
    /* Tràn đã xảy ra nhưng ISR chưa kịp chạy (bị che ngắt): bù thủ công */
    if (tim2_overflow_pending() && lo < 0x8000U) {
        hi++;
    }
    return (hi << 16) | lo;
}

/* ---------- Bảng thống kê dùng chung, bảo vệ bằng mutex ---------- */
static SemaphoreHandle_t s_mutex;
static cpu_stats_t       s_stats;

/* Dữ liệu làm việc riêng của MonitorTask (không chia sẻ nên không cần khóa) */
static TaskStatus_t s_status[CPU_MON_MAX_TASKS];
static struct { UBaseType_t num; uint32_t runtime; } s_prev[CPU_MON_MAX_TASKS];
static uint8_t      s_prev_n;
static uint32_t     s_prev_total;
static uint32_t     s_cal_start;
static uint32_t     s_idle_max_per_s;
static uint32_t     s_last_sample_tb;

void cpu_monitor_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    configASSERT(s_mutex != NULL);
}

void cpu_monitor_calibrate_begin(void)
{
    s_sleep_enabled = false;              /* hiệu chuẩn phải đo khi CPU không ngủ */
    s_cal_start = cpu_monitor_timebase_get();
    s_idle_counter = 0;
}

/* a * b / c bằng số 32 bit, không tràn khi (c - 1) * b < 2^32 (tránh phép chia 64 bit tốn Flash) */
static uint32_t mul_div(uint32_t a, uint32_t b, uint32_t c)
{
    return (a / c) * b + ((a % c) * b) / c;
}

uint32_t cpu_monitor_calibrate_end(void)
{
    uint32_t count = s_idle_counter;
    uint32_t now = cpu_monitor_timebase_get();
    uint32_t elapsed = now - s_cal_start;
    if (elapsed == 0U) { elapsed = 1U; }
    /* Quy về số lần/giây: count * TIMEBASE_HZ / elapsed */
    s_idle_max_per_s = mul_div(count, TIMEBASE_HZ, elapsed);
    s_idle_counter = 0;
    s_last_sample_tb = now;
    return s_idle_max_per_s;
}

static uint32_t find_prev(UBaseType_t num, bool *found)
{
    for (uint8_t i = 0; i < s_prev_n; i++) {
        if (s_prev[i].num == num) { *found = true; return s_prev[i].runtime; }
    }
    *found = false;
    return 0;
}

void cpu_monitor_sample(void)
{
    /* --- (1) idle hook --- */
    uint32_t idle_cnt = s_idle_counter;
    s_idle_counter = 0;
    uint32_t now_tb = cpu_monitor_timebase_get();
    uint32_t win = now_tb - s_last_sample_tb;
    s_last_sample_tb = now_tb;
    if (win == 0U) { win = 1U; }
    uint32_t idle_per_s = mul_div(idle_cnt, TIMEBASE_HZ, win);

    uint16_t load_ih = 0;
    if (s_idle_max_per_s > 0U && idle_per_s < s_idle_max_per_s) {
        load_ih = (uint16_t)(1000U - mul_div(idle_per_s, 1000U, s_idle_max_per_s));
    }

    /* --- (2) run-time stats: hiệu số so với lần lấy mẫu trước --- */
    uint32_t total = 0;
    UBaseType_t n = uxTaskGetSystemState(s_status, CPU_MON_MAX_TASKS, &total);
    uint32_t total_delta = total - s_prev_total;
    if (total_delta == 0U) { total_delta = 1U; }
    TaskHandle_t idle_handle = xTaskGetIdleTaskHandle();

    cpu_stats_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    uint16_t idle_x10 = 0;
    for (UBaseType_t i = 0; i < n; i++) {
        bool found;
        uint32_t prev = find_prev(s_status[i].xTaskNumber, &found);
        uint32_t delta = s_status[i].ulRunTimeCounter - (found ? prev : 0U);
        uint32_t x10r = mul_div(delta, 1000U, total_delta);
        uint16_t x10 = (uint16_t)(x10r > 1000U ? 1000U : x10r);
        if (x10 > 1000U) { x10 = 1000U; }

        cpu_task_stat_t *t = &tmp.tasks[i];
        strncpy(t->name, s_status[i].pcTaskName, configMAX_TASK_NAME_LEN - 1U);
        t->priority = (uint8_t)s_status[i].uxCurrentPriority;
        t->cpu_x10 = x10;
        t->stack_free_words = (uint16_t)s_status[i].usStackHighWaterMark;
        if (s_status[i].xHandle == idle_handle) { idle_x10 = x10; }
    }
    /* Lưu lại để lần sau tính hiệu số */
    for (UBaseType_t i = 0; i < n; i++) {
        s_prev[i].num = s_status[i].xTaskNumber;
        s_prev[i].runtime = s_status[i].ulRunTimeCounter;
    }
    s_prev_n = (uint8_t)n;
    s_prev_total = total;

    tmp.task_count = (uint8_t)n;
    tmp.uptime_s = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
    tmp.idle_count = idle_per_s;
    tmp.idle_max = s_idle_max_per_s;
    tmp.load_idlehook_x10 = load_ih;
    tmp.load_runtime_x10 = (uint16_t)(1000U - idle_x10);
    tmp.sleep_enabled = s_sleep_enabled;
    tmp.heap_free = (uint32_t)xPortGetFreeHeapSize();
    tmp.heap_min_free = (uint32_t)xPortGetMinimumEverFreeHeapSize();

    /* Ghi vào bảng dùng chung dưới sự bảo vệ của mutex */
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        s_stats = tmp;
        xSemaphoreGive(s_mutex);
    }
}

bool cpu_monitor_get(cpu_stats_t *out, TickType_t wait)
{
    if (xSemaphoreTake(s_mutex, wait) != pdTRUE) {
        return false;
    }
    *out = s_stats;
    xSemaphoreGive(s_mutex);
    return true;
}

void cpu_monitor_set_sleep(bool enable)
{
    s_sleep_enabled = enable;
}

bool cpu_monitor_get_sleep(void)
{
    return s_sleep_enabled;
}
