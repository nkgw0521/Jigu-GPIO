#ifndef F303_TIM_H
#define F303_TIM_H
#include "main.h"
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
void MX_TIM1_Init(void);
void MX_TIM3_Init(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
#endif
