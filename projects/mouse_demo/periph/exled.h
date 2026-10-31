#ifndef __EXLED_H
#define __EXLED_H

#include <stdint.h>

// 博仪创 6PIN 数码管驱动芯片: P00 模拟串口 115200 8N1, 一帧 9 字节, DIG1~DIG4(BYTE3~6) 固定 0.
// 芯片上电是休眠的, 收到一帧就醒; 3 分钟收不到数据又会休眠, 要一直亮就隔一会儿重发一次.
#define iEXLED_CC           0x00        // 共阴
#define iEXLED_CA           0x01        // 共阳
#define iEXLED_OFF          0x02        // 关机, 芯片 1 秒后休眠

// mode: 上面三个之一; pins: DIG5, bit0~6 依次是芯片的 PB1 PB2 PB3 PB4 PB5 PA0 PA1, 1 = 点亮(关机时不用)
void exled_send(uint8_t mode, uint8_t pins);
void exled_tx(uint8_t data);

// DIG5 状态位, 1 = 点亮
#define bEXLED_NUM          (1<<6)      // Num Lock 有效
#define bEXLED_CAPS         (1<<5)      // Caps Lock 有效
#define bEXLED_USB          (1<<4)      // 插着有线(USB 5V 在)
#define bEXLED_WIN_LOCK     (1<<3)      // Win 键锁定(Win 模式下 FN+Win 锁住)

// 按当前状态拼出 DIG5, 发送: exled_send(mode, exled_pins())
uint8_t exled_pins(void);
// 状态有变化就发(当拍发, 下一拍补一帧), 不变时每 2 分钟保活一次; 在 device_led_process 里调用
void exled_process(void);
// 进休眠时调用: 发关机帧; MCU 醒来后 exled_process 第一拍自动重新唤醒
void exled_sleep(void);
#endif