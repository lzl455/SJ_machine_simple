/*******************************************************************************
* Copyright (c) 2013, SHENZHEN SEMITEK Co.,LTD.
* All rights reserved.
*
* 文件名称: wheel.c
*
* 功能说明: 滚轮扫描处理
* 修改说明:
*******************************************************************************/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "om_driver.h"

#include "main.h"
#include "wheel.h"
#include "led.h"

#include "key_code_define.h"
#include "flash_process.h"
#include "hid_process.h"

//-----------------------------------------------------------------------------;
// 常量定义区
//
#define iWHEEL_VOL_REPEAT       2       // 编码器转一格发几次音量加/减键: Windows 每收到一次改 2, 发 2 次 = 每格 ±4
#define iWHEEL_LED_SAVE_MS      1000    // 编码器调亮度: 停手这么久才存 flash, 转的过程中一格写一次太伤 flash

//-----------------------------------------------------------------------------;
// 变量定义区
//
bool                        bwheel_mode_swtich;
uint8_t                     wheel_status;
uint32_t                    wheel_press_time_count;
static uint8_t              wheel_vol_sent;             // 当前这一格已经发了几次音量键
static bool                 bwheel_led_save;            // 编码器调过亮度, 还没存
static uint32_t             wheel_led_save_tick;        // 最后一次调亮度的时刻

//=============================================================================;
// 代码区                                                                      ;
//=============================================================================;

/*******************************************************************************
* 函数名称: wheel_scan()
* 功能描述: 滚轮扫描处理
* 输入参数：无
* 输出参数：无
* 说    明:
*******************************************************************************/
void wheel_scan(void)
{
    if(OM_ENCODER->VALID&ENCODER_VALID_CNT_MASK)
    {
        wheel_data = OM_ENCODER->CNT;
        if(wheel_data & 0x08)
            wheel_data |= 0xf0;
        else
            wheel_data &= 0x0f;
    }

    rtc_cur_value = drv_rtc_read(OM_RTC);

    // 亮度存在配置区(flash_save_cfg_dpi), 不在状态区; 停手后存一次
    if((true == bwheel_led_save) && (mMS_TO_TICK(iWHEEL_LED_SAVE_MS) < (rtc_cur_value - wheel_led_save_tick)))
    {
        bwheel_led_save = false;
        bsave_cfg_dpi = true;
    }

    if(0==(wheel_status&bwheel_press))
    {
        bwheel_mode_swtich = false;
        if(0!=(wheel_status&bwheel_press_bak))
        {
            wheel_status &= ~bwheel_press_bak;

            if(mMS_TO_TICK(1000) > (rtc_cur_value - wheel_press_time_count))
            {
                wheel_press_time_count = 0;
                if(0 ==(wheel_status&bwheel_mode))
                {
                    mmk_data_l = iMMK_MUTE_L;
                    mmk_data_h = iMMK_MUTE_H;

                    m_data_flag |= bmmk_data_send;
                    mmk_tx_count = 8;
                }
                else
                {                                   // 背光模式短按: 开/关背光(原来是切灯效, RGB 灯效已移除)
                    led_backlight_toggle();
                    hid_ctrl_reg |= bdevice_status_change;
                    bsave_status = true;
                }
            }
        }
    }
    else
    {
        if(0==(wheel_status&bwheel_press_bak))
        {
            wheel_status |= bwheel_press_bak;
            wheel_press_time_count = rtc_cur_value;
        }
        else
        {
            if(mMS_TO_TICK(3000) < (rtc_cur_value - wheel_press_time_count))
            {
                if(false == bwheel_mode_swtich)
                {
                    bwheel_mode_swtich = true;
                    wheel_status ^= bwheel_mode;
                    bsave_status = true;

                    if(0 == hint_time_count)
                        hint_time_count = 0x37;
                }
		    }
        }
    }

    if(0 != wheel_data)
	{
        if(0 ==(wheel_status&bwheel_mode))
        {
            if(0==(m_data_flag&bmmk_data_send))
            {
                if(0!=(wheel_data&(1<<7)))
                {
                    mmk_data_l = iMMK_VOLUMEDOWN_L;
                    mmk_data_h = iMMK_VOLUMEDOWN_H;
                }
                else
                {
                     mmk_data_l = iMMK_VOLUMEUP_L;
                     mmk_data_h = iMMK_VOLUMEUP_H;
                }

                /* 一格发 iWHEEL_VOL_REPEAT 次才消掉这一格. 两次之间按键扫描
                   (matrixkey_scan 在 wheel_scan 之前)会先补发一个"松开"报文,
                   主机才会当成两次按下, 而不是一直按着 */
                wheel_vol_sent++;
                if(iWHEEL_VOL_REPEAT <= wheel_vol_sent)
                {
                    wheel_vol_sent = 0;
                    if(0!=(wheel_data&(1<<7)))
                        wheel_data++;
                    else
                        wheel_data--;
                }
                m_data_flag |= bmmk_data_send;
                mmk_tx_count = 8;
            }
        }
        else
        {
            if(false==bled_bright_en)
			{
            	wheel_data = 0;
    			return;
			}

            if(0!=(wheel_data&(1<<7)))
            {
                if(led_light_lev > iLED_LEV_MIN)
                {                                   // 一格一档, 和按键一样走 6 档
                    led_light_lev = (enum led_level)lev6_next(led_light_lev, false);
                    hid_ctrl_reg |= bdevice_status_change;
                    bwheel_led_save = true;
                    wheel_led_save_tick = rtc_cur_value;
                }
                else
                {
                    if(0 == hint_time_count)
                        hint_time_count = 0x37;
                }
            }
            else
            {
                if(led_light_lev < iLED_LEV_MAX)
                {
                    led_light_lev = (enum led_level)lev6_next(led_light_lev, true);
                    hid_ctrl_reg |= bdevice_status_change;
                    bwheel_led_save = true;
                    wheel_led_save_tick = rtc_cur_value;
                }
                else
                {
                    led_light_lev = iLED_LEV_MAX;
                    if(0 == hint_time_count)
                        hint_time_count = 0x37;
                }
            }

            wheel_data = 0;
        }
    }
}
