/**
 * @file    settings_store.h
 * @brief   [MIDDLEWARE] Lưu một khối cài đặt vào trang Flash cuối (0x0800FC00) để giữ qua mất điện.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Bố cục trang: [magic 2 byte][độ dài 2 byte][checksum 2 byte][dự phòng 2 byte][dữ liệu].
 * Trang này được loại khỏi vùng nạp chương trình (linker GCC và Keil IROM1 chỉ dùng 63 KB).
 */
#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <stdint.h>
#include <stdbool.h>

#define SETTINGS_STORE_ADDR   0x0800FC00UL
#define SETTINGS_STORE_MAX    64U

/** Đọc khối đã lưu. @return false nếu chưa từng lưu / sai checksum / khác độ dài. */
bool settings_store_load(void *data, uint16_t len);

/** Ghi khối (bỏ qua nếu giống hệt dữ liệu đang có, để đỡ mòn Flash). */
bool settings_store_save(const void *data, uint16_t len);

#endif /* SETTINGS_STORE_H */
