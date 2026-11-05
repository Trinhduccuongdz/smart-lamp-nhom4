/**
 * @file    icons.h
 * @brief   [APPLICATION] Kho icon hiển thị trên OLED (chế độ Manual), lưu trong Flash.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 *
 * Dữ liệu là mảng hằng (const) nên trình biên dịch đặt ở vùng Flash (RO-data), không tốn RAM.
 * Dữ liệu sinh bởi docs/src/icons/gen_icons.py.
 */
#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>

#define ICON_COUNT   16U
#define ICON_W       24U
#define ICON_H       24U
#define ICON_BYTES   (ICON_W * ICON_H / 8U)   /* 72 byte / icon */

/** Ảnh 1 bit của icon thứ index (index ngoài miền -> icon 0). */
const uint8_t *icon_bitmap(uint8_t index);
/** Tên icon (không dấu, vừa font 5x7 của OLED). */
const char    *icon_name(uint8_t index);

#endif /* ICONS_H */
