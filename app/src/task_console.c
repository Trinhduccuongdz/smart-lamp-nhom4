/**
 * @file    task_console.c
 * @brief   [APPLICATION] ConsoleTask: nhận lệnh từ máy tính (USB-TTL), phân tích, chuyển thành
 *          sự kiện cho LampTask hoặc trả về thông tin trạng thái / tải CPU.
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 *
 * ISR USART1 gom ký tự vào buffer; đủ một dòng thì vTaskNotifyGiveFromISR đánh thức task này.
 * Lệnh (kết thúc bằng Enter, cấu hình terminal 115200 8N1):
 *   help                 danh sách lệnh
 *   status               trạng thái đèn
 *   on | off | toggle    bật / tắt / đảo đèn
 *   mode <0..2>          0 FULL, 1 BREATH, 2 WAVE
 *   bright <1..16>       độ sáng tối đa
 *   timer <phut>         MANUAL: thời gian sáng N phút, đếm ngược trên ma trận (0 = không)
 *   auto <0|1>           1 = AUTO (theo cảm biến), 0 = MANUAL
 *   icon <0..15>         icon hiển thị trên OLED ở MANUAL
 *   time [hh:mm[:ss]]    xem / đặt giờ RTC
 *   cpu                  bảng tải CPU + stack của từng task
 *   sleep <0|1>          cho CPU ngủ (__WFI) khi rảnh
 *   log <0|1>            in tải CPU mỗi giây (để vẽ đồ thị trên máy tính)
 */
#include "app_ipc.h"
#include "task_console.h"
#include "task_monitor.h"
#include "app_cfg.h"
#include "console.h"
#include "cpu_monitor.h"
#include "lamp_fsm.h"
#include "task.h"
#include <string.h>

static TaskHandle_t s_task;

/* Ngữ cảnh ISR: console middleware báo đã có một dòng hoàn chỉnh */
static void on_line_isr(void)
{
    BaseType_t woken = pdFALSE;
    if (s_task != NULL) {
        vTaskNotifyGiveFromISR(s_task, &woken);
    }
    portYIELD_FROM_ISR(woken);
}

/* Tách "lenh so": trả về con trỏ tới tham số (hoặc NULL) và cắt chuỗi lệnh */
static const char *split_arg(char *line)
{
    char *p = line;
    while (*p != '\0' && *p != ' ') { p++; }
    if (*p == '\0') { return NULL; }
    *p++ = '\0';
    while (*p == ' ') { p++; }
    return (*p != '\0') ? p : NULL;
}

static bool parse_uint(const char *s, uint32_t *out)
{
    if (s == NULL || *s == '\0') { return false; }
    uint32_t v = 0;
    while (*s != '\0') {
        if (*s < '0' || *s > '9') { return false; }
        v = v * 10U + (uint32_t)(*s++ - '0');
        if (v > 100000U) { return false; }
    }
    *out = v;
    return true;
}

static void cmd_help(void)
{
    log_printf("on off toggle status cpu time [hh:mm]");
    log_printf("mode 0-2 bright 1-16 timer 0-60 auto 0/1");
    log_printf("icon 0-15 sleep 0/1 log 0/1");
}

static void cmd_status(void)
{
    lamp_view_t v;
    if (xQueuePeek(g_qLampView, &v, 0) != pdPASS) {
        log_printf("Chua co");
        return;
    }
    log_printf("Den %s muc %u/%u troi %s toi %u",
               lamp_state_name((lamp_state_t)v.state), (unsigned)v.level,
               (unsigned)v.settings.max_level, v.dark ? "TOI" : "SANG", (unsigned)v.dark_events);
    log_printf("%s %s icon %u hen %u", v.settings.auto_on ? "AUTO" : "MANUAL",
               lamp_mode_name(v.settings.mode), (unsigned)v.settings.icon, (unsigned)v.settings.off_timer_min);
    log_printf("Gio %02u:%02u:%02u con %us", (unsigned)(v.now_s / 3600U), (unsigned)(v.now_s / 60U % 60U),
               (unsigned)(v.now_s % 60U), (unsigned)v.remain_s);
}

static void cmd_cpu(void)
{
    static cpu_stats_t st;   /* static: tránh chiếm ~240 byte stack của task */
    if (!cpu_monitor_get(&st, pdMS_TO_TICKS(50))) {
        log_printf("Loi");
        return;
    }
    log_printf("Up %lus ngu %s heap %lu min %lu",
               (unsigned long)st.uptime_s, st.sleep_enabled ? "BAT" : "TAT",
               (unsigned long)st.heap_free, (unsigned long)st.heap_min_free);
    log_printf("CPU idle %u.%u%% rt %u.%u%%",
               (unsigned)(st.load_idlehook_x10 / 10U), (unsigned)(st.load_idlehook_x10 % 10U),
               (unsigned)(st.load_runtime_x10 / 10U), (unsigned)(st.load_runtime_x10 % 10U));
    log_printf("idle %lu/%lu lan/s",
               (unsigned long)st.idle_count, (unsigned long)st.idle_max);
    log_printf("Task       Pr   CPU  Stack");
    for (uint8_t i = 0; i < st.task_count; i++) {
        const cpu_task_stat_t *t = &st.tasks[i];
        log_printf("%-10s %2u %3u.%u%% %4u", t->name, (unsigned)t->priority,
                   (unsigned)(t->cpu_x10 / 10U), (unsigned)(t->cpu_x10 % 10U),
                   (unsigned)t->stack_free_words);
    }
}

/* "hh:mm" hoặc "hh:mm:ss" -> giây trong ngày */
static bool parse_time(const char *s, uint32_t *out)
{
    uint32_t part[3] = { 0, 0, 0 };
    uint8_t n = 0;
    bool digit = false;
    if (s == NULL) { return false; }
    for (; *s != '\0'; s++) {
        if (*s >= '0' && *s <= '9') {
            part[n] = part[n] * 10U + (uint32_t)(*s - '0');
            digit = true;
            if (part[n] > 99U) { return false; }
        } else if (*s == ':' && digit && n < 2U) {
            n++;
            digit = false;
        } else {
            return false;
        }
    }
    if (!digit || n == 0U || part[0] > 23U || part[1] > 59U || part[2] > 59U) { return false; }
    *out = part[0] * 3600U + part[1] * 60U + part[2];
    return true;
}

static void cmd_time(const char *arg)
{
    lamp_view_t v;
    uint32_t t;
    if (arg == NULL) {
        if (xQueuePeek(g_qLampView, &v, 0) == pdPASS) {
            log_printf("Gio %02u:%02u:%02u", (unsigned)(v.now_s / 3600U),
                       (unsigned)(v.now_s / 60U % 60U), (unsigned)(v.now_s % 60U));
        }
        return;
    }
    if (!parse_time(arg, &t)) {
        log_printf("Sai: time hh:mm");
        return;
    }
    (void)app_post_event(EV_CMD_CLOCK, (uint8_t)(t / 3600U), (uint16_t)(t % 3600U), pdMS_TO_TICKS(50));
}

static void handle_line(char *line)
{
    const char *arg = split_arg(line);
    uint32_t v = 0;
    bool has_num = parse_uint(arg, &v);

    if (strcmp(line, "help") == 0)        { cmd_help(); }
    else if (strcmp(line, "status") == 0) { cmd_status(); }
    else if (strcmp(line, "on") == 0)     { (void)app_post_event(EV_CMD_LAMP, 0, 1, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "off") == 0)    { (void)app_post_event(EV_CMD_LAMP, 0, 0, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "toggle") == 0) { (void)app_post_event(EV_CMD_LAMP, 0, 2, pdMS_TO_TICKS(50)); }
    else if (strcmp(line, "mode") == 0 && has_num && v < LAMP_MODE_COUNT) {
        (void)app_post_event(EV_CMD_SET, SET_MODE, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "bright") == 0 && has_num && v >= 1U && v <= LAMP_LEVEL_MAX) {
        (void)app_post_event(EV_CMD_SET, SET_MAX_LEVEL, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "timer") == 0 && has_num && v <= LAMP_OFF_TIMER_MAX) {
        (void)app_post_event(EV_CMD_SET, SET_OFF_TIMER, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "auto") == 0 && has_num && v <= 1U) {
        (void)app_post_event(EV_CMD_SET, SET_AUTO, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "icon") == 0 && has_num && v < LAMP_ICON_COUNT) {
        (void)app_post_event(EV_CMD_SET, SET_ICON, (uint16_t)v, pdMS_TO_TICKS(50));
    } else if (strcmp(line, "time") == 0) {
        cmd_time(arg);
    } else if (strcmp(line, "cpu") == 0) {
        cmd_cpu();
    } else if (strcmp(line, "sleep") == 0 && has_num && v <= 1U) {
        cpu_monitor_set_sleep(v == 1U);
        log_printf("Ngu %s", (v == 1U) ? "BAT" : "TAT");
    } else if (strcmp(line, "log") == 0 && has_num && v <= 1U) {
        monitor_set_periodic_log(v == 1U);
    } else {
        log_printf("Sai lenh: %s", line);
        return;
    }
}

static void console_task(void *arg)
{
    (void)arg;
    char line[CONSOLE_LINE_MAX];

    app_wait_system_ready();
    log_printf("Smart Lamp - go help");

    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   /* ngủ đến khi ISR báo có dòng mới */
        while (console_readline(line, sizeof(line))) {
            handle_line(line);
        }
    }
}

void task_console_create(void)
{
    console_init(CONSOLE_BAUD, on_line_isr);
    BaseType_t ok = xTaskCreate(console_task, "Console", STACK_CONSOLE, NULL, PRIO_CONSOLE, &s_task);
    configASSERT(ok == pdPASS);
}
