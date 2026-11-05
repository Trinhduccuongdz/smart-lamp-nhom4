/**
 * @file    button.c
 * @brief   [BSP] Nút nhấn dựa trên driver gpio + exti.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Bốn nút cùng nằm ở PB12..PB15 nên chung một vector EXTI15_10.
 */
#include "button.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_exti.h"
#include <stddef.h>
// dong goi chan vat ly cua nut bam (port stm32+pin)
typedef struct { GPIO_TypeDef *port; uint8_t pin; } button_pin_t;

// static: gioi han pham vi cua mang k_pins chi hien thi ben trong tep ma nguon hien tai, tranh xung dot ten bien voi cac tep khac
// const: dat mang nay vao vung nho flash (READ ONLY), toi uu hoa bo nho RAM quy gia cua VDK
static const button_pin_t k_pins[BUTTON_COUNT] = {
    [BUTTON_NEXT] = { BOARD_BTN_NEXT_PORT, BOARD_BTN_NEXT_PIN },
    [BUTTON_OK]   = { BOARD_BTN_OK_PORT,   BOARD_BTN_OK_PIN   },
    [BUTTON_UP]   = { BOARD_BTN_UP_PORT,   BOARD_BTN_UP_PIN   },
    [BUTTON_DOWN] = { BOARD_BTN_DOWN_PORT, BOARD_BTN_DOWN_PIN },
};
// bien con tro ham callback
static button_edge_cb_t s_cb;

// unit8_t line : dai dien cho so hieu duong ngat ngoai, hoac so chan GPIO vua kich hoat ngat
static void on_exti(uint8_t line)
{
    uint8_t i;
		// kiem tra an toan, dam bao he thong da duoc khoi tao va dky ham callback hop le trc do, neu chua thi return de tranh nhay vao con tro rac khien treo CPU
    if (s_cb == NULL) { return; }
		// vong lap quet tim chan ngat, khi ngat xay ra tangdriver goc chi biet co chan nao do trong nhom ngat thay doi, chua biet cu the
    for (i = 0U; i < (uint8_t)BUTTON_COUNT; i++) {
				// neu tim duoc thay chan ngat, goi nguoc lai ham callback s_cb
        if (k_pins[i].pin == line) { s_cb((button_id_t)i); return; }
    }
}
// ham nay chay 1 lan khi he thong khoi dong de thiet lap chan vi dieu khien ket noi voi cac nut bam vat ly
void button_init(button_edge_cb_t cb)
{
    uint8_t i;
    s_cb = cb;
		// quet qua tat ca nut bam
    for (i = 0U; i < (uint8_t)BUTTON_COUNT; i++) {
				// cau hinh dien tro keo len, y nghia la khi nut bam chua duoc nhan, chan GPIO se duoc keo len muc dien ap cao
        gpio_config_input_pull(k_pins[i].port, k_pins[i].pin, GPIO_PULL_UP);
				// cau hinh ngat ngoai. EXTI_EDGE_BOTH la cai dat bat ngat o ca 2 suon xung
			  // on_exti: dang ky ham phuc vu ngat cua tang phan cung se don nhan tin hieu EXTI dau tien
				// BOARD_IRQ_PRIO_EXTI: thiet lap muc uu tien ngat cho chan EXTI
        exti_config(k_pins[i].port, k_pins[i].pin, EXTI_EDGE_BOTH, BOARD_IRQ_PRIO_EXTI, on_exti);
    }
}
// kiem tra trang thai nut nhan dang nhan hay dang nha
bool button_is_pressed(button_id_t id)
{
		// kiem tra bien de dam bao chi so id truyen vao khong vuot qua gioi han cho phep
    if ((unsigned)id >= (unsigned)BUTTON_COUNT) { return false; }
		// doc muc dien ap thuc te tren chan GPIO tuong ung voi id
    return !gpio_read(k_pins[id].port, k_pins[id].pin);
}
