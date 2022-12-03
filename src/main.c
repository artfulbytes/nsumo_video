#include <msp430.h>

static void test_blink_led(void)
{
    // TODO: Use io functions
    P1DIR |= BIT0;
    volatile unsigned int i; // volatile to prevent optimization
    while (1) {
        P1OUT ^= BIT0;
        for (i = 10000; i > 0; i--) { } // delay
    }
}

int main(void)
{
    // TODO: Move to mcu_init
    WDTCTL = WDTPW + WDTHOLD; // stop watchdog timer
    test_blink_led();
    return 0;
}
