#include "hal_rtc.h"
#include "stm32f10x.h"
#include <stddef.h>
// thanh ghi BKP duoc nuoi bang nguon pin phu VBAT, giu nguyen gia tri khi chip o che do sleep, ko bi reset boi phan mem thong thuong
// khi thiet bi khoi dong, firmware se doc gia tri tu thanh ghi BKP, neu khop thi ko phai cau hinh lai, nguoc lai thi se phai cau hinh toan bo RTC
#define RTC_BKP_MAGIC     0xA5A5U
// nguong thoi gian cho toi da dung cho vong lap kiem tra su on dinh cua thach anh ngoai 32.768 kHZ
#define LSE_WAIT_LOOPS    8000000UL      /* ~1-2 s ở 72 MHz: thạch anh 32.768 kHz khởi động chậm */

// Bien tinh luu tru con tro ham cho su kien ngat moi giay 1 lan 
/* Khi RTC dem du 1s, phan cung sinh ngat, trinh phuc vu ngat phan cung thong qua con tro ham s_cb nay de ban tin hieu 
thong bao nguoc len tang cao*/
static rtc_second_cb_t s_cb;

static void rtc_wait_ready(void)
{
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();
}

rtc_source_t rtc_hw_init(uint8_t irq_prio, rtc_second_cb_t cb)
{
    NVIC_InitTypeDef nv;
    rtc_source_t src;
    uint32_t n;

    s_cb = cb;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);              /* cho phép ghi miền Backup (RTC, BKP) */

    if (BKP_ReadBackupRegister(BKP_DR1) == RTC_BKP_MAGIC) {
        /* RTC đã chạy từ trước (reset mềm): giữ nguyên bộ đếm. */
        src = ((RCC->BDCR & RCC_BDCR_RTCSEL) == RCC_BDCR_RTCSEL_LSI) ? RTC_SRC_LSI : RTC_SRC_LSE;
        if (src == RTC_SRC_LSI) {             /* LSI nằm ở miền chính, tắt sau reset -> bật lại */
            RCC_LSICmd(ENABLE);
            while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) { }
        }
        rtc_wait_ready();
    } else {
        BKP_DeInit();                         /* reset miền Backup để chọn lại nguồn xung */
        RCC_LSEConfig(RCC_LSE_ON);
        for (n = 0U; n < LSE_WAIT_LOOPS; n++) {
            if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == SET) { break; }
        }
        if (n < LSE_WAIT_LOOPS) {
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
            src = RTC_SRC_LSE;
        } else {
            RCC_LSEConfig(RCC_LSE_OFF);
            RCC_LSICmd(ENABLE);
            while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) { }
            RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
            src = RTC_SRC_LSI;
        }
        RCC_RTCCLKCmd(ENABLE);
        rtc_wait_ready();
        RTC_SetPrescaler(src == RTC_SRC_LSE ? 32767U : 39999U);   /* 1 Hz */
        RTC_WaitForLastTask();
        RTC_SetCounter(0U);
        RTC_WaitForLastTask();
        BKP_WriteBackupRegister(BKP_DR1, RTC_BKP_MAGIC);
    }

    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    RTC_WaitForLastTask();

    nv.NVIC_IRQChannel = RTC_IRQn;
    nv.NVIC_IRQChannelPreemptionPriority = irq_prio;
    nv.NVIC_IRQChannelSubPriority = 0;
    nv.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nv);
    return src;
}

uint32_t rtc_hw_get_counter(void)
{
    return RTC_GetCounter();
}

void rtc_hw_set_counter(uint32_t value)
{
    RTC_WaitForLastTask();
    RTC_SetCounter(value);
    RTC_WaitForLastTask();
}

// ISR cua ngoai vi RTC
void RTC_IRQHandler(void)
{		
		// Goi ham RTC_Get.. de doc thanh ghi trang thai ngat, kiem tra xem ngat day RTC_IT_SEC co thuc su dang duoc kich hoat hay khong
    if (RTC_GetITStatus(RTC_IT_SEC) != RESET) {
				// sau khi da nhan biet la co ngat, bat buoc phai xoa co ngat, neu ko thi CPU vua thoat ngat no se lai quay lai ngat
        RTC_ClearITPendingBit(RTC_IT_SEC);
				// kiem tra xem con tro ham Callback co hop le hay khong
				// s_cb() chinh la ham on_rtc_second, ham nay co nhiem vu day su kien nhip day vao queue
        if (s_cb != NULL) { s_cb(); }
    }
}
