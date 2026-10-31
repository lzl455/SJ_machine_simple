/*******************************************************************************
* Copyright (c) 2013, SHENZHEN Co.,LTD.
* All rights reserved.
*
* 文件名称: led.c
*
* 功能说明:
* 修改说明:
*******************************************************************************/
#include <string.h>
#include "om_driver.h"

#include "main.h"
#include "lvd.h"
#include "led.h"

#include "flash.h"
#include "flash_process.h"
#include "usb_cfg_info.h"
#include "matrix_key.h"
#include "time_delay.h"
#include "exled.h"
#include "led_logo_tbl.h"                       // led_single_table / key_led_map 的定义, 只在这里包含
//-----------------------------------------------------------------------------;
// 常量定义区
//


//-----------------------------------------------------------------------------;
// 变量定义区
//
bool                            bdevice_led_status;
bool                            bled_bright_en;
bool                            bargb_valid;

uint8_t                         device_conn_index=0;
uint8_t                         device_hint_count;
uint32_t                        led_delay_count;
uint8_t                         hint_time_count;

// 背光只有纯白(RGB 灯效已移除): 灯效号 0 = 关, 其余都是整板白.
// 灯效号 / 速度 / 颜色还留着: 上位机协议、按键、flash 记录都还读写它们, 只是不再影响画面
uint8_t                         led_mode_bak;
uint8_t                         led_cur_mode;
enum speed_level                led_speed_lev;
unsigned char                   led_rgb_index;
enum led_level                  led_light_lev;

__ALIGNED(4) uint8_t            pwm_rgb_tbl[iLED_MAX][3];
__ALIGNED(4) uint8_t            pwm_out[iLED_NUM_MAX*3];          // logo_flush() 写, LEDC 的 DMA 直接从这里读(无影子缓冲)

extern bool                     b24g_nromal;
extern bool                     busb_valid;

#define iBOOST_SOFT_START_MS    4               // ME2169软启动2ms(典型，规格书无最大值)，留余量取4ms
static uint32_t                 boost_on_tick;

#define iLOGO_RENDER_MS         7               // 渲染节拍门限, 各模式实际落在 7.5~8ms 一帧
static uint32_t                 logo_render_tick;

//=============================================================================;
// 代码区                                                                       ;
//=============================================================================;

/*******************************************************************************
* 函数名称: ledc_init()
* 功能描述:
* 输入参数： 无
* 输出参数： 无
* 说    明:
*******************************************************************************/
void ledc_init(void)
{
    // 单位1/48us，实际为(值+1)个周期。按3528幻彩RGB规格书"控制器发送码型要求"取典型值:
    //   T0H: 0.25(典型)~0.47(最大)us -> 16 => 17/48 = 0.354us
    //   T1H: 0.58(最小)~1.0(最大)us, 典型0.85 -> 40 => 41/48 = 0.854us
    //   码元周期 T0H+T1H = 1.208us >= 1.2us(最小)
    // 原值14/28: T1H=0.604us只比下限高0.024us，多颗级联容易误码
    // Reset>=80us 由帧间空闲低电平保证(写周期>=7.5ms，20灯一帧约0.6ms)
    // ponytail: 码型按"0码=短高+长低、1码=长高+短低"推算，需示波器实测P01确认
    ledc_config_t config = {
        .reset_cycles         = 100,
        .long_pulse_cycles    = 44,
        .short_pulse_cycles   = 14,
    };
    drv_ledc_init(OM_LEDC, &config);
    bargb_valid = true;
}

/*******************************************************************************
* 函数名称: device_led_on()
* 功能描述:
* 输入参数： 无
* 输出参数： 无
* 说    明:
*******************************************************************************/
void device_led_on(void)
{
    bdevice_led_status = true;

    hint_led_status &= 0xF8;
    if(iDEVICE_TYPE_BT0 == device_type)
        hint_led_status |= bled_bt0;
    else if(iDEVICE_TYPE_BT1 == device_type)
        hint_led_status |= bled_bt1;
    else if(iDEVICE_TYPE_24G == device_type)
        hint_led_status |= bled_24g;
}

/*******************************************************************************
* 函数名称: device_led_off()
* 功能描述:
* 输入参数： 无
* 输出参数： 无
* 说    明:
*******************************************************************************/
void device_led_off(void)
{
    bdevice_led_status = false;

    hint_led_status &= 0xF8;
}

void logo_off(void)
{
    bargb_valid = false;

    DRV_RCC_CLOCK_ENABLE(RCC_CLK_LEDC, 0U);
    OM_GPIO0->DATAOUTSET = (1<<ARGB_OUT_PIN);
    OM_GPIO0->DATAOUTCLR = (1<<ARGB_OFF_PIN);   // P26 低: 断开灯珠地(NMOS 关)
    OM_GPIO0->DATAOUTCLR = (1<<BOOST_EN_PIN);   // 先断灯珠地再关升压，ME2169 EN=L时关断电流<1nA
}

/* 开/关背光: FN 切灯效键、开关灯键、滚轮背光模式短按共用, 存 flash 由调用方处理.
   打开时灯效号若是 0(上位机设的"关")就改回白, 不然开了也不亮 */
void led_backlight_toggle(void)
{
    bled_bright_en = !bled_bright_en;
    if((true == bled_bright_en) && (iMODE_LED_OFF == led_cur_mode))
        led_cur_mode = iMODE_LED_ON;
}


//-----------------------------------------------------------------------------;
// 整板限流(移植自 K96)
//
// 统计整帧要发出去的占空比之和, 超预算就整体等比例压暗, 满屏全亮时到不了最高亮度,
// 但画面里看不出压缩痕迹.
//
// 预算按 SJ 的器件参数定: 3528 灯珠每通道恒流 2.5mA(占空比 255 时), 每颗静态 0.5mA.
//   LED 电流(5V侧) ≈ 预算/255*2.5mA + 灯珠数*0.5mA
//   22000 -> 约 216mA + 104颗*0.5mA = 268mA@5V, 经 ME2169 升压(按效率 85%)折到 USB
//   4.6V 输入约 0.34A, 不装电池只插 USB 时也在 500mA 以内, 给 MCU 和 4056 留余量.
// 104 颗全白满亮(24 级) = 104*3*255 = 79560, 会触发, 整板压到约 0.28 倍. 限流按 24 级算,
// 各级在限流后怎么分配见 led_level_max().
#define iLED_CURRENT_BUDGET     (22000)

/* 限流系数用 8 位小数, 负载轻微起伏时不会让整板一档一档地跳 */
#define iLED_SCALE_ONE          (256)

/* 限流系数"快压慢放": 负载变大立刻压暗(保护供电), 负载变小先保持约 1 秒再慢慢放亮.
   原来每帧按当帧负载重算, 繁星/斜风细雨/漫天飞雪这类亮灯数一直在变的灯效, 一超预算
   整板亮度就跟着负载逐帧起伏(斜风细雨亮 8 条和 9 条对角线差 12%), 看起来就是整板在抖.
   周期性的负载峰值只要间隔小于保持时间, 系数就一直停在最低点不动. */
#define iLED_SCALE_HOLD         (128)                   /* 保持帧数, 约 1 秒 */
#define iLED_SCALE_RELEASE      (1)                     /* 每帧放亮 1/256, 从 0.5 回到 1 约 1 秒 */

static uint16_t                 led_scale = iLED_SCALE_ONE;
static uint8_t                  led_scale_hold;

static void led_current_limit(void)
{
    uint32_t load = 0;
    uint16_t i, target;

    for(i=0; i<iLED_MAX; i++)
    {
        if((uint8_t)255 == led_single_table[i])         /* 没接灯珠的位置不算 */
            continue;

        load += pwm_rgb_tbl[i][0];
        load += pwm_rgb_tbl[i][1];
        load += pwm_rgb_tbl[i][2];
    }

    /* 按 24 级(iPWM_MAX -> 255, 约 x4)折算成要发出去的占空比总和; 低几级的分配在 led_level_max() 里 */
    load <<= 2;

    if(iLED_CURRENT_BUDGET >= load)
        target = iLED_SCALE_ONE;
    else
        target = (uint16_t)(((uint32_t)iLED_CURRENT_BUDGET*iLED_SCALE_ONE)/load);

    if(target <= led_scale)
    {
        led_scale = target;                             /* 快压: 不超预算 */
        led_scale_hold = iLED_SCALE_HOLD;
    }
    else if(0 != led_scale_hold)
    {
        led_scale_hold--;                               /* 保持: 吸收负载起伏 */
    }
    else
    {
        led_scale += iLED_SCALE_RELEASE;                /* 慢放 */
        if(led_scale > target)
            led_scale = target;
    }
}

/* 亮度表: 通道值为 iPWM_MAX 时各级发给灯珠的字节.
   1~24 级按人眼明度(CIE L*)等分, 每级看起来变化一样大(约 3.9 个 L*), 低几级相对变化大
   (3->5->7), 高几级相对变化小(207->230->255). 18 级 130 约等于原来 3 档, 24 级 = 原来 4 档.
   1 级 3(约 1.2% 占空比) 是下限: 再暗的话 0~64 的通道值只剩两三级明暗, 灯效看不出样子.
   生成: L*(k) = L0 + (100-L0)*(k-1)/23, L0 = L*(3/255) = 10.4, 输出 = 255*Y(L*), 四舍五入.
   满屏白时(限流后最亮约 69) 1~24 级用这张表也都分得开, 改表后要重跑主机测试确认. */
static uint8_t          const   led_lev_tbl[iLED_LEV_MAX+1]={
      0,   3,   5,   7,   9,  12,  16,  20,  25,  31,  38,  46,
     54,  64,  75,  87, 100, 114, 130, 147, 165, 185, 207, 230, 255,
};

/* 6 档在 0~24 级上的位置, 大致等分(亮度级按明度等分, 所以 6 档看起来也是一样大的台阶) */
static uint8_t          const   lev6_tbl[]={0, 5, 10, 14, 19, 24};
typedef char lev6_check[((iLED_LEV_MAX == 24) && (iSPEED_LEV_MAX == 24)
                         && (sizeof(lev6_tbl) == 6)) ? 1 : -1];

/* 从 lev 往上(up)/往下调一档; 已经在头/尾就原样返回, 调用处据此判断到头 */
uint8_t lev6_next(uint8_t lev, bool up)
{
    uint8_t i;

    if(up)
    {
        for(i=0; i<sizeof(lev6_tbl); i++)
            if(lev6_tbl[i] > lev)
                return lev6_tbl[i];
        return lev6_tbl[sizeof(lev6_tbl)-1];
    }

    for(i=sizeof(lev6_tbl); 0<i; i--)
        if(lev6_tbl[i-1] < lev)
            return lev6_tbl[i-1];
    return lev6_tbl[0];
}

static uint16_t                 led_out_max;    /* 本帧通道值 iPWM_MAX 发出去的字节, 8 位小数 */

/* 按带 8 位小数的级数查亮度表, 相邻两级之间线性插值 */
static uint16_t led_lev_at(uint16_t x)
{
    uint8_t i = (uint8_t)(x>>8);

    if(iLED_LEV_MAX <= i)
        return (uint16_t)led_lev_tbl[iLED_LEV_MAX]<<8;

    return (uint16_t)(((uint16_t)led_lev_tbl[i]<<8) + (led_lev_tbl[i+1]-led_lev_tbl[i])*(x&0xFF));
}

/* 算本帧当前亮度级的上限 led_out_max.
   限流后 24 级最亮只能到 top, 1 级不动, 中间各级在 1 级和 top 之间按亮度表(明度)均匀分布.
   不这样的话, 满屏白这种被限流压住的灯效 14 级往上都一样亮(都被压到 top), 调了没反应;
   单色/流光这类不触发限流的灯效 top=255, 各级就是亮度表本身. */
static void led_level_max(void)
{
    uint16_t top, x;
    uint8_t  i, lev;

    top = (uint16_t)(led_lev_tbl[iLED_LEV_MAX]*led_scale);         /* 8 位小数 */

    /* top 落在亮度表第几级(8 位小数) */
    for(i=iLED_LEV_MAX; (1<i) && (top < ((uint16_t)led_lev_tbl[i]<<8)); i--)
        ;
    x = (uint16_t)i<<8;
    if((iLED_LEV_MAX > i) && (top > ((uint16_t)led_lev_tbl[i]<<8)))
        x += (top - ((uint16_t)led_lev_tbl[i]<<8))/(led_lev_tbl[i+1]-led_lev_tbl[i]);

    lev = led_light_lev;
    if(iLED_LEV_MAX < lev)
        lev = iLED_LEV_MAX;

    /* 当前级按比例落到 1 级 ~ top 之间 */
    if(iLED_LEV_MIN == lev)
        x = 0;
    else
        x = (uint16_t)(256 + ((uint32_t)(lev-1)*(x-256))/(iLED_LEV_MAX-1));

    led_out_max = led_lev_at(x);
    if(led_out_max > top)                               /* top 比 1 级还暗时兜底, 这个预算下不会出现 */
        led_out_max = top;
}

/* 通道值折算成实际发给灯珠的字节.
   通道值正常是 0~iPWM_MAX; 海浪拍打的浪头闪光会到 2*iPWM_MAX, 借用亮度级没用满的那段 PWM,
   所以要封顶 —— 不封的话高几级会溢出, 浪头正中反而是灭的 */
static uint8_t led_bright(uint8_t value)
{
    uint32_t out;

    out = ((uint32_t)value*led_out_max)/((uint32_t)iPWM_MAX<<8);
    if(0xFF < out)
        return 0xFF;

    return (uint8_t)out;
}

/******************************************************************************
* 函数名称: logo_flush
* 功能描述: 把网格上渲染好的 pwm_rgb_tbl 限流、调亮度后按灯带顺序写进 pwm_out
* 输入参数: 无
* 输出参数: 无
* 说    明: pwm_rgb_tbl 每行已是 GRB 顺序(iG_INDEX=0, iR_INDEX=1, iB_INDEX=2),
*           和 3528 灯珠"高位先发, GRB 顺序"一致, 原样搬过去即可.
*           LEDC 的 DMA 直接读 pwm_out(没有影子缓冲), 发送由 drv_ledc_write() 在
*           USB 主循环/蓝牙扫描/2.4G RTC 中断里调用, 约束见 drv_ledc.c.
******************************************************************************/
/* 模式键灯: 下标是 device_type, 蓝牙 1/2/3 = Q/W/E 键(闪蓝), 2.4G = R 键(闪绿); 值是 key_led_map 里这几个键下的灯珠号 */
static uint8_t          const   mode_key_led[iDEVICE_TYPE_24G+1]={38, 39, 40, 41};

/* 模式键灯在闪: 无线模式下对码快闪 / 回连慢闪 / 模式提示, 节奏由 device_led_step 定, bdevice_led_status 是当前亮灭 */
static bool mode_key_hint(void)
{
    return (iDEVICE_TYPE_24G >= device_type) && (0 != (hint_led_status&bled_hint_mode));
}

/* 背光黑着(回连 / 低电 / 背光关)时要不要给灯珠供电: 空格键电量灯(低电红闪的亮拍 / 充电绿 / 充满蓝)
   或模式键灯正亮着才供电; 灭的那一拍整条断电, 省下 104 颗灯珠约 50mA 的静态电流和升压损耗 */
static bool hint_led_lit(void)
{
    return (0 != (hint_led_status&(bled_low|bled_chaing|bled_chaok)))
        || (mode_key_hint() && (true == bdevice_led_status));
}

void logo_flush(void)
{
    uint16_t i, pos;

    led_current_limit();
    led_level_max();

    for(i=0; i<iLED_MAX; i++)
    {
        if((uint8_t)255 == led_single_table[i])
            continue;

        pos = (uint16_t)led_single_table[i]*3;
        pwm_out[pos+0] = led_bright(pwm_rgb_tbl[i][0]);
        pwm_out[pos+1] = led_bright(pwm_rgb_tbl[i][1]);
        pwm_out[pos+2] = led_bright(pwm_rgb_tbl[i][2]);
    }

    // 空格键(灯珠 100)电量灯: 低电红闪 / 充电绿 / 充满蓝, 满亮盖过背光
    switch(hint_led_status&0x70)
    {
        case bled_low:
            pwm_out[300] = 0;
            pwm_out[301] = 255;
            pwm_out[302] = 0;
            break;
        case bled_chaing:
            pwm_out[300] = 255;
            pwm_out[301] = 0;
            pwm_out[302] = 0;
            break;
        case bled_chaok:
            pwm_out[300] = 0;
            pwm_out[301] = 0;
            pwm_out[302] = 255;
            break;
    }

    // 模式键灯盖过背光: 亮时蓝牙蓝、2.4G 绿, 满亮(和上面的电量灯一样不走亮度档); 灭时全黑, 闪起来才看得出
    if(mode_key_hint())
    {
        pos = (uint16_t)mode_key_led[device_type]*3;
        memset(pwm_out+pos, 0x00, 3);
        if(true == bdevice_led_status)
            pwm_out[pos + ((iDEVICE_TYPE_24G == device_type) ? iG_INDEX : iB_INDEX)] = 255;
    }
}

extern void key_into_fcc_test(void);

/******************************************************************************
* 函数名称: device_led_step
* 功能描述: 模式提示 / 对码快闪 / 回连慢闪的状态和亮灭节奏
* 输入参数:
* 输出参数:
* 说    明:
******************************************************************************/
static void device_led_step(void)
{
//-----------------------------------------------------------------------------
// 模式提示
//
    if(0 != hint_time_count)
    {
        hint_led_status |= bled_hint_mode;
        if(0 != (hint_time_count&0x08))
        {
            bdevice_led_status = true;
            hint_led_status |= (bled_hint_mode|bled_24g|bled_bt1|bled_bt0);
        }
        else
        {
            device_led_off();
        }

        hint_time_count--;
        if(2>hint_time_count)
        {
            hint_time_count = 0;
            hint_led_status = 0x00;
        }

        return;
    }

    rtc_cur_value = drv_rtc_read(OM_RTC);
    if(iDEVICE_TYPE_USB != device_type)
    {
        if(ble_24g_ctrl & bom6239_conning)
        {
            hint_led_status |= bled_hint_mode;
            if(0 == device_conn_index)
            {
                device_conn_index++;
                led_delay_count = rtc_cur_value;
            }
            else if(1 == device_conn_index)
            {
                if(ble_24g_ctrl&bom6239_conn_ok)
                {
                    device_conn_index++;
                    led_delay_count = rtc_cur_value;
                    device_hint_count = 0;
                }

                key_into_fcc_test();
                if(false == bdevice_led_status)
                {
                    device_led_off();
                    if(mMS_TO_TICK(250) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = true;
                        led_delay_count = rtc_cur_value;
                    }
                }
                else
                {
                    device_led_on();
                    if(mMS_TO_TICK(250) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = false;
                        led_delay_count = rtc_cur_value;
                    }
                }
            }
            else if(2 == device_conn_index)
            {
                device_conn_index = 0;
                ble_24g_ctrl &= ~bom6239_conning;
            }
            return;
        }
        else if(ble_24g_ctrl&bom6239_sync_ing)
        {
            if(true == b24g_nromal)
                return;

            hint_led_status |= bled_hint_mode;
            if(0 == device_conn_index)
            {
                device_conn_index++;
                led_delay_count = rtc_cur_value;
            }
            else if(1 == device_conn_index)
            {
                if(ble_24g_ctrl&bom6239_sync_ok)
                {
                    device_conn_index++;
                    led_delay_count = rtc_cur_value;
                    device_hint_count = 0;
                }

                if(false == bdevice_led_status)
                {
                    device_led_off();
                    if(mMS_TO_TICK(1000) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = true;
                        led_delay_count = rtc_cur_value;
                    }
                }
                else
                {
                    device_led_on();
                    if(mMS_TO_TICK(1000) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = false;
                        led_delay_count = rtc_cur_value;
                    }
                }
            }
            else if(2 == device_conn_index)
            {
                device_conn_index = 0;
                ble_24g_ctrl &= ~bom6239_sync_ing;
            }
            return;
        }
        hint_led_status &= ~bled_hint_mode;
        device_conn_index = 0;
    }
    else
    {
        hint_led_status &= ~bled_hint_mode;
        if(0 != (usb_ctrl_flag&busb_suspend_resume))
            return;
    }
}

/* 指示灯节拍(约 15ms): 先算模式键灯的状态和亮灭, 再把小板指示灯(Num/Caps/USB/Win 锁)的变化当拍发出 */
void device_led_process(void)
{
    device_led_step();
    exled_process();
}

/******************************************************************************
* 函数名称: logo_process
* 功能描述: 背光渲染(含上电时序, 空格键电量灯 / 模式键灯叠加)
* 输入参数:
* 输出参数: true = 这次渲染出了新的一帧(在 pwm_out 里), 调用方可以紧接着 drv_ledc_write() 发出去
* 说    明:
******************************************************************************/
bool logo_process(void)
{
    bool    dark;

    if(true==bsave_status)
    {
        bsave_status = false;
        flash_save_status();
    }
    if(true==bsave_cfg_dpi)
    {
        bsave_cfg_dpi = false;
        flash_save_cfg_dpi();
    }
    if(iDEVICE_TYPE_24G == device_type)
    {
        if(true==bflash_ctrl)
            return false;
    }

    if(iDEVICE_TYPE_USB == device_type)
    {
        // 只有和电脑枚举过(收到过SOF)才按USB挂起关灯；接充电器/无主机时没有SOF，
        // 5s超时会误判为挂起，导致只插USB时灯不亮
        if((true == busb_valid) && (0 != (usb_ctrl_flag&busb_suspend_resume)))
        {
            logo_off();
            return false;
        }
    }

    // 回连、低电(lvd.c 置 battery_status)、背光关着时整板不亮; 这时只在空格键电量灯或模式键灯
    // 正亮着的那一拍给灯珠供电, 整板黑着只亮这个键(logo_flush 里叠加), 灭的那一拍断电
    dark = ((iDEVICE_TYPE_USB != device_type) && (0 != (ble_24g_ctrl&bom6239_sync_ing)))
        || (0 != battery_status)
        || (false == bled_bright_en) || (iLED_LEV_MIN == led_light_lev) || (iMODE_LED_OFF == led_cur_mode);
    if(dark && (false == hint_led_lit()))
    {
        logo_off();
        return false;
    }

    if(false==bargb_valid)
    {
        // 上电顺序: 先开升压 -> 等软启动完成 -> 再接通灯珠地(P26) -> 发数据
        // ME2169: EN高有效，软启动2ms(典型)；未升压时5VDD≈VBAT-0.4V(经L1+SS54直通)，
        // 可能低于3528灯珠最低工作电压3.5V，所以等5VDD稳定后再带灯珠负载。非阻塞等待，不影响RF时序
        if(0 == (OM_GPIO0->DATAOUT & (1<<BOOST_EN_PIN)))
        {
            OM_GPIO0->DATAOUTSET = (1<<BOOST_EN_PIN);
            boost_on_tick = drv_rtc_read(OM_RTC);
            return false;
        }
        if(mMS_TO_TICK(iBOOST_SOFT_START_MS) > (drv_rtc_read(OM_RTC) - boost_on_tick))
            return false;

        OM_GPIO0->DATAOUTSET = (1<<ARGB_OFF_PIN);   // P26 高: 接通灯珠地(NMOS 开), 升压已稳定
        ledc_init();
        OM_GPIO0->DATAOUTCLR = (1<<ARGB_OUT_PIN);
    }

    if(mMS_TO_TICK(iLOGO_RENDER_MS) > (drv_rtc_read(OM_RTC) - logo_render_tick))
        return false;
    logo_render_tick = drv_rtc_read(OM_RTC);

    // 背光只有纯白(RGB 灯效已移除); 回连/低电/背光关/灯效号 0(dark)时整板黑,
    // 只剩 logo_flush 叠上去的指示键. 亮度档和整板限流在 logo_flush 里照常处理
    memset(pwm_rgb_tbl, dark ? 0 : iPWM_MAX, iLED_MAX*3);
    logo_flush();
    return true;
}
