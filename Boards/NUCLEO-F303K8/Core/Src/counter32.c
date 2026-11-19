#include "counter32.h"

/* TIM3 is 16-bit; the update interrupt extends it modulo 2^32. */
static volatile uint32_t counter_high;

static void counter32_fold_pending_overflow(void)
{
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE) != RESET) {
        counter_high += 0x10000U;
        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    }
    HAL_NVIC_ClearPendingIRQ(TIM3_IRQn);
}

HAL_StatusTypeDef counter32_start(void)
{
    uint32_t primask = __get_PRIMASK();
    HAL_StatusTypeDef status;
    __disable_irq();
    /* Preserve a pending overflow across stop/start, rather than discarding it. */
    counter32_fold_pending_overflow();
    status = HAL_TIM_Base_Start_IT(&htim3);
    __set_PRIMASK(primask);
    return status;
}

void counter32_stop(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    /* HAL's disable macro may retain CEN when capture channel CC2E is set. */
    CLEAR_BIT(htim3.Instance->CR1, TIM_CR1_CEN);
    HAL_TIM_Base_Stop_IT(&htim3);
    counter32_fold_pending_overflow();
    __set_PRIMASK(primask);
}

void counter32_set(uint32_t value)
{
    uint32_t primask = __get_PRIMASK();
    uint32_t was_running;
    __disable_irq();
    was_running = htim3.Instance->CR1 & TIM_CR1_CEN;
    /* Pause while replacing both halves; edges during this explicit set are ignored. */
    CLEAR_BIT(htim3.Instance->CR1, TIM_CR1_CEN);
    counter_high = value & 0xFFFF0000U;
    __HAL_TIM_SET_COUNTER(&htim3, value & 0xFFFFU);
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    HAL_NVIC_ClearPendingIRQ(TIM3_IRQn);
    if (was_running != 0U) {
        SET_BIT(htim3.Instance->CR1, TIM_CR1_CEN);
    }
    __set_PRIMASK(primask);
}

uint32_t counter32_get(void)
{
    uint32_t primask = __get_PRIMASK();
    uint32_t high, low;
    __disable_irq();
    high = counter_high;
    low = __HAL_TIM_GET_COUNTER(&htim3) & 0xFFFFU;
    /* The counter continues running while interrupts are masked. If it wrapped
       before/during the sample, reread the low half after observing UIF. Leave
       UIF set so the ISR accounts for this overflow exactly once afterward. */
    if (__HAL_TIM_GET_FLAG(&htim3, TIM_FLAG_UPDATE) != RESET) {
        high += 0x10000U;
        low = __HAL_TIM_GET_COUNTER(&htim3) & 0xFFFFU;
    }
    __set_PRIMASK(primask);
    return high | low;
}

void counter32_on_overflow(void)
{
    /* HAL_TIM_IRQHandler clears UIF before calling PeriodElapsedCallback. */
    counter_high += 0x10000U;
}
