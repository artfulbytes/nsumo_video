#include "drivers/mcu_init.h"
#include "drivers/io.h"
#include "common/assert_handler.h"
#include <msp430.h>

static void init_clocks()
{
    /* There are some variations between individual units, so TI calibrates
     * each unit during manufacturing and stores the calibration value in
     * memory to achieve a similar clock rate between different units. Sanity
     * check that the calibration data has not been erased. */
    ASSERT(CALBC1_1MHZ != 0xFF && CALBC1_16MHZ != 0xFF);

    /* Configure the internal oscillator (main clock) to run at 16 MHz.
     * This clock is used as a reference to produce a more stable DCO. */
    BCSCTL1 |= CALBC1_16MHZ;

    // Sets the clock rate of the digitally controlled oscillator (DCO)
    DCOCTL |= CALDCO_16MHZ;

    /* Set DCO as source for
     * MCLK: Master clock drives the CPU and some peripherals
     * SMCLK: Subsystem master clock drives some peripherals */
    // BCSCTL2 default
}

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
    init_clocks();
    io_init();
    // Enables globally
    _enable_interrupts();
}
