#include "drivers/io.h"
#include "drivers/mcu_init.h"
#include "drivers/led.h"
#include "common/assert_handler.h"
#include "common/defines.h"
#include <msp430.h>

static void test_setup(void)
{
    mcu_init();
}

/*
// TODO: Move to test file
static void test_assert(void)
{
    test_setup();
    ASSERT(0);
}
*/

// TODO: Move to test file
static void test_blink_led(void)
{
    test_setup();
    led_init();
    led_state_e led_state = LED_STATE_OFF;
    while (1) {
        led_state = (led_state == LED_STATE_OFF) ? LED_STATE_ON : LED_STATE_OFF;
        led_set(LED_TEST, led_state);
        BUSY_WAIT_ms(1000);
    }
}

/*
// Configure all pins as output and then toggle them in a loop. Verify with a logic analyzer.
// TODO: Move to test file
static void test_launchpad_io_pins_output(void)
{
    test_setup();
    const struct io_config output_config = { .select = IO_SELECT_GPIO,
                                            .resistor = IO_RESISTOR_DISABLED,
                                            .dir = IO_DIR_OUTPUT,
                                            .out = IO_OUT_LOW };

    // Configure all pins as output
    for (io_generic_e io = IO_10; io <= IO_27; io++)
    {
        io_configure(io, &output_config);
    }
    while (1) {
        for (io_generic_e io = IO_10; io <= IO_27; io++) {
            io_set_out(io, IO_OUT_HIGH);
            BUSY_WAIT_ms(10);
            io_set_out(io, IO_OUT_LOW);
        }
    }
}
*/

/* Configure all pins except one (pin 1.0) as input with internal pull-up resistors. Configure
 * the exception (pin 1.0) as output to control an LED. Verify by pulling each pin down in
 * increasing order with an external pull-down resistor. LED state changes when the right pin is
 * pulled down. Once all pins have been verified OK, the LED blinks repeatedly.
 *
 * Note, the pins are configured with internal pull-up resistors (instead of pull-down) because
 * some pins on the LAUNCHPAD are already pulled up by external circuitry */
// TODO: Move to test file
/*
static void test_launchpad_io_pins_input(void)
{
    test_setup();
    led_init();
    const struct io_config input_config = {
        .select = IO_SELECT_GPIO,
        .resistor = IO_RESISTOR_ENABLED,
        .dir = IO_DIR_INPUT,
        .out = IO_OUT_HIGH // pull-up
    };

    // Configure all pins as input
    for (io_generic_e io = IO_10; io <= IO_27; io++) {
        io_configure(io, &input_config);
    }

    for (io_generic_e io = IO_10; io <= IO_27; io++) {
        if (io == (io_generic_e)IO_TEST_LED) {
            continue;
        }
        led_set(LED_TEST, LED_STATE_ON);
        // Wait for user to pull pin low
        while (io_get_input(io) == IO_IN_HIGH) {
            BUSY_WAIT_ms(100);
        }
        led_set(LED_TEST, LED_STATE_OFF);
        // Wait for user to disconnect
        while (io_get_input(io) == IO_IN_LOW) {
            BUSY_WAIT_ms(100);
        }
    }

    // Blink LED when test is done
    while (1) {
        led_set(LED_TEST, LED_STATE_ON);
        BUSY_WAIT_ms(500);
        led_set(LED_TEST, LED_STATE_OFF);
        BUSY_WAIT_ms(2000);
    }
}
*/
int main(void)
{
    // test_assert();
    test_blink_led();
    // test_launchpad_io_pins_output();
    // test_launchpad_io_pins_input();
    return 0;
}
