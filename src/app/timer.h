#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

// A poll-based timer implementation

// UINT32_MAX milliseconds ~= 25 days is max timeout
typedef uint32_t timer_t;

void timer_start(timer_t *timer, uint32_t timeout_ms);
bool timer_timeout(const timer_t *timer);
void timer_clear(timer_t *timer);

#endif // TIMER_H
