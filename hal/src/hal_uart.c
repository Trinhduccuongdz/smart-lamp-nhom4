/**
 * @file    hal_uart.c
 * @brief   [HAL] USART1 + DMA1 Channel4 dựa trên SPL (slide USART và slide DMA, Program 4).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "hal_uart.h"
#include "hal_gpio.h"
#include "stm32f10x.h"
#include <stddef.h>

static uart_rx_callback_t      s_rx_cb;
static uart_tx_done_callback_t s_tx_done_cb;
static volatile bool           s_tx_busy;

static void nvic_enable(uint8_t irq, uint8_t prio)
{
    NVIC_InitTypeDef nv;
    nv.NVIC_IRQChannel                   = irq;
    nv.NVIC_IRQChannelPreemptionPriority = prio;
    nv.NVIC_IRQChannelSubPriority        = 0;
    nv.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&nv);
}

void uart1_init(uint32_t baud, uint8_t irq_prio,
                uart_rx_callback_t rx_cb, uart_tx_done_callback_t tx_done_cb)
{
    s_rx_cb = rx_cb;
    s_tx_done_cb = tx_done_cb;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    gpio_config(GPIOA, 9, GPIO_MODE_AF_PP);            /* TX1 */
    gpio_config_input_pull(GPIOA, 10, GPIO_PULL_UP);   /* RX1 (slide: "RX1 = input with pull-up") */

    /* 8N1. SPL tự tính BRR = PCLK2 / baud = 72 MHz / 115200 = 625 (USART1 nằm trên APB2) */
    USART_InitTypeDef u;
    u.USART_BaudRate            = baud;
    u.USART_WordLength          = USART_WordLength_8b;
    u.USART_StopBits            = USART_StopBits_1;
    u.USART_Parity              = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &u);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);     /* RXNEIE = 1: ngắt khi nhận được byte */
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);     /* CR3.DMAT = 1 */
    USART_Cmd(USART1, ENABLE);                         /* UE = 1 */

    /* DMA1 kênh 4 = USART1_TX: bộ nhớ -> thanh ghi DR, ngắt khi chuyển xong */
    DMA_DeInit(DMA1_Channel4);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);

    nvic_enable(USART1_IRQn, irq_prio);
    nvic_enable(DMA1_Channel4_IRQn, irq_prio);
}

bool uart1_send_dma(const uint8_t *buf, uint16_t len)
{
    if (s_tx_busy || len == 0U) {
        return false;
    }
    s_tx_busy = true;

    DMA_Cmd(DMA1_Channel4, DISABLE);                   /* phải tắt kênh trước khi nạp lại */
    DMA_InitTypeDef d;
    d.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;  /* CPAR */
    d.DMA_MemoryBaseAddr     = (uint32_t)buf;          /* CMAR */
    d.DMA_DIR                = DMA_DIR_PeripheralDST;  /* DIR = 1: đọc bộ nhớ, ghi ngoại vi */
    d.DMA_BufferSize         = len;                    /* CNDTR */
    d.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    d.DMA_MemoryInc          = DMA_MemoryInc_Enable;   /* MINC = 1 */
    d.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    d.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    d.DMA_Mode               = DMA_Mode_Normal;
    d.DMA_Priority           = DMA_Priority_Medium;
    d.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &d);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);    /* DMA_Init ghi đè CCR nên bật lại TCIE */
    DMA_Cmd(DMA1_Channel4, ENABLE);                    /* bắt đầu chuyển */
    return true;
}

void uart1_send_blocking(const char *s)
{
    while (*s != '\0') {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET) { }
        USART_SendData(USART1, (uint8_t)*s++);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) { }
}

void USART1_IRQHandler(void)
{
    /* Tràn (ORE): đọc DR để xóa cờ, bỏ byte lỗi */
    if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET) {
        (void)USART_ReceiveData(USART1);
        return;
    }
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t byte = (uint8_t)USART_ReceiveData(USART1);   /* đọc DR tự xóa RXNE */
        if (s_rx_cb != NULL) {
            s_rx_cb(byte);
        }
    }
}

void DMA1_Channel4_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC4) != RESET) {
        DMA_ClearITPendingBit(DMA1_IT_GL4);            /* xóa cờ (DMA_IFCR) */
        DMA_Cmd(DMA1_Channel4, DISABLE);
        s_tx_busy = false;
        if (s_tx_done_cb != NULL) {
            s_tx_done_cb();
        }
    } else {
        DMA_ClearITPendingBit(DMA1_IT_GL4);
    }
}
