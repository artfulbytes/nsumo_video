#include "common/ring_buffer.h"
#include "common/assert_handler.h"
#include <string.h>

void ring_buffer_put(struct ring_buffer *rb, const void *data)
{
    // Wrap around (increment tail)
    if (rb->full) {
        ring_buffer_get(rb, NULL);
    }
    memcpy(&rb->buffer[rb->head_idx * rb->elem_size], data, rb->elem_size);

    rb->head_idx++;
    // Avoid expensive modulo operation
    if (rb->head_idx == rb->buffer_size) {
        rb->head_idx = 0;
    }
    if (rb->head_idx == rb->tail_idx) {
        rb->full = true;
    }
}

void ring_buffer_get(struct ring_buffer *rb, void *data)
{
    ASSERT(!ring_buffer_empty(rb));
    if (data) {
        memcpy(data, &rb->buffer[rb->tail_idx * rb->elem_size], rb->elem_size);
    }
    rb->tail_idx++;

    // Avoid expensive modulo operation
    if (rb->tail_idx == rb->buffer_size) {
        rb->tail_idx = 0;
    }
    if (rb->full) {
        rb->full = false;
    }
}

void ring_buffer_peek_tail(const struct ring_buffer *rb, void *data)
{
    ASSERT(!ring_buffer_empty(rb));
    memcpy(data, &rb->buffer[rb->tail_idx * rb->elem_size], rb->elem_size);
}

void ring_buffer_peek_head(const struct ring_buffer *rb, void *data, uint8_t offset)
{
    ASSERT(offset < ring_buffer_count(rb));
    // Note, head_idx is pointing to the next empty slot, so decrement by 1
    int16_t offset_idx = ((int16_t)rb->head_idx - 1) - offset;
    if (offset_idx < 0) {
        offset_idx = rb->buffer_size + offset_idx;
    }
    memcpy(data, &rb->buffer[(uint8_t)offset_idx * rb->elem_size], rb->elem_size);
}

uint8_t ring_buffer_count(const struct ring_buffer *rb)
{
    if (rb->full) {
        return rb->buffer_size;
    } else if (rb->tail_idx <= rb->head_idx) {
        return rb->head_idx - rb->tail_idx;
    } else {
        return (rb->buffer_size - rb->tail_idx) + rb->head_idx + 1;
    }
}

bool ring_buffer_empty(const struct ring_buffer *rb)
{
    return !rb->full && (rb->head_idx == rb->tail_idx);
}

bool ring_buffer_full(const struct ring_buffer *rb)
{
    return rb->full;
}
