/**
 * @file    app_ipc.h
 * @brief   [APPLICATION] "Hợp đồng" giao tiếp giữa các task. Cả nhóm thống nhất file này
 *          TRƯỚC khi code; sau đó mỗi người chỉ code task của mình.
 *
 * Quy tắc:
 *  - Task KHÔNG gọi hàm, KHÔNG đọc/ghi biến của task người khác.
 *  - Mọi trao đổi đi qua các kênh dưới đây. Kiểm tra lỗi: xem dữ liệu trên kênh đúng quy ước chưa
 *    -> sai thì lỗi ở bên gửi, đúng mà phản ứng sai thì lỗi ở bên nhận.
 *
 *  Kênh          Loại                       Gửi                              Nhận
 *  -----------   ------------------------   ------------------------------   -----------------------
 *  qEvents       queue 10 x app_event_t     InputTask, ConsoleTask, timer,   LampTask
 *                                           ngắt giây RTC
 *  qLampView     mailbox 1 x lamp_view_t    LampTask (overwrite)             DisplayTask, ConsoleTask (peek)
 *  qCpuStat      mailbox 1 x cpu_summary_t  MonitorTask (overwrite)          DisplayTask (peek)
 *  qLog          queue 16 x log_msg_t       mọi task (log_printf)            LogTask
 *  egSystem      event group                MonitorTask (bit READY)          mọi task (chờ READY)
 */
#ifndef APP_IPC_H
#define APP_IPC_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"
#include "lamp_types.h"
#include "menu.h"
#include "xprintf.h"

/* ---------------- qEvents ---------------- */
typedef enum {
    EV_SENSOR_DARK = 0,   /* InputTask: trời chuyển sang tối (đã chống nhiễu) */
    EV_SENSOR_LIGHT,      /* InputTask: trời chuyển sang sáng */
    EV_BTN_NEXT,          /* InputTask: nhấn ngắn NEXT */
    EV_BTN_OK,            /* InputTask: nhấn ngắn OK */
    EV_BTN_NEXT_HOLD,     /* InputTask: nhấn giữ NEXT */
    EV_BTN_OK_HOLD,       /* InputTask: nhấn giữ OK */
    EV_FADE_TICK,         /* tmrFade: bước sáng/tối dần */
    EV_RTC_SECOND,        /* ngắt giây RTC: cập nhật đồng hồ, đếm ngược */
    EV_CMD_LAMP,          /* ConsoleTask: value 0 = tắt, 1 = bật, 2 = đảo */
    EV_CMD_SET,           /* ConsoleTask: param = app_setting_id_t, value = giá trị mới */
    EV_BTN_UP,            /* InputTask: nhấn UP */
    EV_BTN_DOWN,          /* InputTask: nhấn DOWN */
    EV_CMD_CLOCK          /* ConsoleTask: đặt giờ RTC, param = giờ, value = phút * 60 + giây */
} app_event_type_t;

typedef enum { SET_MODE = 0, SET_MAX_LEVEL, SET_OFF_TIMER, SET_AUTO, SET_ICON } app_setting_id_t;

typedef struct {
    uint8_t  type;    /* app_event_type_t */
    uint8_t  param;
    uint16_t value;
} app_event_t;

/* ---------------- qLampView (mailbox) ---------------- */
typedef struct {
    uint8_t         state;        /* lamp_state_t */
    uint8_t         level;        /* 0..16 */
    bool            dark;
    lamp_settings_t settings;
    menu_t          menu;         /* để DisplayTask vẽ menu */
    uint16_t        dark_events;  /* số lần trời chuyển tối từ khi khởi động */
    uint16_t        remain_s;     /* MANUAL: số giây còn lại của đếm ngược (0 = không đếm) */
    uint16_t        total_s;      /* MANUAL: tổng thời gian đếm ngược */
    uint32_t        now_s;        /* giờ RTC: giây kể từ 00:00:00 */
} lamp_view_t;

/* ---------------- qCpuStat (mailbox) ---------------- */
typedef struct {
    uint16_t load_idlehook_x10;
    uint16_t load_runtime_x10;
    bool     sleep_enabled;
    uint32_t uptime_s;
} cpu_summary_t;

/* ---------------- qLog ---------------- */
#define LOG_MSG_LEN 64U
typedef struct {
    char text[LOG_MSG_LEN];
} log_msg_t;

/* ---------------- egSystem ---------------- */
#define EVT_SYSTEM_READY  (1UL << 0)   /* MonitorTask hiệu chuẩn xong, hệ thống chạy */

extern QueueHandle_t      g_qEvents;
extern QueueHandle_t      g_qLampView;
extern QueueHandle_t      g_qCpuStat;
extern QueueHandle_t      g_qLog;
extern EventGroupHandle_t g_egSystem;

/** Tạo toàn bộ queue / event group. Gọi một lần trong main() trước khi tạo task. */
void app_ipc_init(void);

/** Chặn task đến khi bit READY được đặt. */
void app_wait_system_ready(void);

/** Gửi sự kiện vào qEvents (dùng trong task hoặc callback software timer). */
bool app_post_event(uint8_t type, uint8_t param, uint16_t value, TickType_t wait);

/** Gửi sự kiện từ ISR (không chờ). Trả về pdTRUE trong *woken nếu cần chuyển ngữ cảnh. */
bool app_post_event_from_isr(uint8_t type, uint8_t param, uint16_t value, BaseType_t *woken);

/** In một dòng log (tự thêm "\r\n"). Định dạng giống printf, xem xprintf.h. */
void log_printf(const char *fmt, ...) XPRINTF_FORMAT(1, 2);

#endif /* APP_IPC_H */
