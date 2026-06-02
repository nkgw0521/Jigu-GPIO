#ifndef PWM_PORT_H_
#define PWM_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t cnt;
    uint32_t ccr;
    uint32_t arr;
    uint32_t output_level;
} pwm_port_snapshot_t;

bool pwm_port_configure_output(uint32_t freq_hz, uint32_t width_us, uint32_t polarity);
bool pwm_port_start_output(void);
void pwm_port_stop_all(void);
void pwm_port_stop_output_from_isr(void);

void pwm_port_disable_update_irq(void);
void pwm_port_enable_compare_irq(void);
void pwm_port_disable_compare_irq(void);
void pwm_port_clear_update_flag(void);
void pwm_port_clear_compare_flag(void);
void pwm_port_clear_update_pending_irq(void);
void pwm_port_clear_compare_pending_irq(void);

void pwm_port_capture_snapshot(pwm_port_snapshot_t *snapshot);

void pwm_port_counter_set(uint32_t value);
uint32_t pwm_port_counter_get(void);
void pwm_port_counter_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* PWM_PORT_H_ */
