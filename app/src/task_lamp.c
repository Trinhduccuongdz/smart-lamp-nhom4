/**
 * @file    task_lamp.c
 * @brief   LampTask: nhận sự kiện từ qEvents, chạy máy trạng thái đèn + menu,
 *          điều khiển software timer sáng/tối dần, đếm ngược theo RTC (MANUAL),
 *          lưu cài đặt vào Flash, công bố trạng thái qua qLampView.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Firmware hướng sự kiện (event-driven): task chỉ chạy khi có sự kiện trong qEvents,
 * còn lại nằm ở trạng thái Blocked, không tốn CPU.
 * Đồng hồ: ngắt giây của RTC gửi EV_RTC_SECOND vào qEvents (xQueueSendToBackFromISR).
 */
#include "app_ipc.h"
#include "task_lamp.h"
#include "app_cfg.h"
#include "board.h"
#include "lamp_fsm.h"
#include "menu.h"
#include "rtc_clock.h"
#include "settings_store.h"
#include "task.h"
#include "timers.h"

static TimerHandle_t s_tmr_fade;   // software timer auto-reload 100 ms: mỗi nhịp tăng/giảm level 1 bậc

static lamp_fsm_t      s_fsm; // luu trang thai hien tai cua may trang thai fsm
static menu_t          s_menu; // luu trang thai hien tai cua menu
static lamp_settings_t s_settings;  // luu cau hinh va cai dat cua he thong den
static uint16_t        s_dark_events; // bien dem so luong su kien troi toi
static uint16_t        s_remain_s;  // đếm ngược còn lại (giây), 0 = không đếm va tat den
static uint16_t        s_total_s;   // tổng thời gian đếm ngược ban dau
static uint32_t        s_now_s;     // thoi gian hien tai cua he thong theo RTC 
static const char     *s_rtc_src = "?";  // thong bao xem dong ho RTC dang chay bang nguon LSE hay LSI

/* Callback software timer chạy trong Timer daemon: chỉ gửi sự kiện, không chờ (block time 0) */
static void on_fade_timer(TimerHandle_t t)
{
    (void)t;
    (void)app_post_event(EV_FADE_TICK, 0, 0, 0);
}

/* Chạy trong ISR của RTC (mỗi giây) */
static void on_rtc_second(void)
{
    BaseType_t woken = pdFALSE;
    (void)app_post_event_from_isr(EV_RTC_SECOND, 0, 0, &woken);
    portYIELD_FROM_ISR(woken);
}
// tiep nhan bien co mat na bit hanh dong tu may trang thai den led tra ve de thi hanh tac vu phan mem tuong ung
static void apply_actions(uint8_t act)
{
		// chuyen doi thoi gian 10ms thanh so tick cua FreeRTOS de lam thoi gian cho toi da khi goi cac ham thao tac vs Software timer
    const TickType_t wait = pdMS_TO_TICKS(10);
		// 
    if ((act & LAMP_ACT_FADE_STOP) != 0U){ 
				(void)xTimerStop(s_tmr_fade, wait); 
		}
		// Xu ly dung bo dem thoi gian tat den 
    if ((act & LAMP_ACT_OFF_TIMER_STOP) != 0U){ 
				s_remain_s = 0U; s_total_s = 0U; 
		}
		// xu ly khoi dong lai hieu ung mo dan
    if ((act & LAMP_ACT_FADE_START) != 0U){ 
				(void)xTimerReset(s_tmr_fade, wait); 
		}
    if ((act & LAMP_ACT_OFF_TIMER_START) != 0U) {
				// quy doi tu phut sang giay
        uint16_t total = (uint16_t)((uint16_t)s_settings.off_timer_min * 60U);
				// neu he thong hien tai khong co tien trinh dem nguoc hoac la dang dem dung so phut do thi nguoi dung thay doi muc hen gio moi
        if (s_remain_s == 0U || s_total_s != total) {   /* đang đếm cùng thời lượng thì giữ nguyên */
            s_total_s = total;
            s_remain_s = total;
            log_printf("[LAMP] Hen %u ph", (unsigned)s_settings.off_timer_min);
        }
				
    }
}

static void publish_view(void)
{
    lamp_view_t v;
    v.state = (uint8_t)s_fsm.state;
    v.level = s_fsm.level;
    v.dark = s_fsm.dark;
    v.settings = s_settings;
    v.menu = s_menu;
    v.dark_events = s_dark_events;
    v.remain_s = s_remain_s;
    v.total_s = s_total_s;
    v.now_s = s_now_s;
    (void)xQueueOverwrite(g_qLampView, &v);   /* mailbox: luôn giữ giá trị mới nhất */
}

static void save_settings(void)
{
    log_printf("[LAMP] %s %s sang %u icon %u hen %u",
               s_settings.auto_on ? "AUTO" : "MANUAL", lamp_mode_name(s_settings.mode),
               (unsigned)s_settings.max_level, (unsigned)s_settings.icon,
               (unsigned)s_settings.off_timer_min);
    if (!settings_store_save(&s_settings, (uint16_t)sizeof(s_settings))) {
        log_printf("[LAMP] Loi Flash");
    }
}

static void apply_setting(uint8_t param, uint16_t value)
{
    switch (param) {
    case SET_MODE:      s_settings.mode = (uint8_t)value; break;
    case SET_MAX_LEVEL: s_settings.max_level = (uint8_t)value; break;
    case SET_OFF_TIMER: s_settings.off_timer_min = (uint8_t)(value > 255U ? 255U : value); break;
    case SET_AUTO:      s_settings.auto_on = (uint8_t)value; break;
    case SET_ICON:      s_settings.icon = (uint8_t)(value > 255U ? 255U : value);
                        lamp_apply_icon_scene(&s_settings); break;
    default: break;
    }
    lamp_settings_sanitize(&s_settings);
}

// cap nhat thoi gian moi cho he thong, dong bo hoa bien luu tru thoi gian cuc bo, in log bao cao
static void set_clock(uint32_t sec_of_day)
{
		// goi ham tang middleware de ghi gia tri thoi gian moi
    rtc_clock_set(sec_of_day);
		// Gan gia tri giay trong ngay vua dat vao bien tinh nay de cac task khac dung chung
    s_now_s = sec_of_day % RTC_SECONDS_PER_DAY;
		// in log thong bao qua cong uart
    log_printf("[RTC] Dat gio %02u:%02u:%02u", (unsigned)(s_now_s / 3600U),
               (unsigned)(s_now_s / 60U % 60U), (unsigned)(s_now_s % 60U));
}

// anh xa cac ma su kien tho cua he thong thanh cac ma phim dieu khien menu tuong ung
static menu_key_t key_of(uint8_t type)
{
    switch (type) {
    case EV_BTN_NEXT:      return MENU_KEY_NEXT;
    case EV_BTN_OK:        return MENU_KEY_OK;
    case EV_BTN_NEXT_HOLD: return MENU_KEY_NEXT_HOLD;
    case EV_BTN_OK_HOLD:   return MENU_KEY_OK_HOLD;
    case EV_BTN_UP:        return MENU_KEY_UP;
    default:               return MENU_KEY_DOWN;
    }
}

// dieu phoi su kien trung tam cho toan bo he thong den thong minh
// khi lamptask lay duoc su kien tu qEvents, no se chuyen su kien do vao ham nay de phan loai 
// xem la trang thai cua cua den va menu can phan ung the nao
static uint8_t handle_event(const app_event_t *ev)
{
    switch (ev->type) {
		/* Nhom su kien cam bien va nhip thoi gian co ban */
    case EV_SENSOR_DARK: // tin hieu tu cam bien quang khi moi truong -> toi
        s_dark_events++; // bien dem troi toi tang len 1
				// tra ve gia tri goi ham may trang thai de truyen vao dau vao tuong ung, FSM quyet dinh bat den tu dong neu dang o auto
        return lamp_fsm_handle(&s_fsm, LAMP_IN_DARK, &s_settings);
    case EV_SENSOR_LIGHT: // tin hieu tu cam bien quang khi moi truong -> sang
				// tuong tu o tren nhung la FSM quyet dinh tat den
        return lamp_fsm_handle(&s_fsm, LAMP_IN_LIGHT, &s_settings);
		
    case EV_FADE_TICK: // Nhan nhip dinh ky 100ms tu softwaretimer mo dan
				// chuyen tin hieu thanh LAMP_IN_.. de FSM tang hoac giam do sang cua den them 1 bac cho den khi dat dich
        return lamp_fsm_handle(&s_fsm, LAMP_IN_FADE_TICK, &s_settings);

    case EV_RTC_SECOND: // su kien dong ho thoi gian thuc
        s_now_s = rtc_clock_now();
        if (s_remain_s > 0U) {
            s_remain_s--;
            if (s_remain_s == 0U) {
                log_printf("[LAMP] Het gio, tat den");
                return lamp_fsm_handle(&s_fsm, LAMP_IN_OFF_TIMEOUT, &s_settings);
            }
        }
        return LAMP_ACT_NONE;
		/* nhom su kien nut bam vat ly */
    case EV_BTN_NEXT: case EV_BTN_OK: case EV_BTN_NEXT_HOLD: case EV_BTN_OK_HOLD:
    case EV_BTN_UP: case EV_BTN_DOWN: {
				// Cap nhat thoi gian phut hien tai cho menu
        s_menu.now_min = (uint16_t)(s_now_s / 60U);
				// Goi ham menu_handle de xu ly logic dieu huong/ chinh sua thong so dua tren ham dich phim key_of
        uint8_t res = menu_handle(&s_menu, key_of(ev->type), &s_settings);
				// Neu nguoi dung yeu cau bat tat den nhanh qua menu, goi ham FSM xu ly 
        if ((res & MENU_RES_TOGGLE_LAMP) != 0U) {
            return lamp_fsm_handle(&s_fsm, LAMP_IN_TOGGLE, &s_settings);
        }
				// Neu co yeu cau cap nhat gio, goi ham set_clock
        if ((res & MENU_RES_SET_CLOCK) != 0U) {
            set_clock((uint32_t)s_menu.clock_min * 60U);
        }
				// Neu co thay doi cai dat, tien hanh luu cau hinh vao Flash
        if ((res & MENU_RES_SETTINGS) != 0U) {
            save_settings();
            uint8_t act = lamp_fsm_handle(&s_fsm, LAMP_IN_SETTINGS, &s_settings);
            if ((res & MENU_RES_SCENE) != 0U) {          /* chọn icon = bật cảnh */
                apply_actions(act);
                act = lamp_fsm_handle(&s_fsm, LAMP_IN_ON_REQ, &s_settings);
            }
            return act;
        }
        return LAMP_ACT_NONE;
    }
		// Nhom su kien dieu khien tu xa qua Console/Uart
    case EV_CMD_LAMP:
        if (ev->value == 0U) { return lamp_fsm_handle(&s_fsm, LAMP_IN_OFF_REQ, &s_settings); }
        if (ev->value == 1U) { return lamp_fsm_handle(&s_fsm, LAMP_IN_ON_REQ, &s_settings); }
        return lamp_fsm_handle(&s_fsm, LAMP_IN_TOGGLE, &s_settings);

    case EV_CMD_SET: {
        apply_setting(ev->param, ev->value);
        save_settings();
        uint8_t act = lamp_fsm_handle(&s_fsm, LAMP_IN_SETTINGS, &s_settings);
        if (ev->param == SET_ICON) {                     /* chọn icon = bật cảnh */
            apply_actions(act);
            act = lamp_fsm_handle(&s_fsm, LAMP_IN_ON_REQ, &s_settings);
        }
        return act;
    }

    case EV_CMD_CLOCK:
        set_clock((uint32_t)ev->param * 3600U + ev->value);
        return LAMP_ACT_NONE;
		// neu su kien truyen vao khong khop voi bat ky loai nao o tren, thi khong co hanh dong phan mem nao thuc thi
    default:
        return LAMP_ACT_NONE;
    }
}
// task huong su kien, se ngu cho den khi co su kien xuat hien trong hang doi
static void lamp_task(void *arg)
{
    (void)arg; // bo qua tham so truyen vao khi tao task
		// chung nao ma monitor chua hieu chuan xong va bat bit EVT_SYSTEM_READY thi van o day
    app_wait_system_ready();
		// sau khi monitor task da hieu chuan xong roi, các ngoai vi và he thong on dinh hoan toan
		// lay thoi gian hien tai tu phan cung RTC roi in log bao cao cho nguon xung nhip cung gio phut khoi diem
    s_now_s = rtc_clock_now();
    log_printf("[RTC] %s %02u:%02u", s_rtc_src,
               (unsigned)(s_now_s / 3600U), (unsigned)(s_now_s / 60U % 60U));
		// Khi monitor task chay xong, thi cai trang ban dau ngay luc moi khoi tao
		// day trang thai do vao mailbox de DisplayTask co du lieu ve ngay lap tuc
    publish_view();

    for (;;) {
        app_event_t ev;
				// task cho vo han cho den khi co su kien day vao queue
        if (xQueueReceive(g_qEvents, &ev, portMAX_DELAY) != pdPASS) {
            continue;
        }
				// luu trang thai hien tai cua state machine lamp truoc khi xu ly su kien de lat nua so sanh xem tt co bi thay doi hay khong
        lamp_state_t before = s_fsm.state;
				// goi ham handle_event de phan loai xem su kien do la gi va tra ve bien mat na bit hanh dong act 8 bit
        uint8_t act = handle_event(&ev);
				// doc bien act vua nhan duoc de goi ham thuc hien cac tac vu phan mem tuong ung
        apply_actions(act);
				// Neu trang thai moi khac trang thai cu
        if (s_fsm.state != before) {
						// neu khac nhau thi tu dong in dong log thong bao qua cong uart
            log_printf("[LAMP] %s -> %s", lamp_state_name(before), lamp_state_name(s_fsm.state));
        }
				// cap nhat giao dien 
        publish_view();
    }
}

// Khoi tao toan bo tai nguyen, cau hinh, FSM, SoftwareTimer, va tao FreeRTOS Task cho module dieu khien lamp trc khi Scheduler van hanh
void task_lamp_create(void)
{
    if (!settings_store_load(&s_settings, (uint16_t)sizeof(s_settings))) {
        lamp_settings_default(&s_settings);   /* lần đầu / Flash trống */
    }
    lamp_settings_sanitize(&s_settings);
		// dua FSM cua lamp ve trang thai goc ban dau, thuong la den tat hoac trang thai mac dinh khi cap nguon
    lamp_fsm_init(&s_fsm);
		// khoi tao cau truc du lieu cho he thong menu hien thi tren man hinh oled (dua con tro menu ve trang chu)
    menu_init(&s_menu);

		// Cap phat Software Timer cho hieu ung mo dan, che do auto Reload
		// Ham callback on_fade_timer ci nhiem vu la gui sk EV_FADE_TICK vao hang doi
    s_tmr_fade = xTimerCreate("fade", pdMS_TO_TICKS(FADE_STEP_MS), pdTRUE, NULL, on_fade_timer);
		// kiem tra an toan, neu heap FreeRTOS ko du de cap thi he thong se dung lai
    configASSERT(s_tmr_fade);

    /* RTC khởi động trước khi scheduler chạy; ngắt giây chỉ gửi vào qEvents (đã tạo ở app_ipc_init) */
    s_rtc_src = rtc_clock_init(BOARD_IRQ_PRIO_RTC, on_rtc_second);
		// Tao LampTask
    BaseType_t ok = xTaskCreate(lamp_task, "Lamp", STACK_LAMP, NULL, PRIO_LAMP, NULL);
    configASSERT(ok == pdPASS);
}
