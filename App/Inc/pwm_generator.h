#ifndef PWM_GENERATOR_H_
#define PWM_GENERATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

void pwm_setup(uint32_t freq, uint32_t width, uint32_t polarity);
void pwm_numbers_set(uint32_t numbers);
void pwm_shot_count_reset(void);
uint32_t pwm_shot_count_get(void);
void pwm_run_state_set(bool running);
bool pwm_is_running(void);
void pwm_service(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_GENERATOR_H_ */
