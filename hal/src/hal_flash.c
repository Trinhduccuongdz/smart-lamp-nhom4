#include "hal_flash.h"
#include "stm32f10x.h"
#include <string.h>

void flash_hw_read(uint32_t addr, void *out, uint32_t len)
{
    memcpy(out, (const void *)addr, len);
}
// ham dung de ghi du lieu vao bo nho Flash noi cua VDK 
bool flash_hw_write_page(uint32_t page_addr, const void *data, uint32_t len)
{
		// ep kieu con tro void sang kieu byte khong dau
    const uint8_t *src = (const uint8_t *)data;
    uint32_t i;
		// mac dinh khai bao ok = true
    bool ok = true;
		// neu len > kich thuoc trang , ngat ngay va tra ve false
    if (len > FLASH_PAGE_BYTES) { return false; }
		// Mo khoa Flash
    FLASH_Unlock();
		// xoa sach cac co trang thai/loi
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
		// Ban chat la cac o nho flash chi co the chuyen doi 1->0, de 0->1 thi bat buoc phai xoa toan bo trang, neu xoa gap loi thi co ok=false
    if (FLASH_ErasePage(page_addr) != FLASH_COMPLETE) { ok = false; }
		// Dong goi va ghi du lieu theo chuan 16bit 
    for (i = 0U; ok && (i < len); i += 2U){
				// lay byte thu nhat lam byte thap
        uint16_t hw = src[i];
        if ((i + 1U) < len){ 
						// neu con byte thu 2 thi dich trai 8 bit lam byte cao roi gop vao bien hw
						hw |= (uint16_t)((uint16_t)src[i + 1U] << 8); 
				}
        else  { hw |= 0xFF00U; }
				// goi ham ghi phan cung tien hanh nap tung khoi 16 bit vao dia chi tuong ung
        if (FLASH_ProgramHalfWord(page_addr + i, hw) != FLASH_COMPLETE) { ok = false; }
    }
		// sau khi ghi xong thi ngay lap tuc khoa Flash lai de bao ve 
    FLASH_Lock();
		// xac thuc lai bang memcmp
    return ok && (memcmp((const void *)page_addr, data, len) == 0);
}
