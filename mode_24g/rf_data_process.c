/*******************************************************************************
* Copyright (c) 2013, SHENZHEN HUNTERSUN Co.,LTD.
* All rights reserved.
*
* 文件名称: rf_data_process.c
* 文件描述: rf数据处理
*
* 版    本: V1.0.0
* 作    者:
* 日    期:
* 功能说明:
* 使用说明:
*******************************************************************************/
#include "main_24g.h"
#include "usb_cfg_info.h"

//-----------------------------------------------------------------------------;
//常量定义区
//

//----------------------------------------------------------------------------
//变量定义区
//
//unsigned char               key_data_1;
//unsigned char               mouse_x_1;
//unsigned char               mouse_y_1;
//unsigned char               mouse_xy_1;

//unsigned char               key_data_2;
//unsigned char               mouse_x_2;
//unsigned char               mouse_y_2;
//unsigned char               mouse_xy_2;

//unsigned char               mouse_x_3;
//unsigned char               mouse_y_3;
//unsigned char               mouse_xy_3;
//unsigned char               mouse_z;

//----------------------------------------------------------------------------
//表格定义区
//

//============================================================================;
// 代码区                                                                     ;
//============================================================================;

/******************************************************************************
* 函数名称: clr_mouse_data()
* 功能描述:
* 输入参数: 无
* 输出参数: 无
* 说    明:
******************************************************************************/
void clr_mouse_data(void)
{
    mouse_x_1 = 0;
    mouse_y_1 = 0;
    mouse_xy_1 = 0;

    mouse_x_2 = 0;
    mouse_y_2 = 0;
    mouse_xy_2 = 0;
}

/******************************************************************************
* 函数名称:  发射中回调函数()
   
------------------------------------------------------------------------------
|              2ms               |                2ms               |  // 周期
               |  TX0          |                    |  TX1        |    // 发射
|                 iqr          |                       iqr        |    // iqr时间点
               tx0 idle          |                tx1 idle          |  // 无数据发射
------------------------------------------------------------------------------

******************************************************************************/

//----------------------------------------------------------------------------
// 灯效渲染(logo_process)不放在这两个 RF 回调里, 挪到 main_24g() 主循环和没接收器的等待循环(sleep.c):
// 104 颗灯的运算挤在射频时隙里会卡按键(K96 实测后也这样改了), 而且对码/回连时这里不一定跑, R 键灯就闪不出来.
void rf_tx1_idle_callback(void)
{
    lvd_scan_count++;
    if(4==(lvd_scan_count&0x07))
        device_led_process();                     // 16ms
}

void rf_tx1_irq_callback(void)
{
    lvd_scan_count++;
    if(4==(lvd_scan_count&0x07))
        device_led_process();
}

/******************************************************************************
* 函数名称: rf_sync_ok_process()
* 功能描述:
* 输入参数: 无
* 输出参数: 无
* 说    明: 无
******************************************************************************/
void rf_sync_ok_process(void)
{
    mMCU_MODE_NORMAL;
    clr_mouse_data();

    if(adv_cfg_value&iPOWER_LOW)
        report_24g = iREPORT_125;
    else
        report_24g = ((report_dpi[key_mode]>>4)&0x03);
    m_tx_count = 0;
    drv_encoder_control(OM_ENCODER, ENCODER_CONTROL_START, NULL);

    if(true == b24g_nromal)
        ble_24g_ctrl &= ~bom6239_sync_ing;
}

/******************************************************************************
* 函数名称: rf_device_info
* 功能描述: RF在一个周期处理完成后进入休眠的处理
* 输入参数:
* 输出参数:
* 说    明: 无
******************************************************************************/
__RAM_CODE void rf_normal_mode_sleep(void)
{
    // 灯亮着时只进 IDLE(WFI, 时钟不停): STOP1 会停掉高速时钟, LEDC 的 DMA 正在发的那一帧
    // 被掐断 —— 实测每个 RF 周期后只发出前 4 颗灯(约 116us)就停了, 后面的灯收不到数据.
    // 和蓝牙 pm_checker_callback() 里 bargb_valid 时返回 IDLE 的处理一致;
    // 灯亮时整板几十 mA, IDLE 比 STOP1 多出的那点电流可以忽略. 灯灭(logo_off)后照常进 STOP1.
    OM_CRITICAL_BEGIN();
    drv_pmu_set_low_power_mode((true == bargb_valid) ? PMU_LPM_IDLE : PMU_LPM_STOP1);
    OM_CRITICAL_END();
}

/******************************************************************************
* 函数名称: rf_conn_data_process()
* 功能描述:
* 输入参数: 无
* 输出参数: 无
* 说    明: 无
******************************************************************************/
void rf_conn_data_process(void)
{
    if(0==(ble_24g_ctrl&bom6239_conn_ok))
    {
        if(mMS_TO_TICK(60*1000) > (drv_rtc_read(OM_RTC) - connect_time_count))
            return;                             // 对码驻留15s

        ble_24g_ctrl &= ~bom6239_conning;
        rf_ctrl_reg |= (brf_sync_time|brf_sync_status);
        rf_mode_convert(iRF_MODE_SYNC);
    }
    else
    {
        rf_ctrl_reg |= (bsearch_dongle|brf_sync_time|brf_sync_status);
        sync_word = iSEARCH_SYNC_WORD;
        rf_mode_convert(iRF_MODE_SYNC);
    }
}
