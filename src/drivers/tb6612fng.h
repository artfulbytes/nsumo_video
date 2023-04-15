#ifndef TB6612FNG_H
#define TB6612FNG_H

// Driver for motor driver TB6612FNG

#include <stdint.h>

typedef enum
{
    TB6612FNG_LEFT,
    TB6612FNG_RIGHT
} tb6612fng_e;

typedef enum
{
    TB6612FNG_MODE_STOP,
    TB6612FNG_MODE_FORWARD, // Clockwise (CC)
    TB6612FNG_MODE_REVERSE, // Counterclockwise (CCW)
} tb6612fng_mode_e;

void tb6612fng_init(void);
void tb6612fng_set_mode(tb6612fng_e tb, tb6612fng_mode_e mode);
void tb6612fng_set_pwm(tb6612fng_e tb, uint8_t duty_cycle);

#endif
