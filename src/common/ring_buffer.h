#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

// Circular buffer (FIFO) that can store elements of any size.

struct ring_buffer
{
    uint8_t *buffer;
    uint8_t buffer_size;
    uint8_t elem_size;
    uint8_t head_idx; // index of the next empty slot
    uint8_t tail_idx; // index of the oldest element
    bool full; // head == tail when full and empty, use this to distinguish
};

#define RING_BUFFER(name, size, type, storage)                                                     \
    static_assert(size < UINT8_MAX);                                                               \
    storage uint8_t name##_buffer[size * sizeof(type)] = { 0 };                                    \
    storage struct ring_buffer name = { .buffer = name##_buffer,                                   \
                                        .buffer_size = size,                                       \
                                        .elem_size = sizeof(type) }

#define LOCAL_RING_BUFFER(name, size, type) RING_BUFFER(name, size, type, )
#define STATIC_RING_BUFFER(name, size, type) RING_BUFFER(name, size, type, static)

/* These functions include out-of-bounds checks, so it's not efficient
 * to call these many times consecutively */

void ring_buffer_put(struct ring_buffer *rb, const void *data);
void ring_buffer_get(struct ring_buffer *rb, void *data);
// Look at the oldest element
void ring_buffer_peek_tail(const struct ring_buffer *rb, void *data);
// Look at the newest element (-offset)
void ring_buffer_peek_head(const struct ring_buffer *rb, void *data, uint8_t offset);

uint8_t ring_buffer_count(const struct ring_buffer *rb);
bool ring_buffer_empty(const struct ring_buffer *rb);
bool ring_buffer_full(const struct ring_buffer *rb);

#endif // RING_BUFFER_H
