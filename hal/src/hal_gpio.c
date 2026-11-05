#include "hal_gpio.h"

	static uint16_t pin_mask(uint8_t pin)
	{
			return (uint16_t)(1U << pin);   /* GPIO_Pin_x của SPL = 1 << x */
	}
// cap clock cho port A,B,C, kem chuc nang funtion
void gpio_clock_enable_all(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
}

void gpio_config(GPIO_TypeDef *port, uint8_t pin, gpio_mode_t mode)
{
    static const GPIOMode_TypeDef k_map[] = {
        GPIO_Mode_AIN,          /* GPIO_MODE_ANALOG      -> CNF/MODE = 0x0 */
        GPIO_Mode_IN_FLOATING,  /* GPIO_MODE_IN_FLOATING -> 0x4 */
        GPIO_Mode_Out_PP,       /* GPIO_MODE_OUT_PP      -> 0x3 */
        GPIO_Mode_Out_OD,       /* GPIO_MODE_OUT_OD      -> 0x7 */
        GPIO_Mode_AF_PP,        /* GPIO_MODE_AF_PP       -> 0xB */
        GPIO_Mode_AF_OD         /* GPIO_MODE_AF_OD       -> 0xF */
    };
    GPIO_InitTypeDef init;
    init.GPIO_Pin   = pin_mask(pin);
    init.GPIO_Speed = GPIO_Speed_50MHz;
    init.GPIO_Mode  = k_map[mode];
		// luu cau hinh
    GPIO_Init(port, &init);
}

void gpio_config_input_pull(GPIO_TypeDef *port, uint8_t pin, gpio_pull_t pull)
{
    /* SPL tự ghi CRL/CRH = 0x8 và đặt bit ODR (1 = kéo lên, 0 = kéo xuống) */
    GPIO_InitTypeDef init;
    init.GPIO_Pin   = pin_mask(pin);
    init.GPIO_Speed = GPIO_Speed_50MHz;
    init.GPIO_Mode  = (pull == GPIO_PULL_UP) ? GPIO_Mode_IPU : GPIO_Mode_IPD;
    GPIO_Init(port, &init);
}
// set muc cao cho chan pin truyen vao
void gpio_set(GPIO_TypeDef *port, uint8_t pin)
{
    GPIO_SetBits(port, pin_mask(pin));
}
// reset muc thap cho chan pin truyen vao
void gpio_reset(GPIO_TypeDef *port, uint8_t pin)
{
    GPIO_ResetBits(port, pin_mask(pin));
}
// ghi logic 0-1 ra chan GPIO tuong ung voi chi so pin
void gpio_write(GPIO_TypeDef *port, uint8_t pin, bool level)
{
    GPIO_WriteBit(port, pin_mask(pin), level ? Bit_SET : Bit_RESET);
}
// doc trang thai muc logic dau vao
bool gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return GPIO_ReadInputDataBit(port, pin_mask(pin)) == (uint8_t)Bit_SET;
}
// dao trang thai muc logic tu thanh ghi ODR tai do, xong roi dao muc.
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    uint16_t m = pin_mask(pin);
    if (GPIO_ReadOutputDataBit(port, m) == (uint8_t)Bit_SET) {
        GPIO_ResetBits(port, m);
    } else {
        GPIO_SetBits(port, m);
    }
}
