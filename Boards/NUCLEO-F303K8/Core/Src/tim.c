#include "tim.h"

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef clock = {0};
  TIM_MasterConfigTypeDef master = {0};
  TIM_OC_InitTypeDef oc = {0};
  TIM_BreakDeadTimeConfigTypeDef bd = {0};
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 63;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK) { Error_Handler(); }
  clock.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &clock) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) { Error_Handler(); }
  master.MasterOutputTrigger = TIM_TRGO_RESET;
  master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &master) != HAL_OK) { Error_Handler(); }
  oc.OCMode = TIM_OCMODE_PWM1;
  oc.Pulse = 500;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  oc.OCIdleState = TIM_OCIDLESTATE_SET;
  oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &oc, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  bd.OffStateRunMode = TIM_OSSR_DISABLE;
  bd.OffStateIDLEMode = TIM_OSSI_ENABLE;
  bd.LockLevel = TIM_LOCKLEVEL_OFF;
  bd.DeadTime = 0;
  bd.BreakState = TIM_BREAK_DISABLE;
  bd.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  bd.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &bd) != HAL_OK) { Error_Handler(); }
  HAL_TIM_MspPostInit(&htim1);
}

void MX_TIM3_Init(void)
{
  TIM_SlaveConfigTypeDef slave = {0};
  TIM_MasterConfigTypeDef master = {0};
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 0xFFFFU;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK) { Error_Handler(); }
  slave.SlaveMode = TIM_SLAVEMODE_EXTERNAL1;
  slave.InputTrigger = TIM_TS_TI2FP2;
  slave.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING;
  slave.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
  slave.TriggerFilter = 0;
  if (HAL_TIM_SlaveConfigSynchro(&htim3, &slave) != HAL_OK) { Error_Handler(); }
  master.MasterOutputTrigger = TIM_TRGO_RESET;
  master.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &master) != HAL_OK) { Error_Handler(); }
  /* Discard the initialization update event before enabling overflow IRQs. */
  __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef gpio = {0};
  if (htim->Instance == TIM1) {
    __HAL_RCC_TIM1_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_UP_TIM16_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
  } else if (htim->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(GPIOB, &gpio);
  }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
  GPIO_InitTypeDef gpio = {0};
  if (htim->Instance == TIM1) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_8;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF6_TIM1;
    HAL_GPIO_Init(GPIOA, &gpio);
  }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) {
    __HAL_RCC_TIM1_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(TIM1_UP_TIM16_IRQn);
    HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
  } else if (htim->Instance == TIM3) {
    __HAL_RCC_TIM3_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(TIM3_IRQn);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);
  }
}
