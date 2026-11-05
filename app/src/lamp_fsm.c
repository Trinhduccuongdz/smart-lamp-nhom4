/**
 * @file    lamp_fsm.c
 * @brief   [APPLICATION] Cài đặt máy trạng thái đèn: OFF -> FADE_IN -> ON -> FADE_OUT -> OFF.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#include "lamp_fsm.h"

void lamp_settings_default(lamp_settings_t *s)
{
    s->mode = LAMP_MODE_FULL;
    s->max_level = LAMP_LEVEL_MAX;
    s->off_timer_min = 0;
    s->auto_on = 1;
    s->icon = 0;
}

/* Cảnh của từng icon: {mức sáng, hiệu ứng, hẹn giờ (phút)}. Thứ tự theo kho icon (icons.c). */
static const uint8_t k_scene[LAMP_ICON_COUNT][3] = {
    {16, LAMP_MODE_FULL,    0},   /* Mat troi   */
    { 2, LAMP_MODE_BREATH, 30},   /* Mat trang  */
    { 4, LAMP_MODE_WAVE,   15},   /* Ngoi sao   */
    { 8, LAMP_MODE_BREATH,  0},   /* Trai tim   */
    {12, LAMP_MODE_FULL,    0},   /* Bong den   */
    {10, LAMP_MODE_FULL,    0},   /* Ngoi nha   */
    { 2, LAMP_MODE_FULL,   15},   /* Giuong ngu */
    {16, LAMP_MODE_FULL,   60},   /* Quyen sach */
    {10, LAMP_MODE_BREATH, 30},   /* Ca phe     */
    {12, LAMP_MODE_WAVE,    0},   /* Am nhac    */
    { 8, LAMP_MODE_FULL,    1},   /* Dong ho: hẹn 1 phút (dùng khi demo) */
    {10, LAMP_MODE_WAVE,    0},   /* Cay thong  */
    { 6, LAMP_MODE_BREATH,  0},   /* May        */
    { 6, LAMP_MODE_WAVE,    0},   /* Mua        */
    {14, LAMP_MODE_BREATH,  0},   /* Tuyet      */
    {12, LAMP_MODE_WAVE,    0},   /* Mat cuoi   */
};

// ap dung canh cua icon
void lamp_apply_icon_scene(lamp_settings_t *s)
{
    if (s->icon >= LAMP_ICON_COUNT) { s->icon = 0U; }
    s->max_level     = k_scene[s->icon][0];
    s->mode          = k_scene[s->icon][1];
    s->off_timer_min = k_scene[s->icon][2];
    s->auto_on       = 0U;                      /* cảnh chỉ có ý nghĩa ở MANUAL */
}

// dam bao cai dat luon hop le truoc khi phan khac dung den
void lamp_settings_sanitize(lamp_settings_t *s)
{
    if (s->mode >= LAMP_MODE_COUNT)          { s->mode = LAMP_MODE_FULL; }
    if (s->max_level < 1U)                   { s->max_level = 1U; }
    if (s->max_level > LAMP_LEVEL_MAX)       { s->max_level = LAMP_LEVEL_MAX; }
    if (s->off_timer_min > LAMP_OFF_TIMER_MAX) { s->off_timer_min = LAMP_OFF_TIMER_MAX; }
    s->auto_on = (s->auto_on != 0U) ? 1U : 0U;
    if (s->icon >= LAMP_ICON_COUNT)          { s->icon = 0U; }
}
// ktao may trang thai, duoc goi 1 lan trong task_lamp_create, truoc khi lap lich chay
void lamp_fsm_init(lamp_fsm_t *f)
{
    f->state = LAMP_OFF;
    f->level = 0;
    f->dark = false;
}

/* Chuyển sang sáng dần (nếu đang tắt hoặc đang tối dần) */
static uint8_t go_up(lamp_fsm_t *f)
{		
		// dang toi hoac dang tat dan thi sang sang dan
    if (f->state == LAMP_OFF || f->state == LAMP_FADE_OUT) {
        f->state = LAMP_FADE_IN;
        return LAMP_ACT_FADE_START;
    }
		// cac truong hop con lai nhu ON hoac fade_in thi ko doi
    return LAMP_ACT_NONE;
}

/* Chuyển sang tối dần (nếu đang sáng hoặc đang sáng dần) */
// ham chuyen doi trang thai trong FSM lamp
static uint8_t go_down(lamp_fsm_t *f)
{
    if (f->state == LAMP_ON || f->state == LAMP_FADE_IN){
        f->state = LAMP_FADE_OUT;
        return LAMP_ACT_FADE_START | LAMP_ACT_OFF_TIMER_STOP;
    }
    return LAMP_ACT_NONE;
}

/* Vào ON. Hẹn giờ đếm ngược chỉ áp dụng ở MANUAL; ở AUTO đèn tắt theo cảm biến. */
static uint8_t enter_on(lamp_fsm_t *f, const lamp_settings_t *s)
{
    f->state = LAMP_ON;
    f->level = s->max_level;
		// neu o che do manual va co hen gio 
    if (s->auto_on == 0U && s->off_timer_min > 0U) {
				// dung timer fade va bat dau dem nguoc
        return LAMP_ACT_FADE_STOP | LAMP_ACT_OFF_TIMER_START;
    }
		// con lai nau la auto hoac la manual ko hen gio, dung timer fade va ko dem nguoc
    return LAMP_ACT_FADE_STOP | LAMP_ACT_OFF_TIMER_STOP;
}

// f:tt hien tai cua den (state,lever,dark), in:su kien vua den, s:cai dat,max lever, auto_on va hen gio
// dau ra cua ham la 1 byte bitmask cho biet lamptask can lam gi tiep
uint8_t lamp_fsm_handle(lamp_fsm_t *f, lamp_input_t in, const lamp_settings_t *s)
{
    uint8_t act = LAMP_ACT_NONE;

    switch (in) {
		// Troi toi
    case LAMP_IN_DARK:
        f->dark = true; // ghi dark=true, ke ca co dang o manual, de sau neu bat auto thi tu biet troi dang toi de bat den tu dong
				// chi khi neu o che do auto thi moi goi go_up de sang dan, con o Manual thi cam bien khong dieu khien den
        if (s->auto_on != 0U) { act = go_up(f); }
        break;
		// Troi sang
    case LAMP_IN_LIGHT:
        f->dark = false; // ghi dark=false
        if (s->auto_on != 0U) { act = go_down(f); }
        break;
		// Lenh ON hoac vua chon ICON
    case LAMP_IN_ON_REQ:
				// goi go_up bat ke khi o auto hay manual
        act = go_up(f);
        break;
		// Lenh OFF 
    case LAMP_IN_OFF_REQ:
        act = go_down(f);
        break;
		// Nut OK khi Menu dong hoac lenh Toggle
    case LAMP_IN_TOGGLE:
				// Neu ma den tat hoac la den dang toi dan thi bat den, khong thi tat den
        act = (f->state == LAMP_OFF || f->state == LAMP_FADE_OUT) ? go_up(f) : go_down(f);
        break;
		// Nhip 100 ms cua tmrFade, lever thuc su thay doi
    case LAMP_IN_FADE_TICK:
				// neu dang tang
        if (f->state == LAMP_FADE_IN) {
						// neu lever van chua dat max thi cu tang lever ++
            if (f->level < s->max_level) { f->level++; }
						// neu lever dat max roi vao enter_on va dung timer
            if (f->level >= s->max_level) { act = enter_on(f, s); }
        } 
				// neu dang toi dan
				else if (f->state == LAMP_FADE_OUT) {
						// Chung nao va lever >0 thi --
            if (f->level > 0U) { f->level--; }
						// neu da ve 0, thi state->OFF
            if (f->level == 0U) {
                f->state = LAMP_OFF;
								// hanh dong la FADE_STOP
                act = LAMP_ACT_FADE_STOP;
            }
        } 
				else {
            act = LAMP_ACT_FADE_STOP;   /* nhịp thừa (đã ở ON/OFF): dừng timer */
        }
        break;
		// dem nguoc hen gio ve 0, goi go_down de toi dan roi tat
    case LAMP_IN_OFF_TIMEOUT:
        act = go_down(f);
        break;
		// cai dat vua doi (luu menu, up/down chinh do sang, bright, auto,...
    case LAMP_IN_SETTINGS:
				// neu dang on 
        if (f->state == LAMP_ON) {
						// so sanh lever hien tai voi max moi
            if (f->level < s->max_level) {
                f->state = LAMP_FADE_IN;                 /* max tăng: sáng dần lên mức mới */
                act = LAMP_ACT_FADE_START;               /* đếm ngược (nếu có) vẫn chạy tiếp */
            } 
						else {
                act = enter_on(f, s);                    /* max giảm/giữ: kẹp level, cập nhật hẹn giờ */
            }
        } 
				// dang o manual ->auto, tai thoi diem do den dang tat va troi toi
				else if (f->state == LAMP_OFF && s->auto_on != 0U && f->dark) {
            act = go_up(f);                              /* vừa bật auto khi trời đang tối */
        }
        break;
		// su kien la thi bo qua va tra 0
    default:
        break;
    }
    return act;
}
// chuyen tt cua den sang -> chuoi ky tu van ban
const char *lamp_state_name(lamp_state_t st)
{
    switch (st) {
    case LAMP_OFF:      return "OFF";
    case LAMP_FADE_IN:  return "FADE_IN";
    case LAMP_ON:       return "ON";
    case LAMP_FADE_OUT: return "FADE_OUT";
    default:            return "?";
    }
}
// chuyen tt cua den dang so nguyen -> ky tu van ban tuong ung de in ra log hoac hien thi oled
const char *lamp_mode_name(uint8_t mode)
{
    switch (mode) {
    case LAMP_MODE_FULL:   return "FULL";
    case LAMP_MODE_BREATH: return "BREATH";
    case LAMP_MODE_WAVE:   return "WAVE";
    default:               return "?";
    }
}
