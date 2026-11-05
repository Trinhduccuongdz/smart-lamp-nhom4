/**
 * @file    hal_uart.h
 * @brief   [HAL] USART1: nhận bằng ngắt RXNE, phát bằng DMA1 kênh 4. Dùng SPL.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Chân: PA9 = TX (AF push-pull) -> nối RX của mạch USB-TTL
 *       PA10 = RX (input pull-up) <- nối TX của mạch USB-TTL
 * Driver không phụ thuộc FreeRTOS: báo sự kiện cho tầng trên bằng callback (chạy trong ISR).
 */
#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*uart_rx_callback_t)(uint8_t byte);   /* gọi từ USART1_IRQHandler */
typedef void (*uart_tx_done_callback_t)(void);      /* gọi từ DMA1_Channel4_IRQHandler */

/**
 * Khởi tạo USART1 8N1.
 * @param baud      tốc độ baud (ví dụ 115200). BRR = PCLK2 / baud (slide USART).
 * @param irq_prio  ưu tiên NVIC cho ngắt USART1 và DMA1_Channel4.
 */
void uart1_init(uint32_t baud, uint8_t irq_prio,
                uart_rx_callback_t rx_cb, uart_tx_done_callback_t tx_done_cb);

/** Bắt đầu phát `len` byte bằng DMA. Trả về false nếu DMA đang bận. Buffer phải tồn tại đến khi tx_done. */
bool uart1_send_dma(const uint8_t *buf, uint16_t len);

/** Phát kiểu polling (chỉ dùng khi chưa có scheduler hoặc khi báo lỗi nghiêm trọng). */
void uart1_send_blocking(const char *s);

#endif /* HAL_UART_H */
