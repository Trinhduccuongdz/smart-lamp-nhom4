#include "light_sensor.h"
#include "board.h"
#include "hal_gpio.h"
#include "hal_exti.h"
#include <stddef.h>
// bien luu tru con tro ham callback 
static light_sensor_edge_cb_t s_cb;

static void on_exti(uint8_t line)
{
		// cam bien chi su dung dung 1 chan phan cung duy nhat, nen tham so line ko can dung den
    (void)line;
		// ngay khi ngat EXTI cua cam bien boc phat, ham lap tuc kiem tra an toan va kich hoat thang callback s_cb()
    if (s_cb != NULL) { s_cb(); }
}
// Ktao phan cung cam bien
void light_sensor_init(light_sensor_edge_cb_t cb)
{	  
		// luu lai dia chi ham xu ly sk ngat suon sung do tang tren truyen xuong
    s_cb = cb;
		// cau hinh chan Input keo len
    gpio_config_input_pull(BOARD_LIGHT_PORT, BOARD_LIGHT_PIN, GPIO_PULL_UP);
		// cau hinh ngat ngoai
    exti_config(BOARD_LIGHT_PORT, BOARD_LIGHT_PIN, EXTI_EDGE_BOTH, BOARD_IRQ_PRIO_EXTI, on_exti);
}
// Doc va danh gia trang thai anh sang
bool light_sensor_is_dark(void)
{
		// Lay trang thai muc logic hien tai (0 hoac 1) tren chan GPIO cua cam bien
    bool level = gpio_read(BOARD_LIGHT_PORT, BOARD_LIGHT_PIN);
		// tra ve true neu moi truong hien tai khop voi troi toi
    return level == (LIGHT_SENSOR_DARK_LEVEL != 0);
}
