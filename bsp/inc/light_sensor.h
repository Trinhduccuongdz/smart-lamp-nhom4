/**
 * @file    light_sensor.h
 * @brief   [BSP] Module cảm biến quang trở 3 chân (VCC, GND, DO) dùng LM393.
 * @owner   Người 1 (Đầu vào + điều khiển đèn)
 *
 * DO chỉ có 2 mức: sáng hơn hay tối hơn ngưỡng (chỉnh bằng biến trở trên module).
 * Đa số module: TỐI -> DO = 1, SÁNG -> DO = 0. Nếu module của nhóm ngược lại,
 * đổi LIGHT_SENSOR_DARK_LEVEL thành 0. Cách kiểm tra: che cảm biến, xem đèn LED DO
 * trên module và đo chân DO.
 */
#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <stdbool.h>

#ifndef LIGHT_SENSOR_DARK_LEVEL
#define LIGHT_SENSOR_DARK_LEVEL 1
#endif

/** Callback chạy trong ISR mỗi khi DO đổi mức (chưa chống nhiễu). */
typedef void (*light_sensor_edge_cb_t)(void);

void light_sensor_init(light_sensor_edge_cb_t cb);
bool light_sensor_is_dark(void);

#endif /* LIGHT_SENSOR_H */
