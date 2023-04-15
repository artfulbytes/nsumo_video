#ifndef PWM_H
#define PWM_H

// Driver that emulates hardware PWM with timer peripheral

#include <stdint.h>

typedef enum
{
    PWM_TB6612FNG_LEFT,
    PWM_TB6612FNG_RIGHT
} pwm_e;

void pwm_init(void);
void pwm_set_duty_cycle(pwm_e pwm, uint8_t duty_cycle_percent);

#endif // PWM_H
