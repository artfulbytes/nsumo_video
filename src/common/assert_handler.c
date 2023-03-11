#include "common/assert_handler.h"
#include "common/defines.h"
#include "drivers/uart.h"
#include "external/printf/printf.h"
#include <msp430.h>

/* The TI compiler provides intrinsic support for calling a specific opcode, which means
 * you can write __op_code(0x4343) to trigger a software breakpoint (when LAUNCHPAD FET
 * debugger is attached). MSP430-GCC does not have this intrinsic, but 0x4343 corresponds
 * to assembly instruction "CLR.B R3". */
#define BREAKPOINT __asm volatile("CLR.B R3");

// Text + Program counter + Null termination
#define ASSERT_STRING_MAX_SIZE (15u + 6u + 1u)

static void assert_trace(uint16_t program_counter)
{
    // UART Tx
    P1SEL |= BIT1;
    P1SEL2 |= BIT1;
    uart_init_assert();
    char assert_string[ASSERT_STRING_MAX_SIZE];
    snprintf(assert_string, sizeof(assert_string), "ASSERT 0x%x\n", program_counter);
    uart_trace_assert(assert_string);
}

static void assert_blink_led(void)
{
    // Configure TEST LED pin on LAUNCHPAD
    P1SEL &= ~(BIT0);
    P1SEL2 &= ~(BIT0);
    P1DIR |= BIT0;
    P1REN &= ~(BIT0);

    // Configure TEST LED pin on NSUMO
    P2SEL &= ~(BIT6);
    P2SEL2 &= ~(BIT6);
    P2DIR |= BIT6;
    P2REN &= ~(BIT6);

    while (1) {
        // Blink LED on both targets in case the wrong target was flashed
        P1OUT ^= BIT0;
        P2OUT ^= BIT6;
        BUSY_WAIT_ms(250);
    };
}

/* Minimize code dependency in this function to reduce the risk of accidently calling
 * a function with an assert in it, which would cause the assert_handler to be called
 * recursively until stack overflow. */
void assert_handler(uint16_t program_counter)
{
    // TODO: Turn off motors ("safe state")
    BREAKPOINT
    assert_trace(program_counter);
    assert_blink_led();
}
