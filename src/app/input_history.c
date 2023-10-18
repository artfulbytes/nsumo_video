#include "app/input_history.h"
#include "common/ring_buffer.h"

static bool input_equal(const struct input *a, const struct input *b)
{
    return a->line == b->line && a->enemy.position == b->enemy.position
        && a->enemy.range == b->enemy.range;
}

void input_history_save(struct ring_buffer *history, const struct input *input)
{
    // Skip if no input detected
    if (input->enemy.position == ENEMY_POS_NONE && input->line == LINE_NONE) {
        return;
    }

    // Skip if identical input detected
    if (ring_buffer_count(history)) {
        struct input last_input;
        ring_buffer_peek_head(history, &last_input, 0);
        if (input_equal(input, &last_input)) {
            return;
        }
    }

    ring_buffer_put(history, input);
}

struct enemy input_history_last_directed_enemy(const struct ring_buffer *history)
{
    for (uint8_t offset = 0; offset < ring_buffer_count(history); offset++) {
        struct input input;
        ring_buffer_peek_head(history, &input, offset);
        if (enemy_at_left(&input.enemy) || enemy_at_right(&input.enemy)) {
            return input.enemy;
        }
    }
    const struct enemy enemy_none = { ENEMY_POS_NONE, ENEMY_RANGE_NONE };
    return enemy_none;
}
