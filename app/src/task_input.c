/**
 * task_input.c
 * đọc cảm biến quang + 4 nút (NEXT, OK, UP, DOWN), chống nhiễu, sinh sự kiện vào qEvents.
 * Luồng xử lý (kỹ thuật "trì hoãn xử lý ngắt" trong slide FreeRTOS phần 2):
 *   1. ISR EXTI chỉ làm một việc: gửi task notification (bit cạnh) cho InputTask.
 *   2. InputTask khởi động lại (reset) software timer chống nhiễu của nguồn đó.
 *   3. Khi tín hiệu đứng yên đủ lâu, timer hết hạn -> callback gửi notification (bit ổn định).
 *   4. InputTask đọc mức chân, so với trạng thái cũ -> sinh sự kiện DARK/LIGHT, BTN_xxx.
 */
#include "app_ipc.h"
#include "task_input.h"
#include "app_cfg.h"
#include "light_sensor.h"
#include "button.h"
#include "task.h"
#include "timers.h"
#include <stdint.h>

/* Bit notification. Nguồn: 0 = cảm biến, 1 + button_id_t = nút (NEXT, OK, UP, DOWN) */
#define SRC_SENSOR  0U  // Kenh cho cam bien anh sang
#define SRC_BUTTON0 1U  // Kenh cho nut nhan
#define SRC_COUNT   (SRC_BUTTON0 + (uint32_t)BUTTON_COUNT) // tong so luong cac kenh dau vao can quan ly trong he thong

// Thoi diem dung: duoc goi thang tu cac ham ngat ISR de bao cho input_task biet la chan phan cung nay vua co canhlen/canhxuong
#define NB_EDGE(src)    (1UL << (src))          /* từ ISR: chân vừa đổi mức, bit 0 den bit 7 */

// Thoi diem dung: Duoc goi tu ham callback cua Software Timer chong doi de bao cho input_task biet la het thoi gian loc nhieu roi, tin hieu da on dinh roi
#define NB_STABLE(src)  (1UL << (8U + (src)))   /* từ timer: chân đã ổn định sau khi qua bo loc chong doi, bit 8 den bit 15 */

static TaskHandle_t  s_task;  // dinh danh dai dien cho InputTask
static TimerHandle_t s_debounce[SRC_COUNT]; // mang cac con tro quan ly SoftwareTimer

/* ---------- Ngữ cảnh ISR (callback từ middleware) ---------- */
// chuyen tai tin hieu ngat tu phan cung vao tien trinh xu ly dau vao input_task
static void notify_from_isr(uint32_t bits)
{		
		/* 
			Bien woken dong vai tro la 1 flag de theo doi xem: Viec gui thong bao nay co lam thuc tinh duoc 1 task nao do
			co do uu tien cao hon task hien tai dang bi chiem quyen hay khong
		*/
    BaseType_t woken = pdFALSE;
		
    if (s_task != NULL) {
				// task nhan thong bao la input_task (s_task luu handle cua input_task)
				// bits la gia tri mat na bit (thong tin ve sk suon xung hoac ma nguon nut bam) duoc truyen vao de danh dau loai su kien nao vua xay ra
        xTaskNotifyFromISR(s_task, bits, eSetBits, &woken);
    }
		// kiem tra woken, neu woken la pdTrue, marco nay se yeu cau VDK thuc hien chuyen doi ngu canh ngay lap tuc ngay khi vua thoat khoi ISR
		// Ý nghia thuc te: ngay khi nguoi dung vua buong tay khoi nut bam.
    portYIELD_FROM_ISR(woken);
}

// Trinh phuc vu ngat phan cung danh rieng cho cam bien anh sang
// Duoc kich hoat tu dong ngay tai khoanh khac chan vdk ghi nhan su thay doi suon xung tu phan cung cam bien
static void on_sensor_edge_isr(void)
{    
		// SRC_SENSOR: hang so dinh danh (ID) dai dien cho kenh phan cung cua cam bien trong he thong
    notify_from_isr(NB_EDGE(SRC_SENSOR));
}

// ISR dung chung cho tat ca cac nut bam vat ly
// Bat ky nut nao co su thay doi dien ap thi VDK se kich hoat ngat ngoai EXTI va nhay vao ham nay
static void on_button_edge_isr(button_id_t id)
{
    notify_from_isr(NB_EDGE(SRC_BUTTON0 + (uint32_t)id));
}

/* ---------- Ngữ cảnh Timer daemon ---------- */
// ham callback cua FreeRTOS SoftwareTimer, danh dau moc thoi gian da loc nhieu hoan tat
static void on_debounce_expired(TimerHandle_t t)
{
		// lay gia tri dinh danh 
    uint32_t src = (uint32_t)(uintptr_t)pvTimerGetTimerID(t);
		// Gui thong bao tin hieu on dinh sang Input Task
    xTaskNotify(s_task, NB_STABLE(src), eSetBits);
}

/* ---------- Ngữ cảnh InputTask ---------- */
// Xu ly logic thay doi trang thai anh sang cua cam bien (toi hay sang) sau khi da qua chong doi
static void handle_sensor(bool *dark_state, TickType_t *dark_since, uint16_t *dark_count)
{
		// dark=gia tri thuc te hien tai cua cam bien quang, true=toi, false=sang
    bool dark = light_sensor_is_dark();
		// so sanh gia tri moi nay voi trang thai cu truoc do
    if (dark == *dark_state) {
        return;                                  /* nhiễu: quay về mức cũ */
    }
    *dark_state = dark;
		
		// ghi nhan moc thoi gian hien tai 
    TickType_t now = xTaskGetTickCount();
		// neu la troi toi
    if (dark) {
				// tang so lan phat hien troi toi len 1 de thong ke
        (*dark_count)++;
				// luu moc thoi gian de luu lai thoi diem bat dau toi, phuc vu cho viec tinh toan khoang thoi gian troi toi keo dai bao lau
        *dark_since = now;
				// in log và gui su kien 
        log_printf("[IN] Troi TOI (lan %u)", (unsigned)*dark_count);
				// gui sk EV_SENSOR_DARK vao qevents voi thoi gian cho timeout toi da la 20ms de lamptask nhan duoc va quyet dinh bat den tu dong
        (void)app_post_event(EV_SENSOR_DARK, 0, 0, pdMS_TO_TICKS(20));
    } 
		// neu la troi sang
		else {
				// tinh toan thoi gian da toi (hien tai - toi truoc do), sau do chia cho so tick trong 1 giay de quy doi ra tong so giay ma troi da duy tri tt toi
        uint32_t secs = (uint32_t)((now - *dark_since) / configTICK_RATE_HZ);
				// in log ra terminal
        log_printf("[IN] Troi SANG (toi %lus)", (unsigned long)secs);
				// Gui sk EV_SENSOR_LIGHT vao qEvents de LampTask biet va tat den tu dong (Che do Auto)
        (void)app_post_event(EV_SENSOR_LIGHT, 0, 0, pdMS_TO_TICKS(20));
    }
}

// Dam nhiem viec xu ly logic nut bam vat ly sau khi da qua bo loc chong doi
// phan loai chi tiet (bam nhanh, bam giu,...)
static void handle_button(uint32_t src, bool *pressed_state, TickType_t *press_tick)
{
		// xac dinh chinh xac chi so nut bam trong day nut bam gan vao bien id
    button_id_t id = (button_id_t)(src - SRC_BUTTON0);
		// Goi ham o tang HAL de lay trang thai vat ly thuc te cua chan GPIO tuong ung voi nut id (true=nhan, false=nha)
    bool pressed = button_is_pressed(id);
		// neu ma trang thai moi doc pressed giong trang thai cu pressed_state thi ket thuc ham ngay
    if (pressed == *pressed_state) {
        return;
    }
		// update cho lan xu ly tiep theo
    *pressed_state = pressed;
		// Xu ly nhom nut UP/DOWN
    if (id == BUTTON_UP || id == BUTTON_DOWN) {
        /* UP/DOWN: phản hồi ngay lúc nhấn, không phân biệt nhấn giữ */
				// khi vua nhan xuong, he thong dua ngay sk EV_BTN_.. vao qEvents qua app_post_events voi timeout 20ms
        if (pressed) {
            (void)app_post_event(id == BUTTON_UP ? EV_BTN_UP : EV_BTN_DOWN, 0, 0, pdMS_TO_TICKS(20));
        }
				// khi nguoi dung bo nut ra thi ko lam gi ca, return luon
        return;
    }
		// Xu ly nhom nut NEXT va OK
		// Khi vua de nut xuong, ham ghi nhan moc thoi gian he thong hien tai va luu con press_tick
		// sau do return ngay va chua chuyen sk ra qEvents, vi chua biet la nguoi dung nhan lau hay nhanh
    if (pressed) {
        *press_tick = xTaskGetTickCount();       /* ghi lại lúc nhấn */
        return;
    }
    /* Nhả nút: phân loại nhấn ngắn / nhấn giữ theo thời gian giữ */
		// thoi gian giu held, lay hien tai tru di trc do
    TickType_t held = xTaskGetTickCount() - *press_tick;
		// Neu thoi gian giu >= nguong =>hold=true
    bool hold = held >= pdMS_TO_TICKS(BUTTON_HOLD_MS);
    uint8_t type;
		// Neu la nut nhan NEXT, kem hold la true thi chuyen sk tuong ung ra qEvents
    if (id == BUTTON_NEXT) { type = hold ? EV_BTN_NEXT_HOLD : EV_BTN_NEXT; }
		// Tuong tu neu nut nhan la OK
    else                   { type = hold ? EV_BTN_OK_HOLD : EV_BTN_OK; }
		// Day su kien hoan chinh vua xac dinh duoc vao qEvents de LampTask xu ly giao dien menu
    (void)app_post_event(type, 0, 0, pdMS_TO_TICKS(20));
}
// Input Task
// Noi toan bo tin hieu phan cung tho tu ngat (cambien va nutnhan) duoc thu va loc nhieu qua softwareTimer va chuyen hoa thanh sk sach de vao qEvents
static void input_task(void *arg)
{
    (void)arg;
    bool       dark = false;  // cam bien dang sang hay dang toi, mac dinh la toi
    TickType_t dark_since = 0;  // moc thoi gian bat dau toi
    uint16_t   dark_count = 0;  // so lan chuyen sang toi
    bool       pressed[SRC_COUNT] = { false }; // mang theo doi cac nut co dang bi bam hay khong
    TickType_t press_tick[SRC_COUNT] = { 0 }; // thoi dien bat dau nhan la khi nao de tinh thoi gian nhan giu

		// Dung cho cho den khi toan bo he thong san sang
    app_wait_system_ready();
		// Ktao cam bien anh sang va gan ham ngat suon xung on_sensor_edge_isr
    light_sensor_init(on_sensor_edge_isr);
		// Ktao cac nut bam va gan ham ngat suon xung on_button_edge_isr
    button_init(on_button_edge_isr);

    /* Đồng bộ trạng thái ban đầu của cảm biến cho LampTask */
    dark = light_sensor_is_dark();
		
		// neu vua cam dien ma troi toi
    if (dark) { 
				dark_count = 1;  // gan so lan dau tien troi toi bat dau tu 1
				dark_since = xTaskGetTickCount();  // tra ve so tick cua he thong tinh tu luc Scheduler bat dau chay, muc dich la de tinh thoi gian toi trc khi sang sang
		}
		// Ngay khi khoi dong, in log hien thi trang thai luon
    log_printf("[IN] Troi %s", dark ? "TOI" : "SANG");
		// Ngay khi khoi dong, ban thang su kien tuong ung EV_SENSOR_.. vao queue voi thoi gian cho vo han, dam bao may tt cua lamptask nam bat chinh xac ngay tu khi cap nguon
    (void)app_post_event(dark ? EV_SENSOR_DARK : EV_SENSOR_LIGHT, 0, 0, portMAX_DELAY);

		// vòng lap chinh cua Task
    for (;;) {
        uint32_t bits = 0;
				// task ngu cho den khi nhan duoc notification duoc gui tu cac ISR hoac tu Software timer chong doi
				// khi thuc day, toan bo gia tri bitmask duoc luu vao bien bits, dong thoi thanh ghi thong bao cua task duoc xoa sach
        (void)xTaskNotifyWait(0, 0xFFFFFFFFUL, &bits, portMAX_DELAY);
				// quet xu ly tung kenh dau vao de kiem tra xem kenh nao vua co co su kien
        for (uint32_t src = 0; src < SRC_COUNT; src++) {
            if ((bits & NB_EDGE(src)) != 0U) {
                /* Có cạnh mới: đếm lại thời gian chống nhiễu từ đầu */
                (void)xTimerReset(s_debounce[src], pdMS_TO_TICKS(10));
            }
						//khi tin hieu da thuc su on dinh, ham callback cua software timer kich hoat va gui bit NB_STABLE vao task
            if ((bits & NB_STABLE(src)) != 0U) {
								// neu la cam bien, goi ham handle_sensor
                if (src == SRC_SENSOR) {
                    handle_sensor(&dark, &dark_since, &dark_count);
                } 
								// nguoc lai neu la nut bam, goi ham handle_button 
								else {
                    handle_button(src, &pressed[src], &press_tick[src]);
                }
            }
        }
    }
}

// Khoi tao toan bo he thong Software Timer dung de chong doi cho cam bien va cac nut bam vat ly
// Tao InputTask truoc khi Scheduler chay
void task_input_create(void)
{
		// Khoi tao Software Timer chong doi cho cam bien anh sang, Che do One_short Timer
    s_debounce[SRC_SENSOR] = xTimerCreate("dbSens", pdMS_TO_TICKS(DEBOUNCE_SENSOR_MS), pdFALSE,
                                          (void *)(uintptr_t)SRC_SENSOR, on_debounce_expired);
		// Kiem tra an toan, Neu Heap cua FreeRTOS ko du dung luong de cap phat timer, he thong se dung lai de tranh loi con tro NULL
    configASSERT(s_debounce[SRC_SENSOR]);
		// Khoi tao mang Software Timer cho cac nut bam vat ly
    for (uint32_t src = SRC_BUTTON0; src < SRC_COUNT; src++) {
        s_debounce[src] = xTimerCreate("dbBtn", pdMS_TO_TICKS(DEBOUNCE_BUTTON_MS), pdFALSE,
                                       (void *)(uintptr_t)src, on_debounce_expired);
        configASSERT(s_debounce[src]);
    }
		// khoi tao InputTask
    BaseType_t ok = xTaskCreate(input_task, "Input", STACK_INPUT, NULL, PRIO_INPUT, &s_task);
		// dam bao task duoc tao thanh cong
    configASSERT(ok == pdPASS);
}
