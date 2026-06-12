#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "pwm_generator.h"
#include "pwm_port.h"

static volatile uint32_t g_pwm_numbers = 0;
static volatile uint32_t g_pwm_shot_count = 0;
static volatile bool g_pwm_running = false;
static volatile bool g_pwm_stop_pending = false;
static volatile bool g_pwm_waiting_cc_stop = false;

/* Debug snapshot for PWM stop timing. Read by PWM:DEBUG?. */
static volatile uint32_t g_dbg_cc_cnt = 0;
static volatile uint32_t g_dbg_cc_enter_cnt = 0;
static volatile uint32_t g_dbg_cc_enter_ccr = 0;
static volatile uint32_t g_dbg_cc_enter_arr = 0;
static volatile uint32_t g_dbg_cc_enter_pa8 = 0;
static volatile uint32_t g_dbg_before_stop_cnt = 0;
static volatile uint32_t g_dbg_before_stop_pa8 = 0;
static volatile uint32_t g_dbg_after_stop_cnt = 0;
static volatile uint32_t g_dbg_after_stop_pa8 = 0;
static volatile uint32_t g_dbg_counter_smcr = 0;
static volatile uint32_t g_dbg_counter_ccmr1 = 0;
static volatile uint32_t g_dbg_counter_ccer = 0;
static volatile uint32_t g_dbg_counter_cr1 = 0;
static volatile uint32_t g_dbg_counter_cnt = 0;

extern void pwm_start_state_set(bool start);

static void pwm_debug_reset(void)
{
    g_dbg_cc_cnt = 0;
    g_dbg_cc_enter_cnt = 0;
    g_dbg_cc_enter_ccr = 0;
    g_dbg_cc_enter_arr = 0;
    g_dbg_cc_enter_pa8 = 0;
    g_dbg_before_stop_cnt = 0;
    g_dbg_before_stop_pa8 = 0;
    g_dbg_after_stop_cnt = 0;
    g_dbg_after_stop_pa8 = 0;
    g_dbg_counter_smcr = 0;
    g_dbg_counter_ccmr1 = 0;
    g_dbg_counter_ccer = 0;
    g_dbg_counter_cr1 = 0;
    g_dbg_counter_cnt = 0;
}

void pwm_generator_set_numbers(uint32_t numbers)
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

void pwm_generator_reset_shot_count(void)
{
    g_pwm_shot_count = 0;
    g_pwm_waiting_cc_stop = false;
    g_pwm_stop_pending = false;
    pwm_debug_reset();
}

uint32_t pwm_generator_get_shot_count(void)
{
    return g_pwm_shot_count;
}

void pwm_generator_set_running(bool running)
{
    g_pwm_running = running;
    if (!running) {
        g_pwm_waiting_cc_stop = false;
        g_pwm_stop_pending = false;
    }
}

bool pwm_generator_is_running(void)
{
    return g_pwm_running;
}

bool pwm_generator_start(const pwm_generator_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }

    pwm_port_stop_all();
    pwm_generator_set_running(false);

    if (!pwm_port_configure_output(cfg->freq_hz, cfg->width_us, cfg->polarity)) {
        pwm_generator_set_running(false);
        return false;
    }

    pwm_generator_set_numbers(cfg->numbers);
    pwm_generator_reset_shot_count();
    pwm_generator_set_running(true);

    if (!pwm_port_start_output()) {
        pwm_generator_set_running(false);
        return false;
    }

    return true;
}

void pwm_generator_stop(void)
{
    pwm_port_stop_all();
    pwm_generator_set_running(false);
    pwm_start_state_set(false);
}

void pwm_generator_service(void)
{
    /* No background stop processing is required in the current design. */
}

/* Backward-compatible entry point used by the existing app_main() loop. */
void pwm_service(void)
{
    pwm_generator_service();
}

void pwm_generator_on_period_elapsed(void)
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

        pwm_port_disable_update_irq();
        pwm_port_clear_compare_flag();
        pwm_port_clear_compare_pending_irq();
        pwm_port_enable_compare_irq();
    }
}

void pwm_generator_on_compare_match(void)
{
    pwm_port_snapshot_t snapshot;

    if (!g_pwm_waiting_cc_stop) {
        return;
    }

    g_dbg_cc_cnt++;

    pwm_port_capture_snapshot(&snapshot);
    g_dbg_cc_enter_cnt = snapshot.cnt;
    g_dbg_cc_enter_ccr = snapshot.ccr;
    g_dbg_cc_enter_arr = snapshot.arr;
    g_dbg_cc_enter_pa8 = snapshot.output_level;

    pwm_port_disable_compare_irq();

    pwm_port_capture_snapshot(&snapshot);
    g_dbg_before_stop_cnt = snapshot.cnt;
    g_dbg_before_stop_pa8 = snapshot.output_level;

    pwm_port_stop_output_from_isr();

    pwm_port_capture_snapshot(&snapshot);
    g_dbg_after_stop_cnt = snapshot.cnt;
    g_dbg_after_stop_pa8 = snapshot.output_level;
    g_dbg_counter_smcr = snapshot.counter_smcr;
    g_dbg_counter_ccmr1 = snapshot.counter_ccmr1;
    g_dbg_counter_ccer = snapshot.counter_ccer;
    g_dbg_counter_cr1 = snapshot.counter_cr1;
    g_dbg_counter_cnt = snapshot.counter_cnt;

    pwm_port_clear_update_flag();
    pwm_port_clear_compare_flag();
    pwm_port_clear_update_pending_irq();
    pwm_port_clear_compare_pending_irq();

    g_pwm_running = false;
    g_pwm_waiting_cc_stop = false;
    g_pwm_stop_pending = false;
    pwm_start_state_set(false);
}

void pwm_generator_counter_set(uint32_t value)
{
    pwm_port_counter_set(value);
}

uint32_t pwm_generator_counter_get(void)
{
    return pwm_port_counter_get();
}

void pwm_generator_counter_reset(void)
{
    pwm_port_counter_reset();
}

void pwm_generator_get_debug_snapshot(pwm_generator_debug_t *debug)
{
    if (debug == NULL) {
        return;
    }

    debug->cc_count = g_dbg_cc_cnt;
    debug->cc_enter_cnt = g_dbg_cc_enter_cnt;
    debug->cc_enter_ccr = g_dbg_cc_enter_ccr;
    debug->cc_enter_arr = g_dbg_cc_enter_arr;
    debug->cc_enter_pa8 = g_dbg_cc_enter_pa8;
    debug->before_stop_cnt = g_dbg_before_stop_cnt;
    debug->before_stop_pa8 = g_dbg_before_stop_pa8;
    debug->after_stop_cnt = g_dbg_after_stop_cnt;
    debug->after_stop_pa8 = g_dbg_after_stop_pa8;
    debug->counter_smcr = g_dbg_counter_smcr;
    debug->counter_ccmr1 = g_dbg_counter_ccmr1;
    debug->counter_ccer = g_dbg_counter_ccer;
    debug->counter_cr1 = g_dbg_counter_cr1;
    debug->counter_cnt = g_dbg_counter_cnt;
}
