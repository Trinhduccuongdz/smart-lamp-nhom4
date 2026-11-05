# Đèn thông minh tự động theo ánh sáng trên nền FreeRTOS, có giám sát tải CPU

Bài tập lớn môn Hệ thống nhúng – Nhóm 4.
STM32F103C8 (Blue Pill), FreeRTOS V11.1.0, Keil MDK + STM32F10x Standard Peripheral Library V3.5.0.

## Cấu trúc thư mục

```
smart-lamp-nhom4/
├── bsp/            # Board support & pin mapping (board.h), nút, cảm biến LM393, MAX7219, SSD1306
├── hal/            # Driver ngoại vi dùng SPL
│   ├── inc/        # hal_gpio.h, hal_exti.h, hal_uart.h, hal_i2c.h, hal_spi.h, ...
│   └── src/
├── middlewares/    # FreeRTOS (kernel, port ARM_CM3, heap_4) + dịch vụ dùng chung
├── app/            # 6 task FreeRTOS và kênh giao tiếp (app_ipc)
│   ├── inc/        # app_ipc.h, app_cfg.h (ưu tiên, stack), task_xxx.h
│   └── src/
├── platform/       # SPL, CMSIS, startup
├── keil/           # Project Keil SmartLamp.uvprojx
└── main.c          # Init hardware, tạo kênh IPC, tạo task, vTaskStartScheduler()
```

## Phân công

| Thành viên | Task | HAL |
|---|---|---|
| Trịnh Đức Cường (leader) | InputTask, LampTask | GPIO, EXTI, RTC, Flash |
| Kiều Cao Việt | DisplayTask, LogTask | SPI, I2C, UART + DMA |
| Nguyễn Hữu Tiến | ConsoleTask, MonitorTask | Clock, NVIC, Timer, FreeRTOSConfig |

## Quy trình nhánh

`main` (ổn định, không commit trực tiếp) ← `develop` (tích hợp) ← `feature/<tên>` (từng người) ;
`integration/system-v1` ghép `main.c`, `app_ipc.c` trước khi release.
