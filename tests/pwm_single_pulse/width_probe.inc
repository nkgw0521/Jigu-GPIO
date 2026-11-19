
/* Test-only capture; never linked into the production firmware. */
volatile uint32_t probe_ready, probe_start, probe_end, probe_status, probe_index;
volatile uint32_t probe_target = 1;
static void width_probe_arm(void)
{
    TIM_TypeDef *t = PWM_COUNTER_HANDLE.Instance;
    t->CR1 = 0; t->DIER = 0; t->SMCR = 0;
    t->PSC = 0; t->ARR = 0xffffU; t->EGR = TIM_EGR_UG;
    t->CCMR1 = TIM_CCMR1_CC1S_1 | TIM_CCMR1_CC2S_0;
    t->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E |
        ((pwm_idle_level == GPIO_PIN_SET) ? TIM_CCER_CC1P : TIM_CCER_CC2P);
    t->CNT = 0; t->CCR1 = 0; t->CCR2 = 0; t->SR = 0;
    probe_ready = 0; probe_index = 0; probe_start = 0; probe_end = 0; probe_status = 0;
    t->DIER = TIM_DIER_CC2IE; t->CR1 = TIM_CR1_CEN;
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *h)
{
    if (h->Instance == PWM_COUNTER_HANDLE.Instance && h->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
        TIM_TypeDef *t = h->Instance;
        if (++probe_index < probe_target) {
            t->SR &= ~(TIM_SR_CC1IF | TIM_SR_CC1OF | TIM_SR_CC2OF);
            return;
        }
        t->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC2E); t->DIER = 0;
        probe_status = t->SR; probe_start = t->CCR1; probe_end = t->CCR2;
        probe_ready = 1;
    }
}
