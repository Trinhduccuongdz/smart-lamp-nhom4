/**
 * @file    menu.c
 * @brief   [APPLICATION] Cài đặt menu 4 nút (UP, DOWN, NEXT, OK).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#include "menu.h"
#include "lamp_fsm.h"

#define MINUTES_PER_DAY  1440U

static const uint8_t k_timer_steps[] = { 0, 1, 2, 5, 10, 15, 30, 60 };

void menu_init(menu_t *m)
{
    m->state = MENU_CLOSED;
    m->item = MENU_ITEM_AUTO;
    m->field = 0;
    m->clock_min = 0;
    m->now_min = 0;
    lamp_settings_default(&m->edit);
}

/* Cộng dir (+1/-1) vào v, quay vòng trong [lo, hi] */
// v: gia tri hien tai, dir: chieu tang hoac giam (+-1), lo,hi la gioi han duoi la tren cua gia tri
static uint8_t wrap_step(uint8_t v, int dir, uint8_t lo, uint8_t hi)
{
    if (dir > 0) { return (v >= hi) ? lo : (uint8_t)(v + 1U); }
    return (v <= lo) ? hi : (uint8_t)(v - 1U);
}
// tang hoac giam gio hay phut khi dat gio
static void step_clock(menu_t *m, int dir)
{
    unsigned h = m->clock_min / 60U, mi = m->clock_min % 60U;
    if (m->field == 0U) { h = wrap_step((uint8_t)h, dir, 0U, 23U); }
    else                { mi = wrap_step((uint8_t)mi, dir, 0U, 59U); }
    m->clock_min = (uint16_t)(h * 60U + mi);
}

// doi gia tri cua muc dang sua khi bam up/down
// goi khi luc menu dang o trang thai edit, nguoi dung nhan up down
// Ham nay chi sua ban sao, ko sua cai dat that, den ko thay doi gi cho den khi nguoi dung an ok
static void step_value(menu_t *m, int dir)
{
		// con tro tro toi ban sao
    lamp_settings_t *e = &m->edit; // m->edit la ban sao cai dat, &m->edit la lay dia chi cua ban sao 
    switch (m->item) {
    case MENU_ITEM_AUTO:   e->auto_on = (e->auto_on != 0U) ? 0U : 1U; break; // dao, ko quan tam chieu dir
    case MENU_ITEM_MODE:   e->mode = wrap_step(e->mode, dir, 0U, LAMP_MODE_COUNT - 1U); break; // quay vong Full->Breath->wave
    case MENU_ITEM_BRIGHT: e->max_level = wrap_step(e->max_level, dir, 1U, LAMP_LEVEL_MAX); break; // quay vong, bat dau tu 1
    case MENU_ITEM_ICON:   e->icon = wrap_step(e->icon, dir, 0U, LAMP_ICON_COUNT - 1U); break; // quay vong 16 icon
    case MENU_ITEM_TIMER:  e->off_timer_min = wrap_step(e->off_timer_min, dir, 0U, LAMP_OFF_TIMER_MAX); break; 
    case MENU_ITEM_CLOCK:  step_clock(m, dir); break;  // doi o gio hoac phut
    default: break;
    }
}

/* NEXT trong EDIT: nhảy nhanh (hẹn giờ theo bậc 0,1,2,5,...; đặt giờ: đổi ô giờ/phút) */
// Xu ly nut NEXT khi dang EDIT
static void next_value(menu_t *m)
{
		// Cung chi sua ban sao
    lamp_settings_t *e = &m->edit;
	// Muc HEN GIO:nhay theo bac: Hen gio co 61 gia tri (0-60), muon dat 30p thi neu an up-down phai bam 30 lan
    if (m->item == MENU_ITEM_TIMER) {
        uint8_t next = 0; 
        for (size_t i = 0; i < sizeof(k_timer_steps); i++) {
						// gap bac dau tien lon hon gia tri hien tai thi lay bac do
            if (k_timer_steps[i] > e->off_timer_min) { next = k_timer_steps[i]; break; }
        }
        e->off_timer_min = next; // gan lai
    } 
		else if (m->item == MENU_ITEM_CLOCK) { // Muc dat gio, doi o gio <-> o phut
        m->field ^= 1U;
    } 
		else { // cac muc con lai khong co cach nhay rieng nen next lam giong UP
        step_value(m, +1);
    }
}

uint8_t menu_handle(menu_t *m, menu_key_t key, lamp_settings_t *settings)
{
    switch (m->state) {
    case MENU_CLOSED:
        if (key == MENU_KEY_OK)   { return MENU_RES_TOGGLE_LAMP; }
        if (key == MENU_KEY_NEXT) {
            m->state = MENU_NAV;
            m->item = MENU_ITEM_AUTO;
            return MENU_RES_REDRAW;
        }
        if (key == MENU_KEY_UP || key == MENU_KEY_DOWN) {   /* chỉnh nhanh mức sáng */
            uint8_t lv = settings->max_level;
            if (key == MENU_KEY_UP && lv < LAMP_LEVEL_MAX) { lv++; }
            if (key == MENU_KEY_DOWN && lv > 1U)           { lv--; }
            if (lv == settings->max_level) { return MENU_RES_NONE; }
            settings->max_level = lv;
            return MENU_RES_SETTINGS | MENU_RES_REDRAW;
        }
        return MENU_RES_NONE;

    case MENU_NAV:
        if (key == MENU_KEY_NEXT || key == MENU_KEY_DOWN) {
            m->item = (uint8_t)((m->item + 1U) % MENU_ITEM_COUNT);
        } else if (key == MENU_KEY_UP) {
            m->item = (uint8_t)((m->item + MENU_ITEM_COUNT - 1U) % MENU_ITEM_COUNT);
        } else if (key == MENU_KEY_OK) {
            if (m->item == MENU_ITEM_EXIT) {
                m->state = MENU_CLOSED;
            } else {
                m->edit = *settings;
                m->clock_min = m->now_min;
                m->field = 0;
                m->state = MENU_EDIT;
            }
        } else {
            m->state = MENU_CLOSED;          /* nhấn giữ: đóng menu */
        }
        return MENU_RES_REDRAW;

    case MENU_EDIT:
        if (key == MENU_KEY_UP || key == MENU_KEY_DOWN) {
            step_value(m, key == MENU_KEY_UP ? +1 : -1);
            return MENU_RES_REDRAW;
        }
        if (key == MENU_KEY_NEXT) {
            next_value(m);
            return MENU_RES_REDRAW;
        }
        m->state = MENU_NAV;
        if (key == MENU_KEY_OK) {
            if (m->item == MENU_ITEM_CLOCK) {
                return MENU_RES_SET_CLOCK | MENU_RES_REDRAW;
            }
            *settings = m->edit;             /* lưu */
            lamp_settings_sanitize(settings);
            if (m->item == MENU_ITEM_ICON) {
                lamp_apply_icon_scene(settings);
                return MENU_RES_SETTINGS | MENU_RES_SCENE | MENU_RES_REDRAW;
            }
            return MENU_RES_SETTINGS | MENU_RES_REDRAW;
        }
        return MENU_RES_REDRAW;              /* nhấn giữ: hủy */

    default:
        m->state = MENU_CLOSED;
        return MENU_RES_REDRAW;
    }
}

const char *menu_item_name(uint8_t item)
{
    switch (item) {
    case MENU_ITEM_AUTO:   return "Che do";
    case MENU_ITEM_MODE:   return "Hieu ung";
    case MENU_ITEM_BRIGHT: return "Do sang";
    case MENU_ITEM_ICON:   return "Icon";
    case MENU_ITEM_TIMER:  return "Hen gio";
    case MENU_ITEM_CLOCK:  return "Dat gio";
    case MENU_ITEM_EXIT:   return "Thoat";
    default:               return "?";
    }
}

// doi so nguyen thanh chuoi chu so
// VD: 7 ->07, 12->12
static size_t put_uint(char *out, size_t size, unsigned v, unsigned width)
{
    char tmp[6];
    size_t n = 0, k = 0;
		// tach so tu so don vi 
    do { 
			tmp[n++] = (char)('0' + v % 10U); 
			v /= 10U; 
		} while (v != 0U && n < sizeof(tmp));
		// neu ma so chua du do rong thi them 0 vao cuoi
    while (n < width && n < sizeof(tmp)) { tmp[n++] = '0'; }
		// sau do dao nguoc, chep vao out
    while (n > 0U && k + 1U < size) { out[k++] = tmp[--n]; }
    out[k] = '\0'; // ket thuc chuoi
    return k;
}
// Chep chuoi an toan
static size_t copy_str(char *out, size_t size, const char *s)
{
    size_t k = 0;
    while (s[k] != '\0' && k + 1U < size) { out[k] = s[k]; k++; }
    out[k] = '\0';
    return k;
}
// Tao chu hien thi gia tri cua 1 muc menu
void menu_item_value(const menu_t *m, uint8_t item, const lamp_settings_t *s, char *out, size_t size)
{
    size_t k;
    if (size == 0U) { return; }
    out[0] = '\0';
    switch (item) {
    case MENU_ITEM_AUTO:
        (void)copy_str(out, size, (s->auto_on != 0U) ? "AUTO" : "MANUAL");
        break;
    case MENU_ITEM_MODE:
        (void)copy_str(out, size, lamp_mode_name(s->mode));
        break;
    case MENU_ITEM_BRIGHT:
        (void)put_uint(out, size, s->max_level, 1U);
        break;
    case MENU_ITEM_ICON:
        k = put_uint(out, size, (unsigned)s->icon + 1U, 1U);
        k += copy_str(out + k, size - k, "/");
        (void)put_uint(out + k, size - k, LAMP_ICON_COUNT, 1U);
        break;
    case MENU_ITEM_TIMER:
        if (s->off_timer_min == 0U) {
            (void)copy_str(out, size, "KHONG");
        } else {
            k = put_uint(out, size, s->off_timer_min, 1U);
            (void)copy_str(out + k, size - k, " ph");
        }
        break;
    case MENU_ITEM_CLOCK: {
        unsigned v = (m != NULL) ? ((m->state == MENU_EDIT && m->item == MENU_ITEM_CLOCK) ? m->clock_min : m->now_min) : 0U;
        k = put_uint(out, size, v / 60U, 2U);
        k += copy_str(out + k, size - k, ":");
        (void)put_uint(out + k, size - k, v % 60U, 2U);
        break;
    }
    default:
        break;
    }
}
