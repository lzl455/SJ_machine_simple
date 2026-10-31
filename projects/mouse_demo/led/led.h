/*******************************************************************************
* Copyright (c) 2013, SHENZHEN Co.,LTD.
* All rights reserved.
*
* 文件名称: led.h
*
* 功能说明:
* 修改说明:
*******************************************************************************/
#ifndef _LED_H_
#define _LED_H_

//=============================================================================;
// 配置引用区                                                                  ;
//=============================================================================;

//-----------------------------------------------------------------------------;
// 配置区
//
#define iPWM_MAX                64              // 网格上的满亮值, logo_flush 再按亮度档折算到 0~255
#define iRGB_QICAI              7               // 颜色档"七彩"(flash 出厂默认值用; RGB 灯效已移除, 颜色档不再影响画面)

#define iR_INDEX                0x01
#define iG_INDEX                0x00
#define iB_INDEX                0x02

//-----------------------------------------------------------------------------;
// 配置区
//
// 背光在"行 x 列"网格上处理(整板限流按网格算), 再经 led_single_table 映射到灯带序号.
// SJ 大板 104 颗, 按 PCB 上的位置排成 6 行 x 21 列, 映射表见 led_logo_tbl.h.
#define iLED_COL_NUM            (21)
#define iLED_ROW_NUM            (6)
#define iLED_NUM_MAX         	(104)                                  // 实际灯珠数(灯带长度)
#define iLED_MAX         	    (iLED_COL_NUM*iLED_ROW_NUM)             // 网格位置数 126

#if (iLED_NUM_MAX > iLED_MAX)
#error "灯珠数不能多于网格位置数"
#endif

// 亮度 0~24 共 25 级: 0 级关背光, 1~24 级按人眼明度等分(每级看起来变化一样大),
// 18/24 级约等于原来的 3/4 档. 换算表见 led.c 的 led_lev_tbl[]
enum led_level
{
    iLED_LEV_MIN = 0,           // 关背光
    iLED_LEV_DEF = 19,          // 出厂默认, 6 档里的第 5 档(原来 18 级, 对到档位上)
    iLED_LEV_MAX = 24,          // 最亮
};

// 速度 0~24 共 25 级, 0 级最快. RGB 灯效已移除, 速度只剩上位机/按键/flash 读写, 不影响画面
enum speed_level
{
    iSPEED_LEV_MIN = 0,         // 最快
    iSPEED_LEV_DEF = 5,         // 出厂默认, 6 档里的第 2 档(原来 6 级, 对到档位上)
    iSPEED_LEV_MAX = 24,        // 最慢
};

// 按键和滚轮只在 6 档之间调: 0/5/10/14/19/24 级, 亮度和速度共用(亮度 0 档 = 关背光), 内部刻度仍是 0~24.
// 上位机或老 flash 设的级可能落在两档之间, 调的时候按方向对齐到下一档.
extern uint8_t lev6_next(uint8_t lev, bool up);

// 原来的 0~4 档 <-> 现在的 0~24 级. 上位机协议和老 flash 记录都还是 3 位的 0~4 档.
// 级 -> 档向上取整, 亮着的 1~6 级报 1 档, 不会报成关.
#define iLEV_PER_OLD            (6)
#define mLEV_TO_OLD(lev)        (((lev)+iLEV_PER_OLD-1)/iLEV_PER_OLD)

// 灯效号: RGB 灯效已移除, 背光只有纯白 —— 0 = 关, 1~15 都是整板白(亮度 6 档、限流、电量/模式指示照常).
// 枚举值不能随意动: 上位机协议和 flash 里存的都是这个数字, 按键/滚轮切灯效仍在 1~15 之间循环.
typedef enum {
    iMODE_LED_OFF       =0,                     // 关
    iMODE_LED_ON        =1,                     // 白(出厂默认)
    iMODE_LED_MAX       =15,                    // 1~15 都是白
    iMODE_LED_INVALID   =16,
    iLOGO_INVALID   = 0x0F,
} logo_mode_t;

//=============================================================================;
// 外部定义内部引用区                                                           ;
//=============================================================================;

//-----------------------------------------------------------------------------;
// 外部寄存器引用
//

//-----------------------------------------------------------------------------;
// 外部函数引用
//

//=============================================================================;
// 内部定义外部引用区                                                          ;
//=============================================================================;

//-----------------------------------------------------------------------------;
// 寄存器引用
//
extern bool                     bled_bright_en;
extern bool                     bargb_valid;

extern uint8_t                  led_cur_mode;
extern uint8_t                  led_mode_bak;
extern enum speed_level         led_speed_lev;
extern unsigned char            led_rgb_index;
extern enum led_level           led_light_lev;

extern uint8_t                  hint_time_count;

extern uint8_t                  pwm_rgb_tbl[iLED_MAX][3];             // 网格上的渲染结果, 0~iPWM_MAX
extern uint8_t                  pwm_out[iLED_NUM_MAX*3];              // 按灯带顺序、限流和亮度档折算后的输出, drv_ledc_write() 从这里取

extern unsigned char    const   led_single_table[];


//-----------------------------------------------------------------------------;
// 函数引用
//
extern void ledc_init(void);

extern void logo_off(void);
extern void device_led_on(void);
extern void device_led_off(void);

extern void device_led_process(void);
extern bool logo_process(void);          // true = 渲染出了新的一帧, 可以紧接着 drv_ledc_write()

extern void logo_flush(void);
extern void led_backlight_toggle(void);
//
//-----------------------------------------------------------------------------;
#endif