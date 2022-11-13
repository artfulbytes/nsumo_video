# Intro
This documentation describes some of the coding guidelines I follow in this project.
Some guidelines have good reasons while others are just based on my personal preference.
Some are also enforced by the formatter (clang-formatter). The goals with them are to
reduce common errors, get a more consistent code-base, and to avoid discussions about
petty details.

# Naming
* snake_case for everything including filenames except defines
    - E.g. i2c.c, uart.c
``` C
void this_is_a_function(void)
{
    uint8_t this_is_a_variable;
}
```
* UPPER_CASE for defines
``` C
#define THIS_IS_A_CONSTANT (0)
```
* One code module per header and implementation file
    - A module is something that makes sense as a single idea
    - e.g. uart.c/uart.h, i2c.c/i2c.h, tb6612fng.c/tb6612fng.h,line_sensor.c/line_sensor.h
* Prefix function names with module name
``` C
void uart_init(void);
void uart_write(void);
void i2c_init(void);
uint8_t i2c_read(void);
void led_set(led_e led, led_state state);
```

# Indentation
* Four spaces
* No tabs

# Comments
* Comment what's not obvious from just reading the code
* Focus on why over how and what
* Don't overdo it
* Prefer good variable and function names over comments
* Use // for single line comments and /* */ for multi-line comments.

# Asserts
Asserts are mainly for catching obvious mistakes during development. In some sense, they also
serve as documentation.
* Use asserts for detecting invalid function arguments, return values, and invalid variable content.
* Don't use asserts to check values received from sensors and other devices

# Defines
* Define all constant numbers and comment if it's unclear where they come from
* Always use parenthesis (even for single numbers) to avoid unexpected macro expansion
``` C
// This number ...
#define CONSTANT_NUMBER (100)
```
* Use do { } while(0) for multi-statement macros to have them act more like functions, i.e. can
  be followed by a semicolon ; without confusion.
``` C
#define ASSERT(expression)                              \
    do {                                                \
        if (!(expression)) {                            \
            assert_handler();                           \
        }                                               \
    } while (0)
```
* Avoid define macros when possible since they are error-prone and less readable

# Header files
* Use #include guards in every header file to avoid duplicated and recursive inclusions
* A brief comment describing the module at top of every header file
``` C
#ifndef UART_H
// A UART driver for setting up and operating the UART peripheral. DMA...
#endif // UART_H
```
* Include header files in order from local to global
``` C
#include "drivers/uart.h"
#include "common/defines.h"
#include "msp430.h"
```
* Limit includes in headers

# Switch statements
* If switching on an enum value, handle all cases and skip the default case
* Without the default case, the compiler warning will force you to update the switch statements
  when a new enum value is added
``` C
switch (io)
{
case IO_TEST_LED:
    // ...
    break;
case IO_UART_RX:
    // ...
    break;
case IO_UART_TX:
    // ...
    break;
}
```

# if/else statements
* Always use brackets, even for single statement

# Functions
* Functions that are only used internally inside a single translation unit (.c file) should be static
* Use void as parameter in function declarations without parameters, because in C, functions with empty
  paranthesis can be called with any number of parameters.
``` C
// io.h
void io_init(void);
```

``` C
// io.c
static void io_helper_function(void)
{
    // ...
}

void io_init(void)
{
    io_helper_function();
    // ...
}
```
* Pass structs by pointer where applicable to reduce stack usage
``` C
struct
{
    io_dir_e dir;
    io_out_e out;
} io_config;

void io_configure(io_e io, struct io_config *config)
{
    // ...
}
```

# Data types
## Typedef
* Typedef function pointers to give them simpler names where appropriate
* Always typedef enums to avoid using the keyword enum before every enum value
* Always suffix typedefed enums with _e
* Enum values should be UPPER_CASE and prefixed with enum name
``` C
typedef enum
{
    IO_TEST_LED,
    IO_UART_RX,
    IO_UART_TX,
} io_e;
```
* Don't typedef structs
* Don't suffix with _t (it's reserved by POSIX)

## Fixed-width integers (stdint.h)
* Use fixed-width integers (uint8_t, uint16_t, int16_t, int32_t, etc.)
* It makes memory usage obvious and porting easier.

## Const correctness
* All variables that shouldn't change should be const
* It protects against accidentally modifying a variable that shouldn't be modified.
* It makes the code more explicit and readable
``` C
void io_configure(io_e io, const struct io_config *config)
{
    const struct io_config current_config = get_config(io);
    // ...
}
```
