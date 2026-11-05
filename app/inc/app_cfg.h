/**
 * @file    app_cfg.h
 * @brief   [APPLICATION] Bảng ưu tiên, kích thước stack và chu kỳ của các task.
 * @owner   Cả nhóm thống nhất (đổi giá trị phải báo các thành viên)
 *
 *  Task          Người  Ưu tiên  Stack (word)  Kích hoạt
 *  -----------   -----  -------  ------------  -----------------------------------------
 *  MonitorTask     3       4        256        tuần hoàn 1 s (xTaskDelayUntil)
 *  InputTask       1       3        160        notification từ ISR EXTI / timer chống nhiễu
 *  (Timer daemon)  -       3        160        software timer của FreeRTOS
 *  LampTask        1       2        192        qEvents
 *  ConsoleTask     3       2        256        notification từ ISR USART1 (đủ 1 dòng)
 *  DisplayTask     2       1        224        tuần hoàn 40 ms
 *  LogTask         2       1        160        qLog
 *  (Idle)          -       0        128        khi không task nào sẵn sàng
 *
 * Nguyên tắc chọn ưu tiên: task phải phản ứng nhanh và chạy ngắn (Monitor, Input) đặt cao;
 * task tốn thời gian và không gấp (Display vẽ OLED qua I2C, Log in UART) đặt thấp.
 */
#ifndef APP_CFG_H
#define APP_CFG_H

#define PRIO_MONITOR   4
#define PRIO_INPUT     3
#define PRIO_LAMP      2
#define PRIO_CONSOLE   2
#define PRIO_DISPLAY   1
#define PRIO_LOG       1

#define STACK_MONITOR  256
#define STACK_INPUT    160
#define STACK_LAMP     224
#define STACK_CONSOLE  256
#define STACK_DISPLAY  224
#define STACK_LOG      160

#define MONITOR_PERIOD_MS        1000U
#define MONITOR_CALIB_MS         1000U
#define DISPLAY_PERIOD_MS        40U     /* 25 khung hình/giây cho ma trận LED */
#define DISPLAY_OLED_DIVIDER     5U      /* OLED vẽ lại mỗi 5 khung = 200 ms */
#define FADE_STEP_MS             100U    /* mỗi bước sáng/tối dần: 16 bước = 1.6 s */
#define DEBOUNCE_BUTTON_MS       30U
#define DEBOUNCE_SENSOR_MS       1000U   /* tín hiệu sáng/tối phải ổn định 1 s */
#define BUTTON_HOLD_MS           800U
#define CONSOLE_BAUD             115200U

#endif /* APP_CFG_H */
