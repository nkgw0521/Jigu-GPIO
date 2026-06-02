#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include "pwm_port.h"
#include "pwm_generator.h"
#include "tim.h"

/*
 * STM32L432KC Nucleo port implementation.
 *
 * PWM output : TIM1_CH1 on PA8
 * Pulse count: TIM2 external clock on PA1/TIM2_CH2
 *
 * Board migration should be done by replacing or conditionalizing only this
 * port layer and the CubeMX timer/GPIO configuration. Generator and SCPI code
 * must not directly access htim1/htim2, TIM registers, GPIO registers or IRQ names.
 */

static uint32_t tim1_get_clk_hz(void)
{
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

    RCC_ClkInitTypeDef clkconfig;
    uint32_t flash_latency = 0;
    HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);

    if (clkconfig.APB2CLKDivider == RCC_HCLK_DIV1) {
        return pclk2;
    } else {
        return pclk2 * 2U;
    }
}

static bool tim1_find_best_freq(uint32_t target_freq_hz,
                                uint16_t *out_psc,
                                uint16_t *out_arr,
                                uint32_t *out_actual_freq_hz)
{
    if ((target_freq_hz == 0U) || (out_psc == NULL) || (out_arr == NULL) || (out_actual_freq_hz == NULL)) {
        return false;
    }

    uint32_t tim_clk = tim1_get_clk_hz();
    uint64_t best_err = UINT64_MAX;
    bool found = false;

    for (uint32_t psc = 0; psc <= 65535U; psc++) {
        uint64_t div_psc = (uint64_t)psc + 1ULL;

        uint64_t arr_plus_1 = ((uint64_t)tim_clk + ((uint64_t)target_freq_hz * div_psc) / 2ULL)
                            / ((uint64_t)target_freq_hz * div_psc);

        if ((arr_plus_1 == 0ULL) || (arr_plus_1 > 65536ULL)) {
            continue;
        }

        uint32_t actual_freq = (uint32_t)((uint64_t)tim_clk / (div_psc * arr_plus_1));
        uint64_t err = (actual_freq > target_freq_hz) ? (actual_freq - target_freq_hz)
                                                      : (target_freq_hz - actual_freq);

        if (err < best_err) {
            best_err = err;
            *out_psc = (uint16_t)psc;
            *out_arr = (uint16_t)(arr_plus_1 - 1ULL);
            *out_actual_freq_hz = actual_freq;
            found = true;

            if (err == 0ULL) {
                break;
            }
        }
    }

    return found;
}

bool pwm_port_configure_output(uint32_t freq_hz, uint32_t width_us, uint32_t polarity)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    if (freq_hz == 0U) {
        return false;
    }

    uint32_t period_us = 1000000UL / freq_hz;
    if ((width_us == 0U) || (width_us >= period_us)) {
        return false;
    }

    uint16_t psc = 0;
    uint16_t arr = 0;
    uint32_t actual_freq_hz = 0;

    if (!tim1_find_best_freq(freq_hz, &psc, &arr, &actual_freq_hz)) {
        return false;
    }

    /* CCR = round((ARR+1) * pulse_us * actual_freq / 1e6) */
    uint64_t ccr64 = ((uint64_t)(arr + 1U) * (uint64_t)width_us * (uint64_t)actual_freq_hz + 500000ULL)
                   / 1000000ULL;

    if (ccr64 == 0ULL) {
        ccr64 = 1ULL;
    }
    if (ccr64 > (uint64_t)(arr + 1U)) {
        ccr64 = (uint64_t)(arr + 1U);
    }

    uint16_t ccr = (uint16_t)ccr64;

    htim1.Init.Prescaler = psc;
    htim1.Init.Period = arr;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        return false;
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = ccr;
    sConfigOC.OCPolarity = (polarity == 0U) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = (polarity == 0U) ? TIM_OCIDLESTATE_SET : TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        return false;
    }

    return true;
}

bool pwm_port_start_output(void)
{
    HAL_StatusTypeDef status;

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    status = HAL_TIM_Base_Start(&htim2);
    if (status != HAL_OK) {
        return false;
    }

    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_ENABLE(&htim1);
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    HAL_NVIC_ClearPendingIRQ(TIM1_UP_TIM16_IRQn);

    status = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    if (status != HAL_OK) {
        return false;
    }

    status = HAL_TIM_Base_Start_IT(&htim1);
    if (status != HAL_OK) {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        return false;
    }

    return true;
}

void pwm_port_stop_all(void)
{
    HAL_TIM_Base_Stop(&htim2);
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_DISABLE(&htim1);
}

void pwm_port_stop_output_from_isr(void)
{
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

void pwm_port_disable_update_irq(void)
{
    __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
}

void pwm_port_enable_compare_irq(void)
{
    __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC1);
}

void pwm_port_disable_compare_irq(void)
{
    __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_CC1);
}

void pwm_port_clear_update_flag(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
}

void pwm_port_clear_compare_flag(void)
{
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1);
}

void pwm_port_clear_update_pending_irq(void)
{
    HAL_NVIC_ClearPendingIRQ(TIM1_UP_TIM16_IRQn);
}

void pwm_port_clear_compare_pending_irq(void)
{
    HAL_NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
}

void pwm_port_capture_snapshot(pwm_port_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    snapshot->cnt = TIM1->CNT;
    snapshot->ccr = TIM1->CCR1;
    snapshot->arr = TIM1->ARR;
    snapshot->output_level = (GPIOA->IDR & GPIO_PIN_8) ? 1U : 0U;
}

void pwm_port_counter_set(uint32_t value)
{
    __disable_irq();
    __HAL_TIM_SET_COUNTER(&htim2, value);
    __enable_irq();
}

uint32_t pwm_port_counter_get(void)
{
    uint32_t value;

    __disable_irq();
    value = __HAL_TIM_GET_COUNTER(&htim2);
    __enable_irq();

    return value;
}

void pwm_port_counter_reset(void)
{
    pwm_port_counter_set(0U);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        pwm_generator_on_period_elapsed();
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM1) {
        return;
    }

    if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) {
        return;
    }

    pwm_generator_on_compare_match();
}
