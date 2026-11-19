#include "tim.h"
#include "width_probe.h"
void width_probe_arm(void)
{
    /* PB5 is TIM3 TI2. IC1 is indirect TI2 (start), IC2 direct TI2 (end).
       Capture timestamps in hardware with a free-running 64 MHz TIM3 clock.
       No capture IRQ or instrumentation runs during the output pulse. */
    TIM3->CR1 = 0;
    TIM3->DIER = 0;
    TIM3->SMCR = 0;
    TIM3->PSC = 0;
    TIM3->ARR = 0xFFFFU;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CCMR1 = TIM_CCMR1_CC1S_1 | TIM_CCMR1_CC2S_0;
    if ((TIM1->CCER & TIM_CCER_CC1P) != 0U) {
        TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC1P | TIM_CCER_CC2E;
    } else {
        TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC2P;
    }
    TIM3->CNT = 0;
    TIM3->CCR1 = 0;
    TIM3->CCR2 = 0;
    TIM3->SR = 0;
    TIM3->DIER = TIM_DIER_CC2IE;
    TIM3->CR1 = TIM_CR1_CEN;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
        /* Both timestamps are already captured. Freeze only capture channels
           after the first pulse so later pulses cannot overwrite its width. */
        TIM3->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E);
        TIM3->DIER = 0;
    }
}
