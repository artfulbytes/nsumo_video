#include "drivers/mcu_init.h"
#include "drivers/io.h"
#include <msp430.h>

/* Watchdog is enabled by default and will reset the microcontroller repeatedly if not
 * explicitly stopped */
static void watchdog_stop(void)
{
    WDTCTL = WDTPW + WDTHOLD;
}

void mcu_init(void)
{
    // Must stop watchdog first before anything else
    watchdog_stop();
    io_init();
    // Enables globally
    _enable_interrupts();
}
