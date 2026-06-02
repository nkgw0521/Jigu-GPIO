#ifndef PWM_GENERATOR_H_
#define PWM_GENERATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t freq_hz;
    uint32_t width_us;
    uint32_t polarity;   /* 0: active low, 1: active high */
    uint32_t numbers;    /* 0: continuous */
} pwm_generator_config_t;

typedef struct {
    uint32_t cc_count;
    uint32_t cc_enter_cnt;
    uint32_t cc_enter_ccr;
    uint32_t cc_enter_arr;
    uint32_t cc_enter_pa8;
    uint32_t before_stop_cnt;
    uint32_t before_stop_pa8;
    uint32_t after_stop_cnt;
    uint32_t after_stop_pa8;
} pwm_generator_debug_t;

bool pwm_generator_start(const pwm_generator_config_t *cfg);
void pwm_generator_stop(void);

void pwm_generator_set_numbers(uint32_t numbers);
void pwm_generator_reset_shot_count(void);
uint32_t pwm_generator_get_shot_count(void);
void pwm_generator_set_running(bool running);
bool pwm_generator_is_running(void);

void pwm_generator_on_period_elapsed(void);
void pwm_generator_on_compare_match(void);
void pwm_generator_service(void);
/* Backward-compatible wrapper for existing application code. */
void pwm_service(void);

void pwm_generator_counter_set(uint32_t value);
uint32_t pwm_generator_counter_get(void);
void pwm_generator_counter_reset(void);

void pwm_generator_get_debug_snapshot(pwm_generator_debug_t *debug);

#ifdef __cplusplus
}
#endif

#endif /* PWM_GENERATOR_H_ */
