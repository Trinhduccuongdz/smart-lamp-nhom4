/**
 * @file    console.c
 * @brief   [MIDDLEWARES] Console UART dựa trên driver uart (ngắt RX + DMA TX) và FreeRTOS semaphore.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "console.h"
#include "hal_uart.h"
#include "board.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>

#define LINE_SLOTS  2U   /* hàng đợi vòng 2 dòng: ISR ghi dòng mới trong khi task đọc dòng cũ */

static char              s_rx_line[CONSOLE_LINE_MAX];
static uint8_t           s_rx_len;
static char              s_lines[LINE_SLOTS][CONSOLE_LINE_MAX];
static volatile uint8_t  s_head;   /* ISR ghi */
static volatile uint8_t  s_tail;   /* task đọc */
static console_line_cb_t s_on_line;

static SemaphoreHandle_t s_tx_done;
static uint8_t           s_tx_buf[96];

/* ---- Ngữ cảnh ISR ---- */
static void on_rx_byte(uint8_t c)
{
    if (c == '\r' || c == '\n') {
        if (s_rx_len == 0U) { return; }              /* bỏ dòng rỗng ("\r\n") */
        uint8_t next = (uint8_t)((s_head + 1U) % LINE_SLOTS);
        if (next != s_tail) {                        /* còn chỗ */
            memcpy(s_lines[s_head], s_rx_line, s_rx_len);
            s_lines[s_head][s_rx_len] = '\0';
            s_head = next;
            if (s_on_line != NULL) { s_on_line(); }
        }
        s_rx_len = 0;
    } else if (c >= 32U && c < 127U && s_rx_len < CONSOLE_LINE_MAX - 1U) {
        s_rx_line[s_rx_len++] = (char)c;
    }
}

static void on_tx_done(void)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_tx_done, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ---- Ngữ cảnh task ---- */
void console_init(uint32_t baud, console_line_cb_t on_line)
{
    s_on_line = on_line;
    s_tx_done = xSemaphoreCreateBinary();
    configASSERT(s_tx_done != NULL);
    uart1_init(baud, BOARD_IRQ_PRIO_UART, on_rx_byte, on_tx_done);
}

bool console_readline(char *out, size_t size)
{
    if (s_tail == s_head) {
        return false;
    }
    strncpy(out, s_lines[s_tail], size - 1U);
    out[size - 1U] = '\0';
    s_tail = (uint8_t)((s_tail + 1U) % LINE_SLOTS);
    return true;
}

void console_write(const char *s, size_t len)
{
    if (len > sizeof(s_tx_buf)) { len = sizeof(s_tx_buf); }
    memcpy(s_tx_buf, s, len);                         /* DMA đọc từ buffer tĩnh */
    if (uart1_send_dma(s_tx_buf, (uint16_t)len)) {
        /* Chờ ISR DMA báo xong; timeout 100 ms để không treo nếu phần cứng lỗi */
        (void)xSemaphoreTake(s_tx_done, pdMS_TO_TICKS(100));
    }
}

void console_panic_write(const char *s)
{
    uart1_send_blocking(s);
}
