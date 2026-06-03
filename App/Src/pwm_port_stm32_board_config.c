#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include "pwm_port.h"
#include "pwm_generator.h"
#include "board_config.h"

/*
 * Common STM32 HAL PWM port implementation.
 *
 * Board-specific handle/channel/IRQ/pin differences are kept in board_config.h.
 * Generator and SCPI code must not directly access TIM/GPIO/HAL details.
 */

static uint32_t pwm_port_get_timer_clk_hz(void)
{
    uint32_t pclk = PWM_TIMER_PCLK_FREQ();

    RCC_ClkInitTypeDef clkconfig;
    uint32_t flash_latency = 0;
    HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);

    if (clkconfig.PWM_TIMER_APB_DIVIDER_FIELD == RCC_HCLK_DIV1) {
        return pclk;
    } else {
        return pclk * 2U;
    }
}

static bool pwm_port_find_best_freq(uint32_t target_freq_hz,
                                    uint16_t *out_psc,
                                    uint16_t *out_arr,
                                    uint32_t *out_actual_freq_hz)
{
    if ((target_freq_hz == 0U) || (out_psc == NULL) ||
        (out_arr == NULL) || (out_actual_freq_hz == NULL)) {
        return false;
    }

    uint32_t tim_clk = pwm_port_get_timer_clk_hz();
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

    if (!pwm_port_find_best_freq(freq_hz, &psc, &arr, &actual_freq_hz)) {
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

    PWM_TIMER_HANDLE.Init.Prescaler = psc;
    PWM_TIMER_HANDLE.Init.Period = arr;
    if (HAL_TIM_PWM_Init(&PWM_TIMER_HANDLE) != HAL_OK) {
        return false;
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = ccr;
    sConfigOC.OCPolarity = (polarity == 0U) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    /*
     * Advanced-control timers such as TIM1 on STM32L432 have complementary
     * output and idle-state fields. General-purpose timers such as TIM21 on
     * STM32L053 do not define these members in TIM_OC_InitTypeDef.
     */
#if defined(TIM_OCNPOLARITY_HIGH)
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
#endif

#if defined(TIM_OCIDLESTATE_SET) && defined(TIM_OCIDLESTATE_RESET)
    sConfigOC.OCIdleState = (polarity == 0U) ? TIM_OCIDLESTATE_SET : TIM_OCIDLESTATE_RESET;
#endif

#if defined(TIM_OCNIDLESTATE_RESET)
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
#endif

    if (HAL_TIM_PWM_ConfigChannel(&PWM_TIMER_HANDLE, &sConfigOC, PWM_CHANNEL) != HAL_OK) {
        return false;
    }

    return true;
}

bool pwm_port_start_output(void)
{
    HAL_StatusTypeDef status;

    __HAL_TIM_CLEAR_FLAG(&PWM_COUNTER_HANDLE, TIM_FLAG_UPDATE);
    status = HAL_TIM_Base_Start(&PWM_COUNTER_HANDLE);
    if (status != HAL_OK) {
        return false;
    }

    __HAL_TIM_SET_COUNTER(&PWM_TIMER_HANDLE, 0);
    __HAL_TIM_ENABLE(&PWM_TIMER_HANDLE);
    __HAL_TIM_CLEAR_FLAG(&PWM_TIMER_HANDLE, PWM_UPDATE_FLAG);
    HAL_NVIC_ClearPendingIRQ(PWM_UPDATE_IRQn);

    status = HAL_TIM_PWM_Start(&PWM_TIMER_HANDLE, PWM_CHANNEL);
    if (status != HAL_OK) {
        return false;
    }

    status = HAL_TIM_Base_Start_IT(&PWM_TIMER_HANDLE);
    if (status != HAL_OK) {
        HAL_TIM_PWM_Stop(&PWM_TIMER_HANDLE, PWM_CHANNEL);
        return false;
    }

    return true;
}

void pwm_port_stop_all(void)
{
    HAL_TIM_Base_Stop(&PWM_COUNTER_HANDLE);
    HAL_TIM_Base_Stop_IT(&PWM_TIMER_HANDLE);
    HAL_TIM_PWM_Stop(&PWM_TIMER_HANDLE, PWM_CHANNEL);
    __HAL_TIM_DISABLE(&PWM_TIMER_HANDLE);
}

void pwm_port_stop_output_from_isr(void)
{
    HAL_TIM_Base_Stop_IT(&PWM_TIMER_HANDLE);
    HAL_TIM_PWM_Stop(&PWM_TIMER_HANDLE, PWM_CHANNEL);
}

void pwm_port_disable_update_irq(void)
{
    __HAL_TIM_DISABLE_IT(&PWM_TIMER_HANDLE, PWM_UPDATE_IT);
}

void pwm_port_enable_compare_irq(void)
{
    __HAL_TIM_ENABLE_IT(&PWM_TIMER_HANDLE, PWM_COMPARE_IT);
}

void pwm_port_disable_compare_irq(void)
{
    __HAL_TIM_DISABLE_IT(&PWM_TIMER_HANDLE, PWM_COMPARE_IT);
}

void pwm_port_clear_update_flag(void)
{
    __HAL_TIM_CLEAR_FLAG(&PWM_TIMER_HANDLE, PWM_UPDATE_FLAG);
}

void pwm_port_clear_compare_flag(void)
{
    __HAL_TIM_CLEAR_FLAG(&PWM_TIMER_HANDLE, PWM_COMPARE_FLAG);
}

void pwm_port_clear_update_pending_irq(void)
{
    HAL_NVIC_ClearPendingIRQ(PWM_UPDATE_IRQn);
}

void pwm_port_clear_compare_pending_irq(void)
{
    HAL_NVIC_ClearPendingIRQ(PWM_CC_IRQn);
}

void pwm_port_capture_snapshot(pwm_port_snapshot_t *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    snapshot->cnt = PWM_TIMER_CNT_VALUE();
    snapshot->ccr = PWM_TIMER_CCR_VALUE();
    snapshot->arr = PWM_TIMER_ARR_VALUE();
    snapshot->output_level = (PWM_OUTPUT_GPIO_PORT->IDR & PWM_OUTPUT_GPIO_PIN) ? 1U : 0U;
}

void pwm_port_counter_set(uint32_t value)
{
    __disable_irq();
    __HAL_TIM_SET_COUNTER(&PWM_COUNTER_HANDLE, value);
    __enable_irq();
}

uint32_t pwm_port_counter_get(void)
{
    uint32_t value;

    __disable_irq();
    value = __HAL_TIM_GET_COUNTER(&PWM_COUNTER_HANDLE);
    __enable_irq();

    return value;
}

void pwm_port_counter_reset(void)
{
    pwm_port_counter_set(0U);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == PWM_TIMER_HANDLE.Instance) {
        pwm_generator_on_period_elapsed();
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != PWM_TIMER_HANDLE.Instance) {
        return;
    }

    if (htim->Channel != PWM_ACTIVE_CHANNEL) {
        return;
    }

    pwm_generator_on_compare_match();
}
