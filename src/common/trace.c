#ifndef DISABLE_TRACE
#include "common/trace.h"
#include "common/assert_handler.h"
#include "drivers/uart.h"
#include "external/printf/printf.h"
#include <stdbool.h>

static bool initialized = false;
void trace_init(void)
{
    ASSERT(!initialized);
    uart_init();
    initialized = true;
}

void trace(const char *format, ...)
{
    ASSERT(initialized);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

#endif // DISABLE_TRACE
