#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

struct ring_buffer
{
    uint8_t *buffer;
    uint8_t size;
    uint8_t head;
    uint8_t tail;
};

// Caller must check if full
void ring_buffer_put(struct ring_buffer *rb, uint8_t data);
// Caller must check if empty
uint8_t ring_buffer_get(struct ring_buffer *rb);
// Caller must check if empty
uint8_t ring_buffer_peek(const struct ring_buffer *rb);
bool ring_buffer_empty(const struct ring_buffer *rb);
bool ring_buffer_full(const struct ring_buffer *rb);

#endif // RING_BUFFER_H
