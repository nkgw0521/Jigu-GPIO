#include <assert.h>
#include <stdio.h>
#include "board_config.h"
#include "pwm_generator.h"
#include "pwm_port.h"
static TIM_TypeDef pwm_regs,counter_regs;
TIM_HandleTypeDef mock_pwm={.Instance=&pwm_regs},mock_counter={.Instance=&counter_regs};
GPIO_TypeDef mock_gpio;
uint32_t mock_irqmask,mock_active_ccr;
static bool scpi_running;
static unsigned synchronized_starts;
static HAL_StatusTypeDef start_result=HAL_OK;
static bool complete_on_unmask;
static unsigned run_pulses(unsigned max);
void mock_restore_irq(uint32_t value){
 mock_irqmask=value;
 if(value==0&&complete_on_unmask){complete_on_unmask=false;assert(run_pulses(3)==1);}
}
void pwm_start_state_set(bool value){scpi_running=value;}
void HAL_GPIO_WritePin(GPIO_TypeDef*g,uint32_t p,GPIO_PinState s){if(s)g->ODR|=p;else g->ODR&=~p;g->IDR=g->ODR;}
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef*c,uint32_t*f){c->APB2CLKDivider=RCC_HCLK_DIV1;*f=0;}
void HAL_NVIC_ClearPendingIRQ(int i){(void)i;}
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef*h){h->Instance->ARR=h->Init.Period;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef*h,TIM_OC_InitTypeDef*o,uint32_t c){(void)c;MOCK_CCR(h)=o->Pulse;h->Instance->CCER=o->OCPolarity?16U:0U;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef*h){h->Instance->CR1|=1;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef*h){h->Instance->CR1&=~1U;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef*h){h->Instance->CR1&=~1U;h->Instance->DIER&=~1U;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef*h,uint32_t c){
 (void)c;assert(mock_active_ccr==0); /* Enabling the output must not start a pulse. */
 assert(MOCK_CCR(h)==0); /* Even a blank-phase overflow must stay inactive. */
 assert(((mock_gpio.MODER>>(TEST_PIN*2))&3U)==1U);
 assert(!!(mock_gpio.ODR&PWM_OUTPUT_GPIO_PIN)==!(h->Instance->CCER&16U));
 assert((h->Instance->DIER&3U)==0);assert(mock_irqmask==1U);
 if(start_result!=HAL_OK)return start_result;
 h->Instance->CCER|=32U;h->Instance->CR1|=1;mock_active_ccr=MOCK_CCR(h);h->Instance->SR|=3U;return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef*h,uint32_t c){(void)c;h->Instance->CR1&=~1U;h->Instance->CCER&=~32U;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef*h,uint32_t e){
 (void)e;assert(h->Instance->CR1&TIM_CR1_URS);
 mock_active_ccr=MOCK_CCR(h);h->Instance->CNT=0;
 if(h->Instance->CR1&1U){assert(h->Instance->CCER&32U);assert(((mock_gpio.MODER>>(TEST_PIN*2))&3U)==2U);assert((h->Instance->SR&3U)==0);assert(mock_active_ccr!=0);synchronized_starts++;}
 /* URS must keep both software UGs out of period counting. */
 assert(pwm_generator_get_shot_count()==0);
 return HAL_OK;
}
static unsigned run_pulses(unsigned max)
{
 unsigned n=0;
 while((pwm_regs.CR1&1U)&&n<max){
  if(mock_active_ccr!=0){counter_regs.CNT++;n++;}
  if(pwm_regs.DIER&PWM_COMPARE_IT){mock_pwm.Channel=TEST_CHANNEL;HAL_TIM_PWM_PulseFinishedCallback(&mock_pwm);}
  if(pwm_regs.CR1&1U){mock_active_ccr=MOCK_CCR(&mock_pwm);if(pwm_regs.DIER&PWM_UPDATE_IT)HAL_TIM_PeriodElapsedCallback(&mock_pwm);}
 }
 return n;
}
int main(void)
{
 for(unsigned pol=0;pol<=1;pol++){
  const unsigned counts[]={1,2,3,10,1};
  for(unsigned i=0;i<5;i++){
   pwm_generator_config_t cfg={1000,100,pol,counts[i]};
   pwm_port_counter_reset();scpi_running=true;
   unsigned starts_before=synchronized_starts;
   assert(pwm_generator_start(&cfg));
   assert(synchronized_starts==starts_before+1);
   assert(mock_irqmask==0);
   assert(((mock_gpio.MODER>>(TEST_PIN*2))&3U)==2U);
   assert(mock_active_ccr!=0); /* UG loaded actual first width before zero preload. */
   if(cfg.numbers==1){assert(MOCK_CCR(&mock_pwm)==0);assert((pwm_regs.DIER&3U)==PWM_COMPARE_IT);}
   unsigned actual=run_pulses(20);
   assert(actual==cfg.numbers&&pwm_port_counter_get()==cfg.numbers);
   assert(!pwm_generator_is_running()&&!scpi_running);
   assert(((mock_gpio.MODER>>(TEST_PIN*2))&3U)==1U);
   assert(!!(mock_gpio.ODR&PWM_OUTPUT_GPIO_PIN)==!pol);
   assert(run_pulses(3)==0); /* No pulse after stop. */
  }
  pwm_generator_config_t continuous={1000,100,pol,0};
  assert(pwm_generator_start(&continuous));assert(run_pulses(5)==5);assert(pwm_generator_is_running());
  pwm_generator_stop();assert(!pwm_generator_is_running());assert(run_pulses(3)==0);
 }
 /* Preserve callers that already disabled interrupts and clean up failed starts. */
 pwm_generator_config_t one={1000,1,1,1};
 mock_irqmask=1;assert(pwm_generator_start(&one));assert(mock_irqmask==1);assert(run_pulses(3)==1);
 mock_irqmask=0;complete_on_unmask=true;
 assert(pwm_generator_start(&one));assert(!pwm_generator_is_running()&&!scpi_running);
 start_result=HAL_ERROR;
 assert(!pwm_generator_start(&one));assert(!pwm_generator_is_running());assert(mock_irqmask==0);
 assert((pwm_regs.CR1&1U)==0);assert((counter_regs.CR1&1U)==0);
 assert(((mock_gpio.MODER>>(TEST_PIN*2))&3U)==1U);
 printf("PASS: channel %d / pin %d: 1,2,3,10/restart/continuous, first width preload and stable idle, both polarities\n",TEST_CHANNEL,TEST_PIN);
 return 0;
}
