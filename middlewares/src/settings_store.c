/**
 * @file    settings_store.c
 * @brief   [MIDDLEWARE] Lưu cài đặt vào Flash qua hal_flash.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 */
#include "settings_store.h"
#include "hal_flash.h"
#include <string.h>
// khi he thong khoi dong va doc vung nho Flash, neu gia tri doc duoc khop voi du lieu nay thi vung nho da duoc ghi du lieu hop le
// nguoc lai thi he thong biet la du lieu ko hop le thi tu dong nap cau hinh mac dinh
#define STORE_MAGIC  0x4C53U     /* "SL" */

typedef struct {
    uint16_t magic; // ma nhan dien toan ven, chua gia tri STORE MAGIC, khi he thong khoi dong, so sanh de quyet dinh xem nap du lieu nao
    uint16_t len;   // do dai du lieu thuc te
    uint16_t sum;   // ma kiem tra loi-check sum
    uint16_t reserved;
    uint8_t  data[SETTINGS_STORE_MAX]; // vung chua du lieu tho, Payload
} store_page_t;

static uint16_t checksum(const uint8_t *p, uint16_t len)
{
    uint16_t s = 0x1234U, i;
    for (i = 0U; i < len; i++) { s = (uint16_t)((s << 1) | (s >> 15)); s ^= p[i]; }
    return s;
}

bool settings_store_load(void *data, uint16_t len)
{
    store_page_t pg;
    if (len > SETTINGS_STORE_MAX) { return false; }
    flash_hw_read(SETTINGS_STORE_ADDR, &pg, 8U + len);
    if (pg.magic != STORE_MAGIC || pg.len != len || pg.sum != checksum(pg.data, len)) { return false; }
    memcpy(data, pg.data, len);
    return true;
}

bool settings_store_save(const void *data, uint16_t len)
{
    store_page_t pg;
    uint8_t cur[SETTINGS_STORE_MAX];
    if (len > SETTINGS_STORE_MAX) { return false; }
    if (settings_store_load(cur, len) && memcmp(cur, data, len) == 0) { return true; }
    pg.magic = STORE_MAGIC;
    pg.len = len;
    pg.reserved = 0xFFFFU;
    memcpy(pg.data, data, len);
    pg.sum = checksum(pg.data, len);
    return flash_hw_write_page(SETTINGS_STORE_ADDR, &pg, 8U + len);
}
