#include "rtc_clock.h"

// Ktao dich vu thoi gian va chon nguon xung
// irq_prio: muc uu tien ngat cho bo dinh thoi RTC
// on_second: con tro ham callback thong bao nhip giay (ham RTC_IRQHandler
const char *rtc_clock_init(uint8_t irq_prio, rtc_second_cb_t on_second)
{
    return (rtc_hw_init(irq_prio, on_second) == RTC_SRC_LSE) ? "LSE" : "LSI";
}
// lay thoi gian hien tai trong ngay
uint32_t rtc_clock_now(void)
{
		// tra ve so giay da troi qua ke tu dau ngay
    return rtc_hw_get_counter() % RTC_SECONDS_PER_DAY;
}

// cai dat lai thoi gian trong ngay
void rtc_clock_set(uint32_t sec_of_day)
{	
		// cap nhat hoac dong ho hoa lai thoi gian he thong
    rtc_hw_set_counter(sec_of_day % RTC_SECONDS_PER_DAY);
}
