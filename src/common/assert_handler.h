#ifndef ASSERT_HANDLER_H

// Assert implementation suitable for a microcontroller

#define ASSERT(expression)                                                                         \
    do {                                                                                           \
        if (!(expression)) {                                                                       \
            assert_handler();                                                                      \
        }                                                                                          \
    } while (0)

void assert_handler(void);

#endif // ASSERT_HANDLER_H
