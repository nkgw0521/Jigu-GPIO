#ifndef PWM_PORT_H_
#define PWM_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "main.h"

typedef struct {
    uint32_t freq_hz;
    uint32_t width_us;
    uint32_t polarity;   /* 0: active low, 1: active high */
    uint32_t numbers;    /* 0: continuous */
} pwm_port_config_t;

HAL_StatusTypeDef pwm_port_start(const pwm_port_config_t *cfg);
void pwm_port_stop(void);

void pwm_port_counter_set(uint32_t value);
uint32_t pwm_port_counter_get(void);
void pwm_port_counter_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_PORT_H_ */
