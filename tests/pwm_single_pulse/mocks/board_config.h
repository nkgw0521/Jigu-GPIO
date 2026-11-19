#ifndef MOCK_BOARD_CONFIG_H
#define MOCK_BOARD_CONFIG_H
#include <stdint.h>
#include <stdbool.h>
typedef struct {uint32_t CR1,SMCR,CCMR1,CCER,ARR,CNT,CCR1,CCR2,SR,DIER;} TIM_TypeDef;
typedef struct {TIM_TypeDef *Instance; struct {uint32_t Prescaler,Period;} Init; uint32_t Channel;} TIM_HandleTypeDef;
typedef struct {uint32_t MODER,PUPDR,IDR,ODR;} GPIO_TypeDef;
typedef enum {GPIO_PIN_RESET,GPIO_PIN_SET} GPIO_PinState;
typedef struct {uint32_t APB2CLKDivider;} RCC_ClkInitTypeDef;
typedef struct {uint32_t OCMode,Pulse,OCPolarity,OCFastMode;} TIM_OC_InitTypeDef;
typedef enum {HAL_OK,HAL_ERROR} HAL_StatusTypeDef;
extern TIM_HandleTypeDef mock_pwm,mock_counter;
extern GPIO_TypeDef mock_gpio;
extern uint32_t mock_irqmask,mock_active_ccr;
#define PWM_TIMER_HANDLE mock_pwm
#define PWM_COUNTER_HANDLE mock_counter
#define PWM_CHANNEL TEST_CHANNEL
#define PWM_ACTIVE_CHANNEL TEST_CHANNEL
#define PWM_UPDATE_IRQn 1
#define PWM_CC_IRQn 2
#define PWM_UPDATE_IT 1U
#define PWM_COMPARE_IT 2U
#define PWM_UPDATE_FLAG 1U
#define PWM_COMPARE_FLAG 2U
#if TEST_CHANNEL == 1
#define MOCK_CCR(h) ((h)->Instance->CCR1)
#else
#define MOCK_CCR(h) ((h)->Instance->CCR2)
#endif
#define PWM_TIMER_CCR_VALUE() MOCK_CCR(&mock_pwm)
#define PWM_TIMER_ARR_VALUE() mock_pwm.Instance->ARR
#define PWM_TIMER_CNT_VALUE() mock_pwm.Instance->CNT
#define PWM_OUTPUT_GPIO_PORT (&mock_gpio)
#define PWM_OUTPUT_GPIO_PIN (1U << TEST_PIN)
#define PWM_TIMER_PCLK_FREQ() 64000000U
#define PWM_TIMER_APB_DIVIDER_FIELD APB2CLKDivider
#define RCC_HCLK_DIV1 1U
#define TIM_CCMR1_CC2S 3U
#define TIM_CCMR1_CC2S_0 1U
#define TIM_CCER_CC2P 2U
#define TIM_CCER_CC2NP 4U
#define TIM_CCER_CC2E 8U
#define TIM_FLAG_UPDATE 1U
#define TIM_OCMODE_PWM1 1U
#define TIM_OCPOLARITY_LOW 0U
#define TIM_OCPOLARITY_HIGH 1U
#define TIM_OCFAST_DISABLE 0U
#define TIM_EVENTSOURCE_UPDATE 1U
#define TIM_CR1_URS 4U
#define GPIO_PULLUP 1U
#define GPIO_PULLDOWN 2U
#define MODIFY_REG(reg,mask,value) ((reg)=((reg)&~(mask))|(value))
#define CLEAR_BIT(reg,bits) ((reg)&=~(bits))
#define SET_BIT(reg,bits) ((reg)|=(bits))
#define __get_PRIMASK() mock_irqmask
#define __disable_irq() (mock_irqmask=1U)
#define __enable_irq() (mock_irqmask=0U)
void mock_restore_irq(uint32_t value);
#define __set_PRIMASK(v) mock_restore_irq(v)
#define __HAL_TIM_SET_PRESCALER(h,v) ((h)->Init.Prescaler=(v))
#define __HAL_TIM_SET_AUTORELOAD(h,v) ((h)->Instance->ARR=(v))
#define __HAL_TIM_SET_COMPARE(h,ch,v) (MOCK_CCR(h)=(v))
#define __HAL_TIM_SET_COUNTER(h,v) ((h)->Instance->CNT=(v))
#define __HAL_TIM_GET_COUNTER(h) ((h)->Instance->CNT)
#define __HAL_TIM_CLEAR_FLAG(h,f) ((h)->Instance->SR &= ~(f))
#define __HAL_TIM_ENABLE_IT(h,f) ((h)->Instance->DIER |= (f))
#define __HAL_TIM_DISABLE_IT(h,f) ((h)->Instance->DIER &= ~(f))
#define __HAL_TIM_DISABLE(h) ((h)->Instance->CR1 &= ~1U)
void HAL_GPIO_WritePin(GPIO_TypeDef*,uint32_t,GPIO_PinState);
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef*,uint32_t*);
void HAL_NVIC_ClearPendingIRQ(int);
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef*,TIM_OC_InitTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef*,uint32_t);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef*);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef*);
#endif
