/**
 * @file    hal_spi.c
 * @brief   [HAL] SPI1 master dựa trên SPL (SPI_Init ghi SPI_CR1, cờ TXE/BSY trong SPI_SR).
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "hal_spi.h"
#include "hal_gpio.h"
#include "stm32f10x.h"

void spi1_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE);

    gpio_config(GPIOA, 5, GPIO_MODE_AF_PP);   /* SCK  */
    gpio_config(GPIOA, 7, GPIO_MODE_AF_PP);   /* MOSI */

    SPI_InitTypeDef s;
    s.SPI_Direction         = SPI_Direction_1Line_Tx;   /* chỉ phát, MAX7219 không trả dữ liệu */
    s.SPI_Mode              = SPI_Mode_Master;          /* MSTR = 1 */
    s.SPI_DataSize          = SPI_DataSize_16b;         /* DFF = 1: gói lệnh MAX7219 16 bit */
    s.SPI_CPOL              = SPI_CPOL_Low;             /* clock nghỉ ở mức 0 */
    s.SPI_CPHA              = SPI_CPHA_1Edge;           /* lấy mẫu cạnh lên -> SPI mode 0 */
    s.SPI_NSS               = SPI_NSS_Soft;             /* SSM = 1, SSI = 1: chân NSS tự do */
    s.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;  /* BR = 010: 72 MHz / 8 = 9 MHz (MAX7219 <= 10 MHz) */
    s.SPI_FirstBit          = SPI_FirstBit_MSB;
    s.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI1, &s);
    SPI_Cmd(SPI1, ENABLE);                              /* SPE = 1 */
}

void spi1_write16(uint16_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) { }   /* chờ bộ đệm phát trống */
    SPI_I2S_SendData(SPI1, data);
    spi1_wait_idle();
}

void spi1_wait_idle(void)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) { }
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) { }
}
