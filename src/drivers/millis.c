#include "drivers/millis.h"
#include "common/defines.h"
#include <msp430.h>

static volatile uint32_t watchdog_interrupt_cnt = 0;

INTERRUPT_FUNCTION(WDT_VECTOR) isr_watchdog(void)
{
    watchdog_interrupt_cnt++;
}

uint32_t millis()
{
    // Disable interrupts while retrieving the counter
    IE1 &= ~WDTIE;
    // Divide by two because the watchdog timer triggers every 0.5 ms
    const uint32_t ms = watchdog_interrupt_cnt / 2;
    IE1 |= WDTIE;
    return ms;
}
