/* ----------------------------------------------------------------------------
 * Copyright (c) 2020-2030 OnMicro Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of OnMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------- */

/**
 * @file     drv_ledc.c
 * @brief    ledc driver
 * @date     23. May 2024
 * @author   OnMicro SW Team
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "RTE_driver.h"
#if (RTE_LEDC)
#include <stddef.h>
#include "drv_ledc.h"
#include "led.h"
#include "main.h"

/*******************************************************************************
 * TYPEDEFS
 */
#if (RTE_LEDC_REGISTER_CALLBACK)
typedef struct {
    drv_isr_callback_t    isr_cb;
} ledc_env_t;
#endif /* (RTE_LEDC_REGISTER_CALLBACK) */


/*******************************************************************************
 * CONST & VARIABLES
 */
#if (RTE_LEDC_REGISTER_CALLBACK)
static ledc_env_t ledc_env = {
    .isr_cb       = NULL,
};
#endif /* (RTE_LEDC_REGISTER_CALLBACK) */


/*******************************************************************************
 * PUBLIC FUNCTIONS
 */
om_error_t drv_ledc_init(OM_LEDC_Type *om_ledc, ledc_config_t *config)
{
    if ((om_ledc == NULL) || (config == NULL)) {
        return OM_ERROR_PARAMETER;
    }

    drv_pmu_ledc_sel(PMU_LEDC_SEL_IO_01);

    DRV_RCC_CLOCK_ENABLE(RCC_CLK_LEDC, 1U);
    NVIC_ClearPendingIRQ(LEDC_IRQn);
    NVIC_SetPriority(LEDC_IRQn, RTE_LEDC_IRQ_PRIORITY);
    NVIC_DisableIRQ(LEDC_IRQn);
    // config timing
    om_ledc->SIG_TIM = (config->long_pulse_cycles << LEDC_SIG_TIM_TL_POS) | config->short_pulse_cycles;
    om_ledc->RST_TIM = config->reset_cycles;

    return OM_ERROR_OK;
}
extern void uart_tx(unsigned char data);
extern bool bargb_valid;

// 调用处: USB 主循环(logo_process 之后)、蓝牙扫描和广播事件(logo_process 之后)、2.4G RTC 中断(每 8ms).
// 2.4G 是在中断里调的, 所以放 RAM. 2.4G 的渲染在主循环 / 没接收器的等待循环里(不在 RF 回调里).
__RAM_CODE om_error_t drv_ledc_write()
{
    // DMA 直接读 pwm_out, 不拷影子缓冲(104 颗省 312 字节 RAM). 约束:
    //  1. 两次调用间隔要大于一帧 + Reset(104 颗约 2.3~3.1ms + 80us), 否则会打断正在发的一帧.
    //     现在 USB >=7ms、蓝牙 7.5ms、2.4G 8ms, 都满足.
    //  2. USB/蓝牙是渲染完紧接着发, 下次改写 pwm_out 时这一帧早已发完.
    //     2.4G 的渲染(主循环)和发送(RTC 中断)不在一处, 渲染可能落在发送途中:
    //     码型不受影响, 只是这一帧里部分灯珠提前一帧(8ms)换成新画面, 看不出来.
    if(false == bargb_valid)
        return OM_ERROR_OK;
OM_CRITICAL_BEGIN();
    OM_LEDC->DMA_CTL = ((uint32_t)pwm_out << LEDC_DMA_CTL_DMA_ADDR_POS) | (LEDC_DMA_CTL_DMA_LEN_MASK & (iLED_NUM_MAX*3));
    // enable ledc
    OM_LEDC->CR |= LEDC_CR_ENABLE_MASK;
OM_CRITICAL_END();

    return OM_ERROR_OK;
}

om_error_t drv_ledc_write_int(OM_LEDC_Type    *om_ledc,
                              uint8_t         *data,
                              uint16_t         data_len)
{
    om_ledc->DMA_CTL = ((uint32_t)data << LEDC_DMA_CTL_DMA_ADDR_POS) | (LEDC_DMA_CTL_DMA_LEN_MASK & data_len);
    // enable ledc and interrupt
    om_ledc->CR |= (LEDC_CR_INT_ENABLE_MASK + LEDC_CR_ENABLE_MASK);

    return OM_ERROR_OK;
}

#if (RTE_LEDC_REGISTER_CALLBACK)
void drv_ledc_register_isr_callback(OM_LEDC_Type *om_ledc, drv_isr_callback_t isr_cb)
{
    ledc_env.isr_cb = isr_cb;
}
#endif /* (RTE_LEDC_REGISTER_CALLBACK) */

__WEAK void drv_ledc_isr_callback(OM_LEDC_Type *om_ledc, drv_event_t event)
{
    #if (RTE_LEDC_REGISTER_CALLBACK)
    if (ledc_env.isr_cb != NULL) {
        ledc_env.isr_cb(om_ledc, event, NULL, NULL);
    }
    #endif /* (RTE_LEDC_REGISTER_CALLBACK) */
}

void drv_ledc_isr(OM_LEDC_Type *om_ledc)
{
    if (om_ledc->CR & LEDC_CR_INT_RAW_STATUS_MASK) {
        om_ledc->CR |= LEDC_CR_INT_RAW_STATUS_MASK;
        drv_ledc_isr_callback((void *)om_ledc, DRV_EVENT_COMMON_GENERAL);
    }
}


#endif  /* (RTE_LEDC) */

/** @} */
