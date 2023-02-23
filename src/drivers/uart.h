#ifndef UART_H
#define UART_H

void uart_init(void);
// TODO: Replace with printf
void uart_putchar_interrupt(char c);
void uart_print_interrupt(const char *string);

#endif // UART_H
