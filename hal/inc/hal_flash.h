/**
 * @file    hal_flash.h
 * @brief   [HAL] Xóa/ghi Flash nội (trang 1 KB) để lưu dữ liệu cấu hình (dùng SPL).
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * Lưu ý: khi xóa/ghi Flash, CPU dừng đọc lệnh từ Flash vài ms -> gọi ít, không gọi trong ISR.
 */
#ifndef HAL_FLASH_H
#define HAL_FLASH_H

#include <stdint.h>
#include <stdbool.h>

#define FLASH_PAGE_BYTES   1024U

/** Đọc `len` byte từ địa chỉ Flash `addr` (Flash đọc trực tiếp như bộ nhớ thường). */
void flash_hw_read(uint32_t addr, void *out, uint32_t len);

/** Xóa 1 trang rồi ghi `len` byte (làm tròn lên bội 2). @return true nếu đọc lại khớp. */
bool flash_hw_write_page(uint32_t page_addr, const void *data, uint32_t len);

#endif /* HAL_FLASH_H */
