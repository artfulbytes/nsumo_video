#include "app/timer.h"
#include "drivers/millis.h"
#include "common/defines.h"

#define TIMER_CLEARED (0u)

void timer_start(timer_t *timer, uint32_t timeout_ms)
{
    *timer = millis() + timeout_ms;
}

bool timer_timeout(const timer_t *timer)
{
    if (*timer == TIMER_CLEARED) {
        return false;
    }
    return millis() > *timer;
}

void timer_clear(timer_t *timer)
{
    *timer = TIMER_CLEARED;
}
