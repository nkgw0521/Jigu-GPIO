#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <limits.h>

#include "pwm_generator.h"
#include "tim.h"

static volatile uint32_t g_pwm_numbers = 0;
static volatile uint32_t g_pwm_shot_count = 0;
static volatile bool g_pwm_running = false;
static volatile bool g_pwm_stop_pending = false;
static volatile bool g_pwm_waiting_cc_stop = false;

/* Debug snapshot for PWM stop timing. Read by PWM:DEBUG?. */
volatile uint32_t g_dbg_cc_cnt = 0;
volatile uint32_t g_dbg_cc_enter_cnt = 0;
volatile uint32_t g_dbg_cc_enter_ccr = 0;
volatile uint32_t g_dbg_cc_enter_arr = 0;
volatile uint32_t g_dbg_cc_enter_pa8 = 0;
volatile uint32_t g_dbg_before_stop_cnt = 0;
volatile uint32_t g_dbg_before_stop_pa8 = 0;
volatile uint32_t g_dbg_after_stop_cnt = 0;
volatile uint32_t g_dbg_after_stop_pa8 = 0;

extern void pwm_start_state_set(bool start);


void pwm_numbers_set(uint32_t numbers)
{
    /*
     * Update IRQ occurs at the start of the next PWM period.
     * To stop after N visible pulses without cutting the final width,
     * arm the stop at update count N-1, then complete the stop at CC1
     * of the final pulse.
     *
     * numbers == 0 : continuous output
     * numbers == 1 : boundary case, kept as-is
     */
    if (numbers > 1U) {
        g_pwm_numbers = numbers - 1U;
    } else {
        g_pwm_numbers = numbers;
    }
}

void pwm_shot_count_reset(void)
{
    g_pwm_shot_count = 0;
    g_pwm_waiting_cc_stop = false;
    g_pwm_stop_pending = false;

    g_dbg_cc_cnt = 0;
    g_dbg_cc_enter_cnt = 0;
    g_dbg_cc_enter_ccr = 0;
    g_dbg_cc_enter_arr = 0;
    g_dbg_cc_enter_pa8 = 0;
    g_dbg_before_stop_cnt = 0;
    g_dbg_before_stop_pa8 = 0;
    g_dbg_after_stop_cnt = 0;
    g_dbg_after_stop_pa8 = 0;
}

uint32_t pwm_shot_count_get(void)
{
    return g_pwm_shot_count;
}

void pwm_run_state_set(bool running)
{
    g_pwm_running = running;
    if (!running) {
        g_pwm_waiting_cc_stop = false;
        g_pwm_stop_pending = false;
    }
}

bool pwm_is_running(void)
{
    return g_pwm_running;
}

static uint32_t tim1_get_clk_hz(void)
{
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();

    RCC_ClkInitTypeDef clkconfig;
    uint32_t flash_latency = 0;
    HAL_RCC_GetClockConfig(&clkconfig, &flash_latency);

    if (clkconfig.APB2CLKDivider == RCC_HCLK_DIV1) {
        return pclk2;
    } else {
        return pclk2 * 2U;
    }
}

static bool tim1_find_best_freq(uint32_t target_freq_hz,
                                uint16_t *out_psc,
                                uint16_t *out_arr,
                                uint32_t *out_actual_freq_hz)
{
    if ((target_freq_hz == 0U) || (out_psc == NULL) || (out_arr == NULL) || (out_actual_freq_hz == NULL)) {
        return false;
    }

    uint32_t tim_clk = tim1_get_clk_hz();
    uint64_t best_err = UINT64_MAX;
    bool found = false;

    for (uint32_t psc = 0; psc <= 65535U; psc++) {
        uint64_t div_psc = (uint64_t)psc + 1ULL;

        uint64_t arr_plus_1 = ((uint64_t)tim_clk + ((uint64_t)target_freq_hz * div_psc) / 2ULL)
                            / ((uint64_t)target_freq_hz * div_psc);

        if (arr_plus_1 == 0ULL || arr_plus_1 > 65536ULL) {
            continue;
        }

        uint32_t actual_freq = (uint32_t)((uint64_t)tim_clk / (div_psc * arr_plus_1));
        uint64_t err = (actual_freq > target_freq_hz) ? (actual_freq - target_freq_hz)
                                                      : (target_freq_hz - actual_freq);

        if (err < best_err) {
            best_err = err;
            *out_psc = (uint16_t)psc;
            *out_arr = (uint16_t)(arr_plus_1 - 1ULL);
            *out_actual_freq_hz = actual_freq;
            found = true;

            if (err == 0ULL) {
                break;
            }
        }
    }

    return found;
}


/**
 * @brief PWM Setup for Pulse generator
 * 
 * @param freq        PWM Frequency(Hz)
 * @param width       PWM Width(us)
 * @param polarity    PWM Polarity [0:Low/1:High]
 * @return * void 
 */
void pwm_setup( uint32_t freq, uint32_t width, uint32_t polarity )
{
  TIM_OC_InitTypeDef sConfigOC = {0};

  if (freq == 0U) {
      return ;
  }

  uint32_t period_us = 1000000UL / freq;
  if (width == 0U || width >= period_us) {
      return ;
  }

  uint16_t psc = 0;
  uint16_t arr = 0;
  uint32_t actual_freq_hz = 0;

  if (!tim1_find_best_freq(freq, &psc, &arr, &actual_freq_hz)) {
      return ;
  }

  /* CCR = round((ARR+1) * pulse_us * actual_freq / 1e6) */
  uint64_t ccr64 = ((uint64_t)(arr + 1U) * (uint64_t)width * (uint64_t)actual_freq_hz + 500000ULL)
                 / 1000000ULL;

  if (ccr64 == 0ULL) {
      ccr64 = 1ULL;
  }
  if (ccr64 > (uint64_t)(arr + 1U)) {
      ccr64 = (uint64_t)(arr + 1U);
  }

  uint16_t ccr = (uint16_t)ccr64;


  htim1.Init.Prescaler = psc;
  htim1.Init.Period = arr;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = ccr;
  sConfigOC.OCPolarity = (polarity == 0) ? TIM_OCPOLARITY_LOW : TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = (polarity == 0) ? TIM_OCIDLESTATE_SET : TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
#if 0
  HAL_TIM_MspPostInit(&htim1);
#endif
}

void pwm_service(void)
{
    /* No background stop processing is required in the current design. */
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        if (!g_pwm_running) {
            return;
        }

        if (g_pwm_waiting_cc_stop) {
            return;
        }

        g_pwm_shot_count++;

        if ((g_pwm_numbers > 0U) && (g_pwm_shot_count >= g_pwm_numbers))
        {
            /*
             * The final visible pulse has just started at this update event.
             * Do not stop PWM here; stopping here cuts the final pulse width.
             * Instead, disable further update counting and stop at CH1 compare,
             * which is the end of the active pulse width in PWM mode 1.
             */
            g_pwm_waiting_cc_stop = true;
            g_pwm_stop_pending = true;

            __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
            __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1);
            HAL_NVIC_ClearPendingIRQ(TIM1_CC_IRQn);
            __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_CC1);
        }
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        if (htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) {
            return;
        }

        if (!g_pwm_waiting_cc_stop) {
            return;
        }

        g_dbg_cc_cnt++;
        g_dbg_cc_enter_cnt = TIM1->CNT;
        g_dbg_cc_enter_ccr = TIM1->CCR1;
        g_dbg_cc_enter_arr = TIM1->ARR;
        g_dbg_cc_enter_pa8 = (GPIOA->IDR & GPIO_PIN_8) ? 1U : 0U;

        __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_CC1);

        g_dbg_before_stop_cnt = TIM1->CNT;
        g_dbg_before_stop_pa8 = (GPIOA->IDR & GPIO_PIN_8) ? 1U : 0U;

        HAL_TIM_Base_Stop_IT(&htim1);
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);

        g_dbg_after_stop_cnt = TIM1->CNT;
        g_dbg_after_stop_pa8 = (GPIOA->IDR & GPIO_PIN_8) ? 1U : 0U;

        __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
        __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1);
        HAL_NVIC_ClearPendingIRQ(TIM1_UP_TIM16_IRQn);
        HAL_NVIC_ClearPendingIRQ(TIM1_CC_IRQn);

        g_pwm_running = false;
        g_pwm_waiting_cc_stop = false;
        g_pwm_stop_pending = false;
        pwm_start_state_set(false);
    }
}
