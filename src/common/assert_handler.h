#ifndef ASSERT_HANDLER_H

#include <stdint.h>

// Assert implementation suitable for a microcontroller

#if defined(NSUMO) || defined(LAUNCHPAD)
#define ASSERT(expression)                                                                         \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            uint16_t pc;                                                                           \
            asm volatile("mov pc, %0" : "=r"(pc));                                                 \
            assert_handler(pc);                                                                    \
        }                                                                                          \
    } while (0)
#else // Host
#define ASSERT(expression)                                                                         \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            assert_handler(0);                                                                     \
        }                                                                                          \
    } while (0)
#endif

// TODO: Decide what this should do
#define ASSERT_INTERRUPT(expression)                                                               \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            while (1) { }                                                                          \
        }                                                                                          \
    } while (0)

void assert_handler(uint16_t program_counter);

#endif // ASSERT_HANDLER_H
