#include "hal_exti.h"
#include <stddef.h>

static exti_callback_t s_callbacks[16];
// mapping , con tro cau truc thanh ghi GPIO -> ma nguon cong tuong ung khi cau hinh bo ghep kenh ngoai vi AFIO
static uint8_t port_source(GPIO_TypeDef *port)
{
    if (port == GPIOA) { return GPIO_PortSourceGPIOA; }
    if (port == GPIOB) { return GPIO_PortSourceGPIOB; }
    if (port == GPIOC) { return GPIO_PortSourceGPIOC; }
    return GPIO_PortSourceGPIOD;
}

static uint8_t line_to_irq(uint8_t line)
{
    if (line <= 4U) { return (uint8_t)(EXTI0_IRQn + line); }
    if (line <= 9U) { return (uint8_t)EXTI9_5_IRQn; }
    return (uint8_t)EXTI15_10_IRQn;
}
// cau hinh tong quat cho ngat ngoai 
void exti_config(GPIO_TypeDef *port, uint8_t pin, exti_edge_t edge,
                 uint8_t irq_prio, exti_callback_t cb)
{
		
    s_callbacks[pin] = cb;

    /* Chọn port nối vào line `pin` (AFIO_EXTICRx). Cần đã bật clock AFIO. */
    GPIO_EXTILineConfig(port_source(port), pin);
		
    EXTI_InitTypeDef ex;
    ex.EXTI_Line    = (uint32_t)1U << pin;
    ex.EXTI_Mode    = EXTI_Mode_Interrupt; // dat che do hoat dong la Ngat Phan Cung
    ex.EXTI_Trigger = (edge == EXTI_EDGE_BOTH)   ? EXTI_Trigger_Rising_Falling : // dung toan tu 3 ngoi de cau hinh kieu bat suon xung dua vao tham so truyen vao
                      (edge == EXTI_EDGE_RISING) ? EXTI_Trigger_Rising : EXTI_Trigger_Falling;
    ex.EXTI_LineCmd = ENABLE; 
    EXTI_ClearITPendingBit(ex.EXTI_Line);   /* xóa cờ chờ cũ */
    EXTI_Init(&ex); // Ghi toan bo cau hinh vao cac thanh ghi phan cung cua EXTI

		// Cau hinh bo dieu khien ngat vector long nhau NVIC
    NVIC_InitTypeDef nv;
    nv.NVIC_IRQChannel                   = line_to_irq(pin); // Anh xa so hieu chan pin sang ten vector ngat chuan cua dong chip
    nv.NVIC_IRQChannelPreemptionPriority = irq_prio;   // gan muc uu tien ngat theo gia tri truyen vao
    nv.NVIC_IRQChannelSubPriority        = 0;   // ko co muc uu tien phu
    nv.NVIC_IRQChannelCmd                = ENABLE; // mo khoa duong ngat nay tai tang NVIC
    NVIC_Init(&nv); // luu cau hinh
}

/* Xử lý chung: duyệt các line trong khoảng [first, last] đang có cờ ngắt */
// Trai tim cua co che dieu phoi ngat ngoai EXTI tren stm32f103
/* khi nhieu chan GPIO khac nhau cung bi gom chung vao 1 duong vector ngat phan cung, lam sao de phan mem biet chinh xac chan vao vua tac dong */
static void exti_dispatch(uint8_t first, uint8_t last)
{
		// Vong lap quet so chan trong day chan
    for (uint8_t line = first; line <= last; line++) {
				// dich bit de tao ra bitmask tuong ung voi tung duong EXTI dang xet trong thanh ghi 
        uint32_t mask = (uint32_t)1U << line;
				// doc trang thai thanh ghi co PR cua ngoai vi EXTI, tra ve ket qua !RESET
        if (EXTI_GetITStatus(mask) != RESET) {
            EXTI_ClearITPendingBit(mask);   /* xóa cờ trước khi gọi callback */
            if (s_callbacks[line] != NULL) {
								// dung chi so line lam index de truy xuat vao mang con tro ham quan ly ngat s_callbacks da duoc dang ky ty truoc
                s_callbacks[line](line);
            }
        }
    }
}
// tap hop cac trinh phuc vu ngat ngoai phan cung ISRs
void EXTI0_IRQHandler(void)     { exti_dispatch(0, 0); } // ngat ngoai phuc vu rieng cho chan so 0 cua bat ky cong GPIO nao
void EXTI1_IRQHandler(void)     { exti_dispatch(1, 1); } // tuong tu o tren
void EXTI2_IRQHandler(void)     { exti_dispatch(2, 2); }
void EXTI3_IRQHandler(void)     { exti_dispatch(3, 3); }
void EXTI4_IRQHandler(void)     { exti_dispatch(4, 4); }
void EXTI9_5_IRQHandler(void)   { exti_dispatch(5, 9); } // ngat chung cho tat ca cac chan tu so 5->9
void EXTI15_10_IRQHandler(void) { exti_dispatch(10, 15); } // ngat chung cho tat ca cac chan tu so 10-15
