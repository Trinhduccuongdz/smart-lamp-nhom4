/**
 * @file    hal_nvic.c
 * @brief   [HAL] NVIC: chọn nhóm ưu tiên (SPL: NVIC_PriorityGroupConfig ghi SCB->AIRCR.PRIGROUP).
 * @owner   Người 3 (Giao tiếp lệnh + giám sát hệ thống)
 */
#include "hal_nvic.h"
#include "stm32f10x.h"

void nvic_init_priority_group(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);   /* 4 bit preemption (0..15), 0 bit sub */
}
