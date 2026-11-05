/**
 * @file    hal_timer.c
 * @brief   [HAL] TIM2 đếm tự do + ngắt tràn dựa trên SPL (slide Timer: PSC, ARR, CNT, UIF;
 *          slide Interrupt: DIER.UIE).
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 */
#include "hal_timer.h"
#include "hal_clock.h"
#include "stm32f10x.h"
#include <stddef.h>

static timer_overflow_cb_t s_ovf_cb;

void tim2_freerun_init(uint32_t count_hz, uint8_t irq_prio, timer_overflow_cb_t cb)
{
    s_ovf_cb = cb;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tb;
    tb.TIM_Prescaler         = (uint16_t)(CLOCK_TIMCLK1_HZ / count_hz - 1U);  /* 72 MHz / 20 kHz - 1 = 3599 */
    tb.TIM_Period            = 0xFFFF;                                         /* ARR: đếm hết 16 bit */
    tb.TIM_ClockDivision     = TIM_CKD_DIV1;
    tb.TIM_CounterMode       = TIM_CounterMode_Up;
    tb.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &tb);

    TIM_ClearFlag(TIM2, TIM_FLAG_Update);         /* TimeBaseInit tạo sự kiện UG làm bật UIF */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);    /* UIE = 1 */

    NVIC_InitTypeDef nv;
    nv.NVIC_IRQChannel                   = TIM2_IRQn;
    nv.NVIC_IRQChannelPreemptionPriority = irq_prio;
    nv.NVIC_IRQChannelSubPriority        = 0;
    nv.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nv);

    TIM_Cmd(TIM2, ENABLE);                        /* CEN = 1 */
}

uint16_t tim2_get_count(void)
{
    return (uint16_t)TIM_GetCounter(TIM2);
}

int tim2_overflow_pending(void)
{
    return TIM_GetFlagStatus(TIM2, TIM_FLAG_Update) == SET;
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        if (s_ovf_cb != NULL) {
            s_ovf_cb();
        }
    }
}
