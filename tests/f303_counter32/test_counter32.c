#include <assert.h>
#include <stdio.h>
#include "counter32.h"
static TIM_TypeDef regs;
TIM_HandleTypeDef htim3 = { &regs };
uint32_t mock_primask, mock_pending;
int mock_wrap_on_read;
static HAL_StatusTypeDef start_result = HAL_OK;
uint32_t mock_get_counter(TIM_HandleTypeDef *h)
{
    uint32_t value = h->Instance->CNT;
    if (mock_wrap_on_read) {
        mock_wrap_on_read = 0;
        h->Instance->CNT = 0;
        h->Instance->SR |= TIM_FLAG_UPDATE;
    }
    return value;
}
void HAL_NVIC_ClearPendingIRQ(int irq) { (void)irq; mock_pending = 0; }
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h)
{
    if (start_result == HAL_OK) { h->Instance->CR1 |= TIM_CR1_CEN; h->Instance->DIER = 1; }
    return start_result;
}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *h)
{
    /* Model the HAL's CC2E condition by deliberately leaving CEN alone. */
    h->Instance->DIER = 0;
    return HAL_OK;
}
static void reset(uint32_t value)
{
    mock_primask = 0; regs.CR1 = 0; regs.SR = 0; regs.DIER = 0;
    mock_wrap_on_read = 0; start_result = HAL_OK;
    counter32_set(value);
}
static void deliver_update_irq(void)
{
    assert(regs.SR & TIM_FLAG_UPDATE);
    regs.SR &= ~TIM_FLAG_UPDATE; /* HAL clears UIF before invoking the callback. */
    counter32_on_overflow();
}
int main(void)
{
    reset(0); assert(counter32_get() == 0);
    reset(65535); assert(counter32_get() == 65535);
    regs.CNT = 0; regs.SR = TIM_FLAG_UPDATE;
    assert(counter32_get() == 65536); /* Pending overflow, ISR not yet run. */
    assert(counter32_get() == 65536); /* Reads must not double-account. */
    deliver_update_irq(); assert(counter32_get() == 65536);
    regs.CNT = 19; regs.SR = TIM_FLAG_UPDATE; deliver_update_irq();
    assert(counter32_get() == 131091);
    reset(65535); mock_wrap_on_read = 1;
    assert(counter32_get() == 65536); /* Wrap between reading CNT and UIF. */
    deliver_update_irq(); assert(counter32_get() == 65536);
    reset(0xFFFFFFFFU); assert(counter32_get() == 0xFFFFFFFFU);
    regs.CNT = 0; regs.SR = TIM_FLAG_UPDATE;
    assert(counter32_get() == 0); deliver_update_irq(); assert(counter32_get() == 0);
    reset(0x12345678U); assert(regs.CNT == 0x5678U); assert(counter32_get() == 0x12345678U);
    regs.CR1 = TIM_CR1_CEN; regs.SR = TIM_FLAG_UPDATE; mock_pending = 1;
    counter32_set(7); assert(counter32_get() == 7); assert(!regs.SR && !mock_pending);
    assert(regs.CR1 & TIM_CR1_CEN); /* Set preserves running state. */
    counter32_stop(); assert(!(regs.CR1 & TIM_CR1_CEN) && !regs.DIER);
    counter32_set(0); assert(!(regs.CR1 & TIM_CR1_CEN)); /* Set preserves stopped state. */
    reset(65535); assert(counter32_start() == HAL_OK);
    regs.CNT = 9; regs.SR = TIM_FLAG_UPDATE; mock_pending = 1;
    counter32_stop(); assert(counter32_get() == 65545); assert(!mock_pending);
    assert(counter32_start() == HAL_OK); assert(counter32_get() == 65545);
    counter32_stop(); reset(0); regs.SR = TIM_FLAG_UPDATE;
    assert(counter32_start() == HAL_OK); assert(counter32_get() == 65536);
    reset(0); start_result = HAL_ERROR; assert(counter32_start() == HAL_ERROR);
    assert(!(regs.CR1 & TIM_CR1_CEN));
    for (int masked = 0; masked <= 1; masked++) {
        reset(3); mock_primask = masked;
        counter32_set(42); assert(mock_primask == (uint32_t)masked);
        assert(counter32_get() == 42 && mock_primask == (uint32_t)masked);
        assert(counter32_start() == HAL_OK && mock_primask == (uint32_t)masked);
        counter32_stop(); assert(mock_primask == (uint32_t)masked);
    }
    puts("PASS: rollover, pending IRQ/race, repeated reads, 32-bit wrap, set/reset, stop/start, failure and PRIMASK preservation");
    return 0;
}
