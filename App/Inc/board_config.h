#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"

/*
 * Board-specific PWM port configuration.
 *
 * App code should include pwm_generator.h / pwm_port.h only.
 * This file is included only by the STM32 port layer.
 *
 * Requirements for a supported board:
 * - PWM timer supports PWM output, update interrupt, and compare interrupt.
 * - Counter timer is configured by CubeMX as an external pulse counter.
 * - CubeMX must provide the handle names used below.
 */

#if defined(STM32L432xx)

#define PWM_TIMER_HANDLE              htim1
#define PWM_COUNTER_HANDLE            htim2

#define PWM_CHANNEL                   TIM_CHANNEL_1
#define PWM_ACTIVE_CHANNEL            HAL_TIM_ACTIVE_CHANNEL_1

#define PWM_UPDATE_IRQn               TIM1_UP_TIM16_IRQn
#define PWM_CC_IRQn                   TIM1_CC_IRQn

#define PWM_UPDATE_IT                 TIM_IT_UPDATE
#define PWM_COMPARE_IT                TIM_IT_CC1
#define PWM_UPDATE_FLAG               TIM_FLAG_UPDATE
#define PWM_COMPARE_FLAG              TIM_FLAG_CC1

#define PWM_TIMER_CCR_VALUE()         (PWM_TIMER_HANDLE.Instance->CCR1)
#define PWM_TIMER_ARR_VALUE()         (PWM_TIMER_HANDLE.Instance->ARR)
#define PWM_TIMER_CNT_VALUE()         (PWM_TIMER_HANDLE.Instance->CNT)

#define PWM_OUTPUT_GPIO_PORT          GPIOA
#define PWM_OUTPUT_GPIO_PIN           GPIO_PIN_8

#define PWM_TIMER_PCLK_FREQ()         HAL_RCC_GetPCLK2Freq()
#define PWM_TIMER_APB_DIVIDER_FIELD   APB2CLKDivider

#elif defined(STM32F407xx)

#define PWM_TIMER_HANDLE              htim1
#define PWM_COUNTER_HANDLE            htim2

#define PWM_CHANNEL                   TIM_CHANNEL_1
#define PWM_ACTIVE_CHANNEL            HAL_TIM_ACTIVE_CHANNEL_1

#define PWM_UPDATE_IRQn               TIM1_UP_TIM10_IRQn
#define PWM_CC_IRQn                   TIM1_CC_IRQn

#define PWM_UPDATE_IT                 TIM_IT_UPDATE
#define PWM_COMPARE_IT                TIM_IT_CC1
#define PWM_UPDATE_FLAG               TIM_FLAG_UPDATE
#define PWM_COMPARE_FLAG              TIM_FLAG_CC1

#define PWM_TIMER_CCR_VALUE()         (PWM_TIMER_HANDLE.Instance->CCR1)
#define PWM_TIMER_ARR_VALUE()         (PWM_TIMER_HANDLE.Instance->ARR)
#define PWM_TIMER_CNT_VALUE()         (PWM_TIMER_HANDLE.Instance->CNT)

#define PWM_OUTPUT_GPIO_PORT          GPIOA
#define PWM_OUTPUT_GPIO_PIN           GPIO_PIN_8

#define PWM_TIMER_PCLK_FREQ()         HAL_RCC_GetPCLK2Freq()
#define PWM_TIMER_APB_DIVIDER_FIELD   APB2CLKDivider

#else
#error "Unsupported STM32 target. Add board configuration to board_config.h."
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H_ */
