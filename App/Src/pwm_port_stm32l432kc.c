#include <stdbool.h>
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
 * port layer and the CubeMX timer/GPIO configuration. Application and SCPI
 * code must not directly access htim1/htim2 or IRQ names.
 */

void pwm_port_stop(void)
{
    HAL_TIM_Base_Stop(&htim2);
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    pwm_run_state_set(false);
}

HAL_StatusTypeDef pwm_port_start(const pwm_port_config_t *cfg)
{
    HAL_StatusTypeDef status;

    if (cfg == NULL) {
        return HAL_ERROR;
    }

    /* Ensure HAL timer states are clean before restarting. */
    HAL_TIM_Base_Stop(&htim2);
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_DISABLE(&htim1);
    pwm_run_state_set(false);

    pwm_setup(cfg->freq_hz, cfg->width_us, cfg->polarity);

    pwm_numbers_set(cfg->numbers);
    pwm_shot_count_reset();
    pwm_run_state_set(true);

    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    status = HAL_TIM_Base_Start(&htim2);
    if (status != HAL_OK) {
        pwm_run_state_set(false);
        return status;
    }

    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_ENABLE(&htim1);
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    HAL_NVIC_ClearPendingIRQ(TIM1_UP_TIM16_IRQn);

    status = HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    if (status != HAL_OK) {
        pwm_run_state_set(false);
        return status;
    }

    status = HAL_TIM_Base_Start_IT(&htim1);
    if (status != HAL_OK) {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        pwm_run_state_set(false);
        return status;
    }

    return HAL_OK;
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
