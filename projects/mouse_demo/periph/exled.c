#include "om_driver.h"
#include "exled.h"
#include "time_delay.h"
#include "main.h"
#include "matrix_key.h"
#define iEXLED_PIN          0           // P00
#define iEXLED_BAUD         115200
#define iEXLED_BIT          ((SYS_CLK + iEXLED_BAUD/2) / iEXLED_BAUD)   // 48M 下一位 417 个 CPU 周期, 编译时算好

#define rgb_54e_uart_h()        (OM_GPIO0->DATAOUTSET = (1<<UART_54E_PIN))
#define rgb_54e_uart_l()        (OM_GPIO0->DATAOUTCLR = (1<<UART_54E_PIN))

static uint8_t              exled_last = 0xFF;      // 上次发出的 DIG5; bit7 不用, 0xFF 保证上电第一拍就发
static bool                 bexled_again;           // 下一拍再补发一帧
static uint32_t             exled_time;             // 上次发帧的 RTC 时刻

// 模拟串口发一个字节: 起始位 0 + 8 位数据(低位先) + 停止位 1, 关中断约 87us.
// 位宽按 SysTick 卡(CPU 时钟的 24 位自由倒计数, SystemInit 里开着, SDK 的 DRV_DELAY_US 也靠它),
// 每个边沿都从起始时刻算: 误差不累积, 不依赖编译出来的指令周期, 蓝牙降到 12M 也自动跟着变.
// 整个函数放 RAM: 在 Flash 里跑时, 取指未命中缓存会把某一位卡长约 7us(关中断也挡不住).
__RAM_CODE void exled_tx(uint8_t data)
{
    uint32_t bit = iEXLED_BIT >> OM_RCC->CLKCTRL;           // CPU 时钟 = 48M >> CLKCTRL, 48M 下一位 417
    uint32_t frame = ((uint32_t)data << 1) | (1u << 9);     // 位 0 起始位, 位 1~8 数据, 位 9 停止位
    uint32_t t0, t = 0;

    OM_CRITICAL_BEGIN();
    t0 = SysTick->VAL;
    do
    {
        if(frame & 1)
            rgb_54e_uart_h();
        else
            rgb_54e_uart_l();
        frame >>= 1;
        t += bit;
        while(((t0 - SysTick->VAL) & SysTick_VAL_CURRENT_Msk) < t);
    }
    while(frame);
    OM_CRITICAL_END();
}

void exled_send(uint8_t mode, uint8_t pins)
{
    uint8_t i;

    if(iEXLED_OFF == mode)
        pins = 0;                                           // 关机帧按说明书: 00 AA 02 00 00 00 00 00 FE

    OM_GPIO0->DATAOUTSET = 1u << iEXLED_PIN;                // 空闲为高: 先拉高再开输出, 免得多出一个起始位
    OM_GPIO0->OUTENSET = 1u << iEXLED_PIN;

    exled_tx(0x00);
    exled_tx(0xAA);
    exled_tx(mode);
    for(i=0; i<4; i++)
        exled_tx(0);                                        // DIG1~DIG4 固定 0
    exled_tx(pins);
    exled_tx((uint8_t)(0 - mode - pins));                   // SUM = 0x100 - (BYTE2 + ... + BYTE7)
}

uint8_t exled_pins(void)
{
    uint8_t pins = 0;

    if(kb_led_status & bnum_lock)
        pins |= bEXLED_NUM;
    if(kb_led_status & bcaps_lock)
        pins |= bEXLED_CAPS;
    if(0 != USB_5V_PIN)
        pins |= bEXLED_USB;
    if((key_ctrl & bwin_mac_sataus) && (key_ctrl & bwin_lock_status))   // Win 锁只在 Win 模式下起作用
        pins |= bEXLED_WIN_LOCK;

    return pins;
}

// 每个指示灯节拍调一次(device_led_process 里, 约 15ms): 状态有变化就当拍发出;
// 变化后下一拍再补一帧(芯片睡着时收到的那一帧可能只够把它叫醒). 状态不变时每 2 分钟发一帧保活:
// 芯片 3 分钟收不到数据会休眠灭灯.
void exled_process(void)
{
    uint8_t  pins = exled_pins();
    uint32_t now  = drv_rtc_read(OM_RTC);

    if((pins == exled_last) && (false == bexled_again) && (mMS_TO_TICK(2*60*1000) > (now - exled_time)))
        return;

    bexled_again = (pins != exled_last);
    exled_last   = pins;
    exled_time   = now;
    exled_send(iEXLED_CC, pins);
}

// 键盘进休眠/关机时调用(mouse_periph_wakeup_enable 里): 发关机帧, 灯灭, 芯片 1 秒后休眠.
// 休眠期间不再调 exled_process; MCU 醒来后第一拍 exled_last 对不上, 当拍发帧叫醒芯片, 下一拍再补一帧.
void exled_sleep(void)
{
    exled_send(iEXLED_OFF, 0);
    exled_last   = 0xFF;
    bexled_again = false;
}
