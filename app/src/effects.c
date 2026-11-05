/**
 * @file    effects.c
 * @brief   [APPLICATION] Hiệu ứng FULL / BREATH / WAVE cho ma trận 8x8.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * DisplayTask gọi mỗi 40 ms (25 khung hình/giây).
 */
#include "effects.h"

#define BREATH_PERIOD  64U   /* 64 khung x 40 ms = 2.56 s cho một chu kỳ thở */
#define WAVE_SPEED     3U    /* sóng dịch 1 cột sau mỗi 3 khung */

/* Chiều cao cột (0..8) theo dạng sóng sin rời rạc */
static const uint8_t k_wave[16] = { 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 1, 1, 2, 3 };

void effects_render(uint8_t mode, uint32_t frame, uint8_t col_offset, uint8_t rows[8])
{
    if (mode != LAMP_MODE_WAVE) {
        for (uint8_t r = 0; r < 8U; r++) { rows[r] = 0xFFU; }   /* sáng toàn bộ */
        return;
    }
    for (uint8_t r = 0; r < 8U; r++) { rows[r] = 0x00U; }
    for (uint8_t c = 0; c < 8U; c++) {
        uint8_t h = k_wave[(uint32_t)(c + col_offset + frame / WAVE_SPEED) % 16U];
        /* tô từ hàng dưới (r = 7) lên h hàng; bit 7 = cột trái */
        for (uint8_t k = 0; k < h; k++) {
            rows[7U - k] |= (uint8_t)(0x80U >> c);
        }
    }
}

uint8_t effects_intensity(uint8_t mode, uint32_t frame, uint8_t level)
{
    if (level == 0U) { return 0U; }
    uint8_t top = (uint8_t)(level - 1U);           /* level 1..16 -> 0..15 */
    if (mode != LAMP_MODE_BREATH) {
        return top;
    }
    uint32_t p = frame % BREATH_PERIOD;
    uint32_t tri = (p < BREATH_PERIOD / 2U) ? p : (BREATH_PERIOD - 1U - p);   /* 0..31..0 */
    return (uint8_t)((top * tri) / (BREATH_PERIOD / 2U - 1U));
}

/* Font số 3x5: mỗi hàng 3 bit thấp (bit 2 = cột trái) */
static const uint8_t k_digit3x5[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2}, {7,5,7,5,7}, {7,5,7,1,7}
};

/* Còn >= 1 phút: số phút (hàng 1..5) + thanh giây ở hàng 7, mỗi chấm ~7,5 s.
 * Còn < 1 phút: số giây, không có thanh. */
void effects_render_countdown(uint16_t remain_s, uint16_t total_s, uint32_t frame, uint8_t rows[8])
{
    unsigned v = (remain_s >= 60U) ? (remain_s / 60U) : remain_s;
    (void)total_s; (void)frame;
    if (v > 99U) { v = 99U; }
    for (uint8_t r = 0; r < 8U; r++) { rows[r] = 0x00U; }
    for (uint8_t r = 0; r < 5U; r++) {
        rows[1U + r] = (uint8_t)((k_digit3x5[v / 10U][r] << 4) | k_digit3x5[v % 10U][r]);   /* cột 1..3 | 5..7 */
    }
    if (remain_s >= 60U) {
        unsigned n = ((remain_s % 60U) * 2U + 14U) / 15U;     /* ceil(giây / 7,5) = 0..8 chấm */
        rows[7] = (uint8_t)(0xFFU << (8U - n));
    }
}
