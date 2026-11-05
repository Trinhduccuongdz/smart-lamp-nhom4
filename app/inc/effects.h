/**
 * @file    effects.h
 * @brief   [APPLICATION] Tạo khung hình và độ sáng cho ma trận LED theo chế độ (thuần logic).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include "lamp_types.h"

/** Vẽ 8 hàng của một module. col_offset dùng khi nối nhiều module (module thứ d: d * 8). */
void    effects_render(uint8_t mode, uint32_t frame, uint8_t col_offset, uint8_t rows[8]);
/** Độ sáng MAX7219 (0..15) cho level 1..16 tại khung hình `frame`. */
uint8_t effects_intensity(uint8_t mode, uint32_t frame, uint8_t level);

/**
 * Khung hình đếm ngược (chế độ MANUAL) trên ma trận 8x8:
 *  - còn >= 1 phút: hai chữ số PHÚT (số 3x5, hàng 1..5, cột 1..3 và 5..7)
 *    + thanh giây ở hàng 7: 8 chấm, mỗi chấm ~7,5 s, ngắn dần
 *  - còn < 1 phút: hai chữ số GIÂY, không có thanh
 */
void    effects_render_countdown(uint16_t remain_s, uint16_t total_s, uint32_t frame, uint8_t rows[8]);

#endif /* EFFECTS_H */
