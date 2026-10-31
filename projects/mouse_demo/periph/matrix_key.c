/*******************************************************************************
* Copyright (c) 2012, SHENZHEN Co.,LTD.
* All rights reserved.
*
* 文件名称: matrix_key.c
* 文件描述: 矩阵处理程序
*
*******************************************************************************/
#include <string.h>
#include "om_driver.h"
#include "main.h"
#include "mouse.h"
#include "main_24g.h"

#include "matrix_key.h"
#include "hid_process.h"
#include "usb_cfg_info.h"
#include "flash_process.h"
#include "key_code_define.h"

//----------------------------------------------------------------------------;
//常量定义
//
extern void key_decode(void);

//----------------------------------------------------------------------------;
//变量定义
//
uint16_t                        key_ctrl;
uint16_t                        key_ctrl_ex;

uint8_t                         key_temp_flag;
uint8_t                         key_temp_flag_1;
uint8_t                         ukey_temp_delay;
uint8_t                         device_key;
uint8_t                         device_key_bak;

//unsigned char                   std_key_count;  // 按下按键的个数
uint8_t                         matrix_status[iMATRIX_COL_NUM];
uint8_t                         key_fn_status[iMATRIX_NUM_MAX];

uint8_t                         gKeyScan_bak[iMATRIX_COL_NUM];
uint8_t                         device_type_bak;

uint8_t                         decode_index;
uint8_t                         col_index;

uint8_t                         ctrl_key_value;
uint8_t                         std_key_value;
uint8_t                         com_ctrl_index;

uint8_t                         desk_mac_index;

//******************************************************************************
// 查表获取 数值中1的个数
unsigned char const get_1_count[256] =
{
    0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
    4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

uint8_t const key_delay_tbl[4] = {
    0x20,
    0x40,
    0x80,
    0xFE,
};

uint8_t const desk_mac[] = {
//----------------------------------------------0x110
    (iLEFT_GUI&0x0F),           iSTD_NULL,  iSTD_NULL,  iSTD_NULL, 
    (iLEFT_GUI|iLEFT_ALT)&0x0F, iSTD_NULL,  iSTD_NULL,  iSTD_NULL, 
    (iLEFT_GUI|iLEFT_ALT)&0x0F, iSTD_H,     iSTD_NULL,  iSTD_NULL, 
    (iLEFT_GUI|iLEFT_ALT)&0x0F, iSTD_H,     iSTD_M,     iSTD_NULL, 
    (iLEFT_GUI|iLEFT_ALT)&0x0F, iSTD_H,     iSTD_NULL,  iSTD_NULL, 
    (iLEFT_GUI|iLEFT_ALT)&0x0F, iSTD_NULL,  iSTD_NULL,  iSTD_NULL, 
    0x00,                       iSTD_NULL,  iSTD_NULL,  iSTD_NULL, 
};

/******************************************************************************
* 函数名称: void key_process(void)
* 功能描述:
* 输入参数:
* 输出参数:
* 说    明:
******************************************************************************/
void __attribute__((noinline)) key_process(void)
{
    uint8_t i, temp_bit;

    if(0==(key_ctrl&bwin_mac_sataus))
    {                                           // mac系统，按键交换
        temp_bit = modifier_key_tmp;
        modifier_key_tmp &= 0x33;
        if(temp_bit&(1<<7))
            modifier_key_tmp |= 0x40;
        if(temp_bit&(1<<6))
            modifier_key_tmp |= 0x80;
        if(temp_bit&(1<<2))
            modifier_key_tmp |= 0x08;
        if(temp_bit&(1<<3))
            modifier_key_tmp |= 0x04;
    }
    if(key_ctrl_ex&bcom_ctrl_en)
	{
	    if(10 > com_ctrl_index)
		{
		    tmp_std_key[0] |= ctrl_key_value;
		}
		else if(20 > com_ctrl_index)
		{
		    tmp_std_key[0] |= ctrl_key_value;
            tmp_std_key[1] = std_key_value;
		}
		else if(250 > com_ctrl_index)
		{
		    tmp_std_key[0] |= ctrl_key_value;
            if(key_ctrl_ex&bcom_ctrl_long)
                tmp_std_key[1] = std_key_value;
            else
                tmp_std_key[1] = 0;
		}
        else if(250 <= com_ctrl_index)
		{
		    tmp_std_key[0] |= ctrl_key_value;
            tmp_std_key[1] = 0;
		    if(key_ctrl_ex&bcom_ctrl_status)
				com_ctrl_index = 0;
			else
		        key_ctrl_ex &= ~bcom_ctrl_en;
		}

		com_ctrl_index++;
        if(iDEVICE_TYPE_BT2 >= device_type)
            com_ctrl_index++;
	}

    if(0 != (key_ctrl_ex&bdesk_mac))
    {
        if(0==(desk_mac_index&0x03))
        {
            i = desk_mac_index;

            tmp_std_key[0] = desk_mac[i++];
            tmp_std_key[1] = desk_mac[i++];
            tmp_std_key[2] = desk_mac[i++];
        }

        desk_mac_index++;
        if(sizeof(desk_mac) <= desk_mac_index)
            key_ctrl_ex &= ~bdesk_mac;
    }

    for(i=0; i<6; i++)
    {
        if(key_data_tmp[i] != std_key[i])
        {
            memcpy(std_key, tmp_std_key, 6);

            key_tx_count = 8;
            m_data_flag |= bstd_kb_data_send;
            return;
        }
    }

    for(i=7; i<22; i++)
    {
        if(tmp_std_key[i] != std_key[i])
        {
            memcpy(std_key+7, tmp_std_key+7, 15);
            key_ex_tx_count = 8;
            m_data_flag |= bstd_ex_data_send;
            return;
        }
    }

    if(multi_data_tmp_l != mmk_data_l)
    {
        if(0==(m_data_flag & bmmk_data_send))
        {
            mmk_data_l = multi_data_tmp_l;
            mmk_data_h = multi_data_tmp_h;
            m_data_flag |= bmmk_data_send;
            mmk_tx_count = 8;
        }
    }
}

/******************************************************************************
* 函数名称: mode_swtich_init
* 功能描述:
* 输入参数:
* 输出参数:
* 说    明:
******************************************************************************/
void mode_swtich_init(void)
{
	memset(std_key, 0x00, 22);

    mmk_data_l = 0;
    mmk_data_h = 0;

    m_data_flag |= (bmouse_data_send|bstd_kb_data_send|bmmk_data_send);
}

/******************************************************************************
* 函数名称: matrixkey_value
* 功能描述: 矩阵按键扫描
* 输入参数:
* 输出参数:
* 说    明: 为提高时间效率,赋值等操作不采用循环模式
******************************************************************************/
__RAM_CODE void matrixkey_value(void)
{
    unsigned char i, x;

    special_key = 0;

    modifier_key_tmp = 0;
    multi_data_tmp_l = 0;
    multi_data_tmp_h = 0;

    key_ctrl &= ~bkey_press_status;

    decode_index = 0;
    for(col_index=0; col_index<16; col_index++)
    {
        key_temp_flag_1 = (gKeyScan_bak[col_index] ^ matrix_status[col_index]);
        gKeyScan_bak[col_index] = matrix_status[col_index];

        key_temp_flag = gKeyScan_bak[col_index];
// 0
        if(0!=key_temp_flag)
            key_ctrl |= bkey_press_status;

        x=0x01;
        for(i=0; i<iMATRIX_ROW_NUM; i++)
        {
            if(0 == (define_key_flag_n&bkey_decode))
            {
                if(key_temp_flag_1&x)
                {                               // 按键有变化
                    goto define_key_process;
                }
                else
                {
			        if(decode_index == com_key_index)
				        com_key_index = 0xFF;
                }
            }
            else
            {
                if(key_temp_flag_1&x)
                {                               // 有变化
define_key_process:
                    if(key_temp_flag&x)
                    {
                        define_key_flag_n = (bdefine_key_press|bkey_decode);
                        key_ctrl |= bkey_press_status;

                        if(define_key_ctrl & bcom_key_break)
                        {                       // 按键可打断
                            define_key_ctrl &= ~bcom_key_break;
                            define_key_status |= bkey_status_change;
                        }
                    }
                    else
                    {
                        define_key_flag_n &= ~bdefine_key_press;
                    }
                }

                key_decode();
                if(key_ctrl&bkey_decode_valid)
                {
                    if((define_key_flag&bkey_decode_end)
			    	    && (0==(define_key_flag&bdefine_key_press)))
                    {
                        define_key_flag = 0;
                    }
                    define_key_flag_n = define_key_flag;
                    if(define_key_flag&bdefine_key_com)
                    {
                        decode_data_l_n = decode_data_l;
                        decode_data_h_n = decode_data_h;
                    }
                }
                if(0==(key_temp_flag&x))
			        key_fn_status[decode_index] = iNULL_MATRIX;
            }
            
            x<<=1;
            decode_index++;
        }
    }

    if(bkey_break_or_realse)
    {
        bkey_break_or_realse = 0;

	    memset(key_data_tmp, 0x00, 22);
    }
	else
	{
	    if((0==(key_ctrl&bkey_press_status)) && (0 == (define_key_ctrl&bcom_decode_ing)))
		{
            multi_data_tmp_l = 0;
            multi_data_tmp_l = 0;
	        memset(key_data_tmp, 0x00, 22);
		}
	}

// 按键去抖处理_END
    device_key_bak &= device_key;
    special_key_bak &= special_key;

    if(special_key&bfn_press)
    {
        key_ctrl |= bfn_key_status;
    }
    else
    {
        key_ctrl &= ~bfn_key_status;
        // Fn 和功能键全松开再存 flash: Mac 主层的 F5/F6 亮度键不经过 Fn, 也要存;
        // 长按连调过程中不存, 松手只写一次
        if(0 == special_key)
        {
            if(key_ctrl&bfn_save_status)
            {
                key_ctrl &= ~bfn_save_status;
                bsave_status = true;
            }

            if(key_ctrl&bfn_save_cfg)
            {
                key_ctrl &= ~bfn_save_cfg;
                bsave_cfg_dpi = true;
            }
        }
    }
}

uint8_t const matrixkey_c_tbl[] = {
    12, 8, 1,  2, 3, 6,  13, 4,
   18,  10,  5, 17, 7,  0,  0,  0,
};//C18为空脚

#define  DELAY

__RAM_CODE void ctrl_54e_inpull(void)
{
    OM_CRITICAL_BEGIN();
    KEY_EX_LOW;
    delayus(1);
    KEY_EX_HIGH;
    OM_CRITICAL_END();
}

void scan_key(void)
{
    MATRX_ALL_IN;
    ctrl_54e_inpull();
    delayus(10);

    MATRX_C0_LOW;
    mDELAY_R;
    matrix_status[0] = MATRIX_RAW;
    MATRX_C0_HIGH;

    MATRX_C16_LOW;
    mDELAY_R;
    matrix_status[16] = MATRIX_RAW;
    MATRX_C16_HIGH;

    MATRX_C15_LOW;
    mDELAY_R;
    matrix_status[15] = MATRIX_RAW;
    MATRX_C15_HIGH;

    MATRX_C9_LOW;
    mDELAY_R;
    matrix_status[9] = MATRIX_RAW;
    MATRX_C9_HIGH;

    MATRX_C11_LOW;
    mDELAY_R;
    matrix_status[11] = MATRIX_RAW;
    MATRX_C11_HIGH;

    MATRX_C14_LOW;
    mDELAY_R;
    matrix_status[14] = MATRIX_RAW;
    MATRX_C14_HIGH;
    mDELAY_R;

    KEY_EX_TOGGLE;
    mDELAY_EX;
    for(uint8_t i=0; i<13; i++)
    {
        mDELAY_EX;
        matrix_status[matrixkey_c_tbl[i]] = MATRIX_RAW;
        KEY_EX_TOGGLE;
    }
    KEY_EX_HIGH;

#if 0   // 矩阵调试打印, 平时关掉:
        // 每次扫键(4~8ms 一次)要发约 344 字节, 模拟串口每字节关中断约 87us, 一次扫键阻塞约 30ms;
        // 而且串口脚 UART_PIN 原来就是 P26 —— 灯珠地 NMOS 的栅极, 打印时灯珠地跟着串口数据通断, 灯会乱闪.
        // 要用时先确认 UART_PIN 不是 P26, 再改成 #if 1.
    for(uint32_t i=0; i<20; i++)
    {
        printf("MATRIX[%02d]=0x%02X\r\n", (int)i, matrix_status[i]);
    }
    printf("\r\n\r\n");
#endif
}

/******************************************************************************
* 函数名称: matrixkey_scan
* 功能描述: 矩阵按键扫描
* 输入参数:
* 输出参数:
* 说    明: 为提高时间效率,赋值等操作不采用循环模式
******************************************************************************/
__RAM_CODE void matrixkey_scan(void)
{
    unsigned char i,j,k,m,key_temp[22];

    device_key = 0;
    special_key = 0;
    key_ctrl_ex &= ~(bgost_key|bcom_ctrl_status);
    wheel_status &= ~bwheel_press;

    modifier_key_tmp = 0;
    multi_data_tmp_l = 0;
    multi_data_tmp_h = 0;

    scan_key();

    k = 0;
    for(i=0; i<iMATRIX_COL_NUM; i++)
    {
        if(0 != matrix_status[i])
        {
            key_temp[k] = matrix_status[i];
            k++;
        }
    }

    if((0!=k) && (0==(key_ctrl&bkey_press_status)))
    {
        key_ctrl |= bkey_press_status;
    }

    for(j=0; j<k; j++)
    {
        i = key_temp[j];

        if(2 <= get_1_count[i])
        {
            key_temp[j] = 0;
            for(m=0; m<k; m++)
            {
                if(0 != (key_temp[m]&i))
                {
                    key_ctrl_ex |= bgost_key;
                    return;
                }
            }
            key_temp[j] = i;
        }
    }

    matrix_status[0]  = matrix_status[0]              
                      | ((matrix_status[9]&0x01)<<4)  
                      | ((matrix_status[9]&0x08)<<2)  
                      | (matrix_status[9]&0xC0);      
    matrix_status[15] = matrix_status[15]
                      | ((matrix_status[16]&0x10)<<2)
                      | ((matrix_status[17]&0x20)<<2);
    matrix_status[9]  = matrix_status[17] | ((matrix_status[9]&0x20)<<2);

    matrixkey_value();
    key_process();
}

extern void                 hi5_usb_init(void);
extern void                 usb_disconnect(void);

extern bool                 busb_valid;

//-----------------------------------------------------------------------------
// 插电识别: 区分电脑(USB 主机)和充电头
//
// 主机 D+/D- 各有 15K 下拉; 充电头 D+/D- 悬空、短接或者接分压电阻, 没有下拉.
// 把 D+/D- 临时切成 150K 内部上拉: 两根都读到低 = 主机, 否则 = 充电头.
// 分压电压偏低的充电头可能被当成主机, 所以进了 USB 模式还要看枚举: 3 秒内没有 SOF 就当充电头退回无线模式.
//
#define RSVD1_USB_CHARGER           (1u<<30)    // USB 模式枚举超时判成充电头, 带过复位: 复位后不再识别, 直接按充电处理

#define iVBUS_OFF                   0           // 没插电
#define iVBUS_DEBOUNCE              1           // 刚插电, 等 D+/D- 接触稳定再识别
#define iVBUS_CHARGER               2           // 充电头: 保持当前模式
#define iVBUS_HOST                  3           // 主机: USB 模式
#define iVBUS_ENUM_START            4           // 上电识别是主机, 进 USB 模式后开始等枚举
#define iVBUS_ENUM_WAIT             5           // 等 SOF

static uint8_t                  vbus_state;
static uint32_t                 vbus_rtc;

/******************************************************************************
* 函数名称: usb_host_detect()
* 功能描述: 识别插的是不是 USB 主机
* 输入参数: 无
* 输出参数: true 主机, false 充电头
* 说    明: 只在没开 USB 控制器(无线模式/上电)时调用, D+/D- 平时是下拉, 测完恢复下拉.
******************************************************************************/
static bool usb_host_detect(void)
{
    uint32_t buf_en = OM_PMU->PAD_BUF_EN;
    bool     host;

    OM_PMU->PAD_BUF_EN |= ((1<<USB_DN_PIN)|(1<<USB_DP_PIN));
    drv_pmu_pin_mode_set(USB_DN_PIN, PMU_PIN_MODE_PU);
    drv_pmu_pin_mode_set(USB_DP_PIN, PMU_PIN_MODE_PU);
    delayus(1000);                              // 150K 上拉给线上电容充电

    host = (0 == (OM_GPIO0->DATA & ((1<<USB_DN_PIN)|(1<<USB_DP_PIN))));

    drv_pmu_pin_mode_set(USB_DN_PIN, PMU_PIN_MODE_PD);
    drv_pmu_pin_mode_set(USB_DP_PIN, PMU_PIN_MODE_PD);
    OM_PMU->PAD_BUF_EN = buf_en;

    return host;
}

/******************************************************************************
* 函数名称: scan_mode_swtich()
* 功能描述: 模式切换
* 输入参数: type 上电检测 iMODE_CHECK_POWERUP, 其余为运行中检测
* 输出参数: 无
* 说    明: 插电时区分主机和充电头: 插的是主机就是 USB 模式; 充电头或没插电是无线模式, 看 P36(MODE_PIN):
*           低 = 蓝牙(用上次的蓝牙通道), 高 = 2.4G.
*           上电时直接按检测结果定 device_type; 运行中和当前模式不一样就复位, 复位后
*           上电流程再按新模式起来. 运行中的 iMODE_CHECK_DELAY / NO_DELAY 一样处理.
******************************************************************************/
void scan_mode_swtich(uint8_t type)
{
    device_type_t dev;
    uint32_t      now;

    // 通道号是从 flash 读的(2 位, 可能读出 3), 读出 3 会把蓝牙当成 2.4G
    if(iDEVICE_TYPE_BT2 < bt_index)
        bt_index = iDEVICE_TYPE_BT0;

    if(iMODE_CHECK_POWERUP == type)
    {
        if(0 == USB_5V_PIN)
            vbus_state = iVBUS_OFF;
        else if(OM_PMU->SW_RSVD1 & RSVD1_USB_CHARGER)
            vbus_state = iVBUS_CHARGER;
        else if(usb_host_detect())
            vbus_state = iVBUS_ENUM_START;
        else
            vbus_state = iVBUS_OFF;             // 刚插上 D+/D- 可能还没接触好, 运行中消抖后再识别一次
        OM_PMU->SW_RSVD1 &= ~RSVD1_USB_CHARGER;

        if(iVBUS_ENUM_START == vbus_state)
            device_type = iDEVICE_TYPE_USB;
        else if(0 == MODE_DATA)
            device_type = bt_index;
        else
            device_type = iDEVICE_TYPE_24G;
        return;
    }

    now = drv_rtc_read(OM_RTC);
    if(0 == USB_5V_PIN)
    {
        vbus_state = iVBUS_OFF;
    }
    else
    {
        switch(vbus_state)
        {
            case iVBUS_OFF:
                vbus_state = iVBUS_DEBOUNCE;
                vbus_rtc = now;
                break;
            case iVBUS_DEBOUNCE:
                if((iDEVICE_TYPE_USB != device_type)
                    && (mMS_TO_TICK(200) < (now - vbus_rtc)))
                    vbus_state = usb_host_detect() ? iVBUS_HOST : iVBUS_CHARGER;
                break;
            case iVBUS_ENUM_START:
                vbus_state = iVBUS_ENUM_WAIT;
                vbus_rtc = now;
                break;
            case iVBUS_ENUM_WAIT:
                if(true == busb_valid)
                    vbus_state = iVBUS_HOST;
                else if(mMS_TO_TICK(3000) < (now - vbus_rtc))
                {                               // 没枚举: 是充电头, 退回无线模式
                    vbus_state = iVBUS_CHARGER;
                    OM_PMU->SW_RSVD1 |= RSVD1_USB_CHARGER;
                }
                break;
            default:
                break;
        }
    }

    if((iVBUS_HOST == vbus_state) || (iVBUS_ENUM_START == vbus_state) || (iVBUS_ENUM_WAIT == vbus_state))
        dev = iDEVICE_TYPE_USB;
    else if(0 == MODE_DATA)                     // 充电头/没插电/还在识别: 无线模式
        dev = bt_index;
    else
        dev = iDEVICE_TYPE_24G;

    if(dev == device_type)
        return;

    if(iDEVICE_TYPE_BT2 >= dev)
    {                                           // 蓝牙通道号写进 SW_RSVD1 带过复位
        OM_PMU->SW_RSVD1 &= ~PMU_SW_RSVD1_RF_MDOE_MASK;
        OM_PMU->SW_RSVD1 |= dev;
    }
    NVIC_SystemReset();
}

