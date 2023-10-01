#ifndef STATE_RETREAT_H
#define STATE_RETREAT_H

// Drive away from the detected line

#include "app/state_common.h"

typedef enum
{
    RETREAT_STATE_REVERSE,
    RETREAT_STATE_FORWARD,
    RETREAT_STATE_ROTATE_LEFT,
    RETREAT_STATE_ROTATE_RIGHT,
    RETREAT_STATE_ARCTURN_LEFT,
    RETREAT_STATE_ARCTURN_RIGHT,
    RETREAT_STATE_ALIGN_LEFT,
    RETREAT_STATE_ALIGN_RIGHT,
} retreat_state_e;

struct state_retreat_data
{
    const struct state_common_data *common;
    retreat_state_e state;
    int move_idx;
};

void state_retreat_init(struct state_retreat_data *data);
void state_retreat_enter(struct state_retreat_data *data, state_e from, state_event_e event);

#endif // STATE_RETREAT_H
