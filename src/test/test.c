#include "drivers/io.h"
#include "drivers/mcu_init.h"
#include "drivers/led.h"
#include "drivers/uart.h"
#include "drivers/ir_remote.h"
#include "drivers/pwm.h"
#include "drivers/tb6612fng.h"
#include "drivers/adc.h"
#include "drivers/qre1113.h"
#include "app/drive.h"
#include "app/line.h"
#include "common/assert_handler.h"
#include "common/defines.h"
#include <msp430.h>
#include "common/trace.h"

SUPPRESS_UNUSED
static void test_setup(void)
{
    mcu_init();
}

SUPPRESS_UNUSED
static void test_assert(void)
{
    test_setup();
    ASSERT(0);
}

SUPPRESS_UNUSED
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

// Configure all pins as output and then toggle them in a loop. Verify with a logic analyzer.
SUPPRESS_UNUSED
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

/* Configure all pins except one (pin 1.0) as input with internal pull-up resistors. Configure
 * the exception (pin 1.0) as output to control an LED. Verify by pulling each pin down in
 * increasing order with an external pull-down resistor. LED state changes when the right pin is
 * pulled down. Once all pins have been verified OK, the LED blinks repeatedly.
 *
 * Note, the pins are configured with internal pull-up resistors (instead of pull-down) because
 * some pins on the LAUNCHPAD are already pulled up by external circuitry */
SUPPRESS_UNUSED
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

SUPPRESS_UNUSED
static void io_11_isr(void)
{
    led_set(LED_TEST, LED_STATE_ON);
}

SUPPRESS_UNUSED
static void io_20_isr(void)
{
    led_set(LED_TEST, LED_STATE_OFF);
}

SUPPRESS_UNUSED
static void test_io_interrupt(void)
{
    test_setup();
    const struct io_config input_config = {
        .select = IO_SELECT_GPIO,
        .resistor = IO_RESISTOR_ENABLED,
        .dir = IO_DIR_INPUT,
        .out = IO_OUT_HIGH // pull-up
    };
    io_configure(IO_11, &input_config);
    io_configure(IO_20, &input_config);
    led_init();
    io_configure_interrupt(IO_11, IO_TRIGGER_FALLING, io_11_isr);
    io_configure_interrupt(IO_20, IO_TRIGGER_FALLING, io_20_isr);
    io_enable_interrupt(IO_11);
    io_enable_interrupt(IO_20);
    while(1);
}

SUPPRESS_UNUSED
static void test_uart(void)
{
    test_setup();
    uart_init();
    while (1) {
        _putchar('A');
        _putchar('R');
        _putchar('T');
        _putchar('F');
        _putchar('U');
        _putchar('L');
        _putchar('\n');
        BUSY_WAIT_ms(100);
    }
}

SUPPRESS_UNUSED
static void test_trace(void)
{
    test_setup();
    trace_init();
    while (1) {
        TRACE("Artful bytes %d", 2023);
        BUSY_WAIT_ms(100);
    }
}

SUPPRESS_UNUSED
static void test_ir_remote(void)
{
    test_setup();
    trace_init();
    ir_remote_init();
    while (1) {
        TRACE("Command %s", ir_remote_cmd_to_string(ir_remote_get_cmd()));
        BUSY_WAIT_ms(250);
    }
}

SUPPRESS_UNUSED
static void test_pwm(void)
{
    test_setup();
    trace_init();
    pwm_init();
    const uint8_t duty_cycles[] = { 100, 75, 50, 25, 1, 0 };
    const uint16_t wait_time = 3000;
    while (1) {
        for (uint8_t i = 0; i < ARRAY_SIZE(duty_cycles); i++) {
            TRACE("Set duty cycle to %d for %d ms", duty_cycles[i], wait_time);
            pwm_set_duty_cycle(PWM_TB6612FNG_LEFT, duty_cycles[i]);
            pwm_set_duty_cycle(PWM_TB6612FNG_RIGHT, duty_cycles[i]);
            BUSY_WAIT_ms(wait_time);
        }
    }
}

SUPPRESS_UNUSED
static void test_tb6612fng(void)
{
    test_setup();
    trace_init();
    tb6612fng_init();
    const tb6612fng_mode_e modes[] =
    {
        TB6612FNG_MODE_FORWARD,
        TB6612FNG_MODE_REVERSE,
        TB6612FNG_MODE_FORWARD,
        TB6612FNG_MODE_REVERSE,
    };
    const uint8_t duty_cycles[] = { 100, 50, 25, 0 };
    while (1) {
        for (uint8_t i = 0; i < ARRAY_SIZE(duty_cycles); i++)
        {
            TRACE("Set mode %d and duty cycle %d", modes[i], duty_cycles[i]);
            tb6612fng_set_mode(TB6612FNG_LEFT, modes[i]);
            tb6612fng_set_mode(TB6612FNG_RIGHT, modes[i]);
            tb6612fng_set_pwm(TB6612FNG_LEFT, duty_cycles[i]);
            tb6612fng_set_pwm(TB6612FNG_RIGHT, duty_cycles[i]);
            BUSY_WAIT_ms(3000);
            tb6612fng_set_mode(TB6612FNG_LEFT, TB6612FNG_MODE_STOP);
            tb6612fng_set_mode(TB6612FNG_RIGHT, TB6612FNG_MODE_STOP);
            BUSY_WAIT_ms(1000);
        }
    }
}

SUPPRESS_UNUSED
static void test_drive(void)
{
    test_setup();
    trace_init();
    drive_init();
    ir_remote_init();
    drive_speed_e speed = DRIVE_SPEED_SLOW;
    drive_dir_e dir = DRIVE_DIR_FORWARD;
    while (1) {
        BUSY_WAIT_ms(100);
        ir_cmd_e cmd = ir_remote_get_cmd();
        switch (cmd) {
        case IR_CMD_0:
            drive_stop();
            continue;
        case IR_CMD_1:
            speed = DRIVE_SPEED_SLOW;
            break;
        case IR_CMD_2:
            speed = DRIVE_SPEED_MEDIUM;
            break;
        case IR_CMD_3:
            speed = DRIVE_SPEED_FAST;
            break;
        case IR_CMD_4:
            speed = DRIVE_SPEED_MAX;
            break;
        case IR_CMD_UP:
            dir = DRIVE_DIR_FORWARD;
            break;
        case IR_CMD_DOWN:
            dir = DRIVE_DIR_REVERSE;
            break;
        case IR_CMD_LEFT:
            dir = DRIVE_DIR_ROTATE_LEFT;
            break;
        case IR_CMD_RIGHT:
            dir = DRIVE_DIR_ROTATE_RIGHT;
            break;
        case IR_CMD_5:
        case IR_CMD_6:
        case IR_CMD_7:
        case IR_CMD_8:
        case IR_CMD_9:
        case IR_CMD_STAR:
        case IR_CMD_HASH:
        case IR_CMD_OK:
        case IR_CMD_NONE:
            continue;
        }
        drive_set(dir, speed);
    }
}

SUPPRESS_UNUSED
static void test_assert_motors(void)
{
    test_setup();
    drive_init();
    drive_set(DRIVE_DIR_FORWARD, DRIVE_SPEED_MAX);
    BUSY_WAIT_ms(3000);
    ASSERT(0);
    while(0) { }
}

SUPPRESS_UNUSED
static void test_adc(void)
{
    test_setup();
    trace_init();
    adc_init();
    while (1) {
        adc_channel_values_t values;
        adc_get_channel_values(values);
        for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
            TRACE("ADC ch %u: %u", i, values[i]);
        }
        BUSY_WAIT_ms(1000);
    }
}

SUPPRESS_UNUSED
static void test_qre1113(void)
{
    test_setup();
    trace_init();
    qre1113_init();
    struct qre1113_voltages voltages = { 0, 0, 0, 0 };
    while (1) {
        qre1113_get_voltages(&voltages);
        TRACE("Voltages fl %u fr %u bl %u br %u", voltages.front_left, voltages.front_right,
                                                  voltages.back_left, voltages.back_right);
        BUSY_WAIT_ms(1000);
    }
}

SUPPRESS_UNUSED
static void test_line(void)
{
    test_setup();
    trace_init();
    line_init();
    while (1) {
        TRACE("Line %u", line_get());
        BUSY_WAIT_ms(1000);
    }
}

int main()
{
    TEST();
    ASSERT(0);
}
