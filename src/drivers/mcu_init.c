#include "drivers/mcu_init.h"
#include "drivers/io.h"
#include "common/assert_handler.h"
#include <msp430.h>

// 16 MHz / 32768 = ~2000 Hz
#define WDT_MDLY_0_5_16MHZ (WDTPW + WDTTMSEL + WDTCNTCL + WDTIS0)

static inline void init_clocks()
{
    /* There are some variations between individual units, so TI calibrates
     * each unit during manufacturing and stores the calibration value in
     * memory to achieve a similar clock rate between different units. Sanity
     * check that the calibration data has not been erased. */
    ASSERT(CALBC1_1MHZ != 0xFF && CALBC1_16MHZ != 0xFF);

    /* Configure the internal oscillator (main clock) to run at 16 MHz.
     * This clock is used as a reference to produce a more stable DCO. */
    BCSCTL1 = CALBC1_16MHZ;

    // Sets the clock rate of the digitally controlled oscillator (DCO)
    DCOCTL = CALDCO_16MHZ;

    /* Set DCO as source for
     * MCLK: Master clock drives the CPU and some peripherals
     * SMCLK: Subsystem master clock drives some peripherals */
    // BCSCTL2 default

    // Select the internal Very Low Frequency oscillator (VLO) as ACLK source
    BCSCTL3 = LFXT1S_2;
}

static inline void watchdog_setup(void)
{
    // Stop watchdog
    WDTCTL = WDTPW + WDTHOLD;

    // Re-purpose watchdog to count milliseconds instead (see millis.c)
    WDTCTL = WDT_MDLY_0_5_16MHZ;
    IE1 |= WDTIE;
}

void mcu_init(void)
{
    // Must stop and configure watchdog before anything else
    watchdog_setup();
    init_clocks();
    io_init();
    // Enables globally
    _enable_interrupts();
}
