#include "drivers/pwm.h"
#include "drivers/io.h"
#include "common/assert_handler.h"
#include "common/defines.h"
#include <msp430.h>
#include <stdbool.h>
#include <assert.h>

/* MSP430G2553 has no dedicated PWM, so use timer A0 to emulate
 * hardware PWM. Each timer has three capture/compare channels
 * (CC) and first channel must be sacrificed for setting the
 * base period (TA0CCR). The two other channels are used for
 * one PWM output each and duty cycle is set by setting the
 * timer value (TA0CCR1 and TA0CCR2). The CC outputs are muxed
 * to corresponding IO pins (see io.h).
 *
 * Example (one period):
 * -----------------_____________ // CC output
 * <----TA0CCRx---->              // Duty cycle
 * <----------TA0CCR0-----------> // Base period
 *
 * Set the base frequency to 20000 Hz because with SMCLK of 16 MHz it
 * gives a base period of 100 ticks, which means the duty cycle
 * percent corresponds to the TA0CCRx directly without any conversion.
 * 20 kHz also gives stable motor behaviour. */
#define PWM_TIMER_FREQ_HZ (SMCLK / TIMER_INPUT_DIVIDER_3)
#define PWM_PERIOD_FREQ_HZ (20000)
#define PWM_PERIOD_TICKS (PWM_TIMER_FREQ_HZ / PWM_PERIOD_FREQ_HZ)
static_assert(PWM_PERIOD_TICKS == 100, "Expect 100 ticks per period");
// Timer counts from 0, so should decrement by 1
#define PWM_TA0CCR0 (PWM_PERIOD_TICKS - 1)

struct pwm_channel_cfg
{
    bool enabled;
    volatile unsigned int *const cctl;
    volatile unsigned int *const ccr;
};

static struct pwm_channel_cfg pwm_cfgs[] = {
    [PWM_TB6612FNG_LEFT] = { .enabled = false, .cctl = &TA0CCTL1, .ccr = &TA0CCR1 },
    [PWM_TB6612FNG_RIGHT] = { .enabled = false, .cctl = &TA0CCTL2, .ccr = &TA0CCR2 },
};

static bool pwm_all_channels_disabled(void)
{
    for (uint8_t ch = 0; ch < ARRAY_SIZE(pwm_cfgs); ch++) {
        if (pwm_cfgs[ch].enabled) {
            return false;
        }
    }
    return true;
}

static bool pwm_enabled = false;
static void pwm_enable(bool enable)
{
    if (pwm_enabled != enable) {
        /* MC_0: Stop
         * MC_1: Count to TACCR0 */
        TA0CTL = (TA0CTL & ~TIMER_MC_MASK) + TACLR + (enable ? MC_1 : MC_0);
        pwm_enabled = enable;
    }
}

static void pwm_channel_enable(pwm_e pwm, bool enable)
{
    if (pwm_cfgs[pwm].enabled != enable) {
        /* OUTMOD_7: Reset/Set
         * OUTMOD_0: Off */
        *pwm_cfgs[pwm].cctl = enable ? OUTMOD_7 : OUTMOD_0;
        pwm_cfgs[pwm].enabled = enable;
        if (enable) {
            pwm_enable(true);
        } else if (pwm_all_channels_disabled()) {
            pwm_enable(false);
        }
    }
}

static inline uint8_t pwm_scale_duty_cycle(uint8_t duty_cycle_percent)
{
    /* Battery is at ~8 V when fully charged and motors are 6 V max,
     * so scale down the duty cycle by 25% to be within specs. This
     * should never return 0. */
    return duty_cycle_percent == 1 ? duty_cycle_percent : duty_cycle_percent * 3 / 4;
}

void pwm_set_duty_cycle(pwm_e pwm, uint8_t duty_cycle_percent)
{
    ASSERT(duty_cycle_percent <= 100);
#if defined(LAUNCHPAD)
    // Not supported
    if (pwm == PWM_TB6612FNG_RIGHT) {
        return;
    }
#endif
    const bool enable = duty_cycle_percent > 0;
    if (enable) {
        *pwm_cfgs[pwm].ccr = pwm_scale_duty_cycle(duty_cycle_percent);
    }
    pwm_channel_enable(pwm, enable);
}

static const struct io_config pwm_io_config = {
    .select = IO_SELECT_ALT1,
    .resistor = IO_RESISTOR_DISABLED,
    .dir = IO_DIR_OUTPUT,
    .out = IO_OUT_LOW,
};

static bool initialized = false;
void pwm_init(void)
{
    ASSERT(!initialized);
    struct io_config current_config;
    io_get_current_config(IO_PWM_MOTORS_LEFT, &current_config);
    ASSERT(io_config_compare(&current_config, &pwm_io_config));
#if defined(NSUMO)
    io_get_current_config(IO_PWM_MOTORS_RIGHT, &current_config);
    ASSERT(io_config_compare(&current_config, &pwm_io_config));
#endif

    /* TASSEL_2: Clock source SMCLK
     * ID_3: Input divider /8
     * MC_0: Stopped */
    TA0CTL = TASSEL_2 + ID_3 + MC_0;
    // Set period
    TA0CCR0 = PWM_TA0CCR0;

    initialized = true;
}
