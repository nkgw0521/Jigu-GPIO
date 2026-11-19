#ifndef MOCK_TIM_H
#define MOCK_TIM_H
#include <stdint.h>
typedef struct { uint32_t CR1, CNT, SR, DIER; } TIM_TypeDef;
typedef struct { TIM_TypeDef *Instance; } TIM_HandleTypeDef;
typedef enum { HAL_OK, HAL_ERROR } HAL_StatusTypeDef;
extern TIM_HandleTypeDef htim3;
extern uint32_t mock_primask, mock_pending;
extern int mock_wrap_on_read;
#define RESET 0U
#define TIM_FLAG_UPDATE 1U
#define TIM_CR1_CEN 1U
#define TIM3_IRQn 29
#define CLEAR_BIT(reg, bits) ((reg) &= ~(bits))
#define SET_BIT(reg, bits) ((reg) |= (bits))
#define __get_PRIMASK() mock_primask
#define __disable_irq() (mock_primask = 1U)
#define __set_PRIMASK(value) (mock_primask = (value))
#define __HAL_TIM_GET_FLAG(h, flag) ((h)->Instance->SR & (flag))
#define __HAL_TIM_CLEAR_FLAG(h, flag) ((h)->Instance->SR &= ~(flag))
#define __HAL_TIM_SET_COUNTER(h, value) ((h)->Instance->CNT = (value))
uint32_t mock_get_counter(TIM_HandleTypeDef *h);
#define __HAL_TIM_GET_COUNTER(h) mock_get_counter(h)
void HAL_NVIC_ClearPendingIRQ(int irq);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *h);
#endif
