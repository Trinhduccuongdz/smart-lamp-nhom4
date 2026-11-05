/**
 * @file    menu.h
 * @brief   [APPLICATION] Menu cài đặt trên OLED điều khiển bằng 4 nút (máy trạng thái thuần logic).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 *  Trạng thái   UP / DOWN                 NEXT (ngắn)            OK (ngắn)                  Nhấn giữ (>= 0.8 s)
 *  ----------   -----------------------   --------------------   ------------------------   -------------------
 *  CLOSED       tăng/giảm mức sáng        mở menu (NAV)          bật/tắt đèn                -
 *  NAV          lên/xuống mục             sang mục kế tiếp       vào sửa mục (EDIT)/Thoát   đóng menu
 *  EDIT         tăng/giảm giá trị         đổi giá trị / ô giờ    lưu, quay lại NAV          hủy, quay lại NAV
 *
 *  Mục "Icon" ở trạng thái EDIT chính là giao diện chọn icon; lưu icon = áp dụng cảnh của icon
 *  (mức sáng, hiệu ứng, hẹn giờ) và bật đèn.
 *  Mục "Dat gio" sửa giờ RTC: NEXT chuyển giữa ô giờ và ô phút.
 */
#ifndef MENU_H
#define MENU_H

#include <stddef.h>
#include "lamp_types.h"

typedef enum {
    MENU_KEY_NEXT = 0, MENU_KEY_OK, MENU_KEY_NEXT_HOLD, MENU_KEY_OK_HOLD,
    MENU_KEY_UP, MENU_KEY_DOWN
} menu_key_t;
typedef enum { MENU_CLOSED = 0, MENU_NAV, MENU_EDIT } menu_state_t;
typedef enum {
    MENU_ITEM_AUTO = 0,   /* Che do: AUTO / MANUAL */
    MENU_ITEM_MODE,       /* Hieu ung */
    MENU_ITEM_BRIGHT,     /* Do sang mong muon */
    MENU_ITEM_ICON,       /* Icon hien thi */
    MENU_ITEM_TIMER,      /* Hen gio (dem nguoc) */
    MENU_ITEM_CLOCK,      /* Dat gio RTC */
    MENU_ITEM_EXIT,
    MENU_ITEM_COUNT
} menu_item_t;

typedef struct {
    uint8_t         state;      /* menu_state_t */
    uint8_t         item;       /* menu_item_t đang chọn */
    uint8_t         field;      /* mục Dat gio: 0 = giờ, 1 = phút */
    lamp_settings_t edit;       /* bản sao đang sửa (chỉ ghi vào cài đặt thật khi nhấn OK) */
    uint16_t        clock_min;  /* mục Dat gio: phút trong ngày đang sửa (0..1439) */
    uint16_t        now_min;    /* LampTask cập nhật: giờ hiện tại (phút trong ngày) */
} menu_t;

/* Kết quả trả về (bitmask) */
#define MENU_RES_NONE        0x00U
#define MENU_RES_TOGGLE_LAMP 0x01U
#define MENU_RES_SETTINGS    0x02U   /* *settings vừa được ghi giá trị mới */
#define MENU_RES_REDRAW      0x04U
#define MENU_RES_SET_CLOCK   0x08U   /* đặt giờ RTC = m->clock_min (giây = 0) */
#define MENU_RES_SCENE       0x10U   /* vừa chọn icon: đã áp dụng cảnh, nên bật đèn */

void        menu_init(menu_t *m);
uint8_t     menu_handle(menu_t *m, menu_key_t key, lamp_settings_t *settings);
const char *menu_item_name(uint8_t item);
/** Ghi chuỗi giá trị của `item` vào out (ví dụ "MANUAL", "BREATH", "12", "5 ph", "3/16", "07:30"). */
void        menu_item_value(const menu_t *m, uint8_t item, const lamp_settings_t *s, char *out, size_t size);

#endif /* MENU_H */
