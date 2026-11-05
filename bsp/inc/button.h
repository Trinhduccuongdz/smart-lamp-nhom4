/**
 * @file    button.h
 * @brief   [BSP] Bốn nút nhấn NEXT, OK, UP, DOWN (nối GND, dùng pull-up trong).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

typedef enum {
    BUTTON_NEXT = 0,
    BUTTON_OK   = 1,
    BUTTON_UP   = 2,
    BUTTON_DOWN = 3,
    BUTTON_COUNT
} button_id_t;

/** Callback chạy trong ISR mỗi khi chân nút đổi mức (cả nhấn lẫn nhả, chưa chống nhiễu). */
typedef void (*button_edge_cb_t)(button_id_t id);

void button_init(button_edge_cb_t cb);

/** Đọc mức hiện tại: true = đang nhấn (chân ở mức 0). */
bool button_is_pressed(button_id_t id);

#endif /* BUTTON_H */
