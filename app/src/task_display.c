/**
 * @file    task_display.c
 * @brief   [APPLICATION] DisplayTask: task DUY NHẤT được dùng ma trận LED (SPI1) và OLED (I2C1)
 *          -> đóng vai trò gatekeeper cho hai thiết bị hiển thị, không cần mutex cho bus.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Chạy tuần hoàn 40 ms bằng xTaskDelayUntil (chu kỳ ổn định, không trôi như vTaskDelay).
 * Đọc dữ liệu bằng xQueuePeek từ hai mailbox qLampView và qCpuStat.
 */
#include "app_ipc.h"
#include "task_display.h"
#include "app_cfg.h"
#include "lamp_fsm.h"
#include "menu.h"
#include "effects.h"
#include "icons.h"
#include "max7219.h"
#include "ssd1306.h"
#include "xprintf.h"
#include "task.h"
#include <string.h>

typedef struct {
    uint8_t rows[8U * MAX7219_NUM_DEVICES];
    uint8_t intensity;
    bool    shutdown;
} matrix_state_t;

static void update_matrix(const lamp_view_t *v, uint32_t frame, matrix_state_t *cur)
{
    if (v->level == 0U) {
        if (!cur->shutdown) {
            max7219_set_shutdown(true);
            cur->shutdown = true;
        }
        return;
    }

    matrix_state_t next;
    bool countdown = (v->settings.auto_on == 0U) && (v->remain_s > 0U) &&
                     (v->state == LAMP_ON || v->state == LAMP_FADE_IN);
    for (uint8_t d = 0; d < MAX7219_NUM_DEVICES; d++) {
        if (countdown) {
            /* MANUAL: hiển thị thời gian còn lại đếm ngược theo RTC */
            effects_render_countdown(v->remain_s, v->total_s, frame, &next.rows[d * 8U]);
        } else {
            effects_render(v->settings.mode, frame, (uint8_t)(d * 8U), &next.rows[d * 8U]);
        }
    }
    next.intensity = countdown ? (uint8_t)(v->level - 1U)
                               : effects_intensity(v->settings.mode, frame, v->level);

    /* Chỉ gửi qua SPI khi có thay đổi: giảm thời gian chiếm bus và tải CPU */
    if (memcmp(next.rows, cur->rows, sizeof(next.rows)) != 0) {
        max7219_write_frame(next.rows);
        memcpy(cur->rows, next.rows, sizeof(cur->rows));
    }
    if (next.intensity != cur->intensity) {
        max7219_set_intensity(next.intensity);
        cur->intensity = next.intensity;
    }
    if (cur->shutdown) {
        max7219_set_shutdown(false);
        cur->shutdown = false;
    }
}

static void draw_header(const lamp_view_t *v)
{
    char line[24];
    xsnprintf(line, sizeof(line), " %02u:%02u:%02u     %-6s ",
              (unsigned)(v->now_s / 3600U), (unsigned)(v->now_s / 60U % 60U), (unsigned)(v->now_s % 60U),
              v->settings.auto_on ? "AUTO" : "MANUAL");
    ssd1306_draw_string(0, 0, line, true);
}

/* Dòng chữ "Muc x/y" ở dòng line_no, thanh độ sáng khung bar_w x 6 px tại (bar_x, bar_y) */
static void draw_level_bar(const lamp_view_t *v, uint8_t line_no, uint8_t bar_x, uint8_t bar_y, uint8_t bar_w)
{
    char line[24];
    xsnprintf(line, sizeof(line), "Muc %2u/%2u", (unsigned)v->level, (unsigned)v->settings.max_level);
    ssd1306_draw_string(0, line_no, line, false);
    ssd1306_draw_rect(bar_x, bar_y, bar_w, 6);
    ssd1306_fill_rect((uint8_t)(bar_x + 2U), (uint8_t)(bar_y + 2U),
                      (uint8_t)(v->level * (bar_w - 4U) / LAMP_LEVEL_MAX), 2, true);
}

static void draw_cpu_lines(const cpu_summary_t *c)
{
    char line[24];
    xsnprintf(line, sizeof(line), "CPU %u.%u%% (rt %u.%u%%)",
              (unsigned)(c->load_idlehook_x10 / 10U), (unsigned)(c->load_idlehook_x10 % 10U),
              (unsigned)(c->load_runtime_x10 / 10U), (unsigned)(c->load_runtime_x10 % 10U));
    ssd1306_draw_string(0, 6, line, false);

    xsnprintf(line, sizeof(line), "Ngu:%s  Up:%lus", c->sleep_enabled ? "BAT" : "TAT",
              (unsigned long)c->uptime_s);
    ssd1306_draw_string(0, 7, line, false);
}

/* AUTO: trạng thái theo cảm biến */
static void draw_auto_page(const lamp_view_t *v, const cpu_summary_t *c)
{
    char line[24];
    draw_header(v);
    xsnprintf(line, sizeof(line), "Trang thai: %s", lamp_state_name((lamp_state_t)v->state));
    ssd1306_draw_string(0, 1, line, false);
    xsnprintf(line, sizeof(line), "Anh sang  : %s", v->dark ? "TOI" : "SANG");
    ssd1306_draw_string(0, 2, line, false);
    xsnprintf(line, sizeof(line), "Hieu ung  : %s", lamp_mode_name(v->settings.mode));
    ssd1306_draw_string(0, 3, line, false);
    draw_level_bar(v, 4, 62, 33, 64);
    ssd1306_draw_string(0, 5, "Tu dong theo cam bien", false);
    draw_cpu_lines(c);
}

/* MANUAL: icon đã chọn bên phải, thời gian còn lại */
static void draw_manual_page(const lamp_view_t *v, const cpu_summary_t *c)
{
    char line[24];
    draw_header(v);
    xsnprintf(line, sizeof(line), "Den: %s", lamp_state_name((lamp_state_t)v->state));
    ssd1306_draw_string(0, 1, line, false);
    xsnprintf(line, sizeof(line), "HU : %s", lamp_mode_name(v->settings.mode));
    ssd1306_draw_string(0, 2, line, false);
    draw_level_bar(v, 3, 0, 33, 86);
    ssd1306_draw_bitmap(98, 13, ICON_W, ICON_H, icon_bitmap(v->settings.icon));

    if (v->remain_s > 0U) {
        xsnprintf(line, sizeof(line), "Con lai: %02u:%02u", (unsigned)(v->remain_s / 60U),
                  (unsigned)(v->remain_s % 60U));
    } else if (v->settings.off_timer_min == 0U) {
        xsnprintf(line, sizeof(line), "Hen gio: KHONG");
    } else {
        xsnprintf(line, sizeof(line), "Hen gio: %u phut", (unsigned)v->settings.off_timer_min);
    }
    ssd1306_draw_string(0, 5, line, false);
    draw_cpu_lines(c);
}

/* Giao diện chọn icon: icon trước - icon đang chọn (có khung) - icon sau */
static void draw_icon_picker(const menu_t *m)
{
    char line[24];
    uint8_t cur = m->edit.icon;
    uint8_t prev = (uint8_t)((cur + LAMP_ICON_COUNT - 1U) % LAMP_ICON_COUNT);
    uint8_t next = (uint8_t)((cur + 1U) % LAMP_ICON_COUNT);

    ssd1306_draw_string(0, 0, "      CHON ICON      ", true);
    ssd1306_draw_bitmap(12, 16, ICON_W, ICON_H, icon_bitmap(prev));
    ssd1306_draw_bitmap(52, 16, ICON_W, ICON_H, icon_bitmap(cur));
    ssd1306_draw_bitmap(92, 16, ICON_W, ICON_H, icon_bitmap(next));
    ssd1306_draw_rect(48, 12, 32, 32);
    xsnprintf(line, sizeof(line), "%u/%u %s", (unsigned)cur + 1U, (unsigned)LAMP_ICON_COUNT, icon_name(cur));
    ssd1306_draw_string((uint8_t)((SSD1306_WIDTH - (uint8_t)(strlen(line) * 6U)) / 2U), 6, line, false);
    ssd1306_draw_string(0, 7, "UP/DN:doi OK:luu", false);
}

#define MENU_ROWS  6U   /* số mục hiển thị cùng lúc (dòng 1..6) */

static void draw_menu_page(const lamp_view_t *v)
{
    char value[12];
    char line[24];
    const menu_t *m = &v->menu;
    uint8_t first = (m->item >= MENU_ROWS) ? (uint8_t)(m->item - MENU_ROWS + 1U) : 0U;

    if (m->state == MENU_EDIT && m->item == MENU_ITEM_ICON) {
        draw_icon_picker(m);
        return;
    }
    ssd1306_draw_string(0, 0, "       CAI DAT       ", true);
    for (uint8_t r = 0; r < MENU_ROWS && (uint8_t)(first + r) < MENU_ITEM_COUNT; r++) {
        uint8_t i = (uint8_t)(first + r);
        bool selected = (i == m->item);
        bool editing = selected && (m->state == MENU_EDIT);
        /* Đang sửa thì hiện giá trị tạm (m->edit), còn lại hiện giá trị đang dùng */
        menu_item_value(m, i, editing ? &m->edit : &v->settings, value, sizeof(value));
        if (i == MENU_ITEM_EXIT) {
            xsnprintf(line, sizeof(line), "%c %s", selected ? '>' : ' ', menu_item_name(i));
        } else if (editing && i == MENU_ITEM_CLOCK) {
            /* ô đang sửa đặt trong ngoặc: [07]:30 hoặc 07:[30] */
            xsnprintf(line, sizeof(line), m->field == 0U ? "> %-8s [%02u]:%02u" : "> %-8s %02u:[%02u]",
                      menu_item_name(i), (unsigned)(m->clock_min / 60U), (unsigned)(m->clock_min % 60U));
        } else if (editing) {
            xsnprintf(line, sizeof(line), "> %-8s [%s]", menu_item_name(i), value);
        } else {
            xsnprintf(line, sizeof(line), "%c %-8s %s", selected ? '>' : ' ', menu_item_name(i), value);
        }
        ssd1306_draw_string(0, (uint8_t)(1U + r), line, selected && !editing);
    }
    if (m->state == MENU_EDIT) {
        ssd1306_draw_string(0, 7, (m->item == MENU_ITEM_CLOCK) ? "UP/DN:doi NEXT:o OK" : "UP/DN:doi   OK:luu", false);
    } else {
        ssd1306_draw_string(0, 7, "UP/DN:chon  OK:vao", false);
    }
}

static void display_task(void *arg)
{
    (void)arg;
    app_wait_system_ready();

    max7219_init();
    bool oled_ok = ssd1306_init();
    log_printf("[DISP] MAX7219 OK");
    if (oled_ok) {
        log_printf("[DISP] OLED OK 0x%X", (unsigned)ssd1306_address());
    } else {
        log_printf("[DISP] OLED loi %s", ssd1306_error_text());
    }

    matrix_state_t cur;
    memset(&cur, 0, sizeof(cur));
    cur.shutdown = true;
    cur.intensity = 0xFF;   /* giá trị không hợp lệ để lần đầu chắc chắn được ghi */

    lamp_view_t view;
    memset(&view, 0, sizeof(view));
    lamp_settings_default(&view.settings);
    cpu_summary_t cpu;
    memset(&cpu, 0, sizeof(cpu));

    uint32_t frame = 0;
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        (void)xQueuePeek(g_qLampView, &view, 0);   /* không có dữ liệu mới thì giữ bản cũ */
        update_matrix(&view, frame, &cur);

        if ((frame % DISPLAY_OLED_DIVIDER) == 0U) {
            (void)xQueuePeek(g_qCpuStat, &cpu, 0);
            ssd1306_clear();
            if (view.menu.state != MENU_CLOSED) {
                draw_menu_page(&view);
            } else if (view.settings.auto_on != 0U) {
                draw_auto_page(&view, &cpu);
            } else {
                draw_manual_page(&view, &cpu);
            }
            (void)ssd1306_update();               /* tự bỏ qua nếu khung không đổi */
        }

        frame++;
        (void)xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DISPLAY_PERIOD_MS));
    }
}

void task_display_create(void)
{
    BaseType_t ok = xTaskCreate(display_task, "Display", STACK_DISPLAY, NULL, PRIO_DISPLAY, NULL);
    configASSERT(ok == pdPASS);
}
