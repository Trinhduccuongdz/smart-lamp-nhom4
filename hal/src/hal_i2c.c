/**
 * @file    hal_i2c.c
 * @brief   [HAL] I2C1 master dựa trên SPL, theo các bước "Sending data in master mode" (slide I2C):
 *          START -> chờ SB (EV5) -> gửi địa chỉ -> chờ ADDR (EV6) -> gửi từng byte, chờ TxE (EV8)
 *          -> chờ BTF (EV8_2) -> STOP.
 * @owner   Người 2 (Hiển thị + xuất dữ liệu)
 */
#include "hal_i2c.h"
#include "hal_gpio.h"
#include "stm32f10x.h"

#define I2C_TIMEOUT  20000UL   /* số vòng lặp chờ tối đa cho mỗi bước */

/* Chờ một sự kiện I2C; trả về 0 nếu xảy ra, -2 nếu slave không ACK, -1 nếu quá thời gian */
static int wait_event(uint32_t event)
{
    for (uint32_t t = 0; t < I2C_TIMEOUT; t++) {
        if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF) == SET) { return -2; }
        if (I2C_CheckEvent(I2C1, event) == SUCCESS)      { return 0; }
    }
    return -1;
}

/* Trễ ngắn ~5 us (nửa chu kỳ clock 100 kHz) dùng khi tự tạo xung trên SCL */
static void bus_delay(void)
{
    for (volatile uint32_t i = 0; i < 60U; i++) { }
}

/*
 * Giải phóng bus bị kẹt. Nếu vi điều khiển bị reset khi OLED đang gửi/nhận dở một byte,
 * OLED vẫn giữ SDA ở mức 0 chờ xung clock -> khối I2C không tạo được START.
 * Cách xử lý chuẩn (I2C-bus specification, mục "Bus clear"): tạm dùng PB6/PB7 làm GPIO
 * open-drain, phát tối đa 9 xung trên SCL cho tới khi slave nhả SDA, rồi tự tạo điều kiện STOP.
 */
static void i2c1_bus_clear(void)
{
    gpio_config(GPIOB, 6, GPIO_MODE_OUT_OD);   /* SCL */
    gpio_config(GPIOB, 7, GPIO_MODE_OUT_OD);   /* SDA */
    gpio_set(GPIOB, 6);
    gpio_set(GPIOB, 7);
    bus_delay();

    for (int i = 0; i < 9 && !gpio_read(GPIOB, 7); i++) {   /* SDA còn bị giữ ở 0 */
        gpio_reset(GPIOB, 6); bus_delay();
        gpio_set(GPIOB, 6);   bus_delay();
    }
    /* Điều kiện STOP: SDA đi từ 0 lên 1 khi SCL đang ở mức 1 */
    gpio_reset(GPIOB, 6); bus_delay();
    gpio_reset(GPIOB, 7); bus_delay();
    gpio_set(GPIOB, 6);   bus_delay();
    gpio_set(GPIOB, 7);   bus_delay();
}

void i2c1_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    I2C_Cmd(I2C1, DISABLE);
    i2c1_bus_clear();                          /* nhả bus nếu slave đang giữ SDA */

    gpio_config(GPIOB, 6, GPIO_MODE_AF_OD);    /* SCL */
    gpio_config(GPIOB, 7, GPIO_MODE_AF_OD);    /* SDA */

    /* Reset mềm khối I2C để xóa cờ BUSY/ARLO/BERR còn sót (errata STM32F10x mục 2.13.7) */
    I2C_SoftwareResetCmd(I2C1, ENABLE);
    I2C_SoftwareResetCmd(I2C1, DISABLE);

    I2C_InitTypeDef i;
    i.I2C_Mode                = I2C_Mode_I2C;
    i.I2C_DutyCycle           = I2C_DutyCycle_2;      /* Fast mode, DUTY = 0: tlow/thigh = 2 */
    i.I2C_OwnAddress1         = 0x00;
    i.I2C_Ack                 = I2C_Ack_Enable;
    i.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    i.I2C_ClockSpeed          = 400000;               /* SPL tự tính: CR2.FREQ = 36, CCR = 36M/(3x400k) = 30, TRISE = 11 */
    I2C_Init(I2C1, &i);
    I2C_Cmd(I2C1, ENABLE);                            /* PE = 1 */
}

static i2c_status_t i2c1_abort(i2c_status_t err)
{
    I2C_GenerateSTOP(I2C1, ENABLE);
    I2C_ClearFlag(I2C1, I2C_FLAG_AF);
    return err;
}

i2c_status_t i2c1_write(uint8_t addr7, uint8_t prefix, const uint8_t *data, uint16_t len)
{
    uint32_t t;
    int r;

    /* 1. Chờ bus rảnh (BUSY trong SR2) */
    for (t = 0; t < I2C_TIMEOUT && I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET; t++) { }
    if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) == SET) {
        i2c1_init();                                   /* thử khôi phục bus */
        return I2C_ERR_BUSY;
    }

    /* Chờ lệnh STOP của lần truyền trước kết thúc (bit STOP trong CR1 tự xóa) */
    for (t = 0; t < I2C_TIMEOUT && (I2C1->CR1 & I2C_CR1_STOP) != 0U; t++) { }

    /* 2-3. START, chờ EV5 (SB = 1, MSL = 1) */
    I2C_GenerateSTART(I2C1, ENABLE);
    if (wait_event(I2C_EVENT_MASTER_MODE_SELECT) != 0) { return i2c1_abort(I2C_ERR_START); }

    /* 4-5. Gửi địa chỉ + bit W, chờ EV6 (ADDR = 1). I2C_CheckEvent đọc SR1 rồi SR2 nên tự xóa ADDR */
    I2C_Send7bitAddress(I2C1, (uint8_t)(addr7 << 1), I2C_Direction_Transmitter);
    r = wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
    if (r == -2) { return i2c1_abort(I2C_ERR_NACK); }
    if (r != 0)  { return i2c1_abort(I2C_ERR_TIMEOUT); }

    /* 6-7. Gửi byte tiền tố rồi dữ liệu, mỗi byte chờ EV8 (TxE = 1) */
    I2C_SendData(I2C1, prefix);
    for (uint16_t i = 0; i < len; i++) {
        r = wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTING);
        if (r != 0) { return i2c1_abort(r == -2 ? I2C_ERR_NACK : I2C_ERR_TIMEOUT); }
        I2C_SendData(I2C1, data[i]);
    }

    /* 8. Chờ EV8_2 (TxE = 1, BTF = 1) rồi STOP */
    r = wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
    I2C_GenerateSTOP(I2C1, ENABLE);
    return (r == 0) ? I2C_OK : I2C_ERR_TIMEOUT;
}
