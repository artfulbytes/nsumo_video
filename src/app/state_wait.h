#ifndef STATE_WAIT_H
#define STATE_WAIT_H

#include "app/state_common.h"

// Wait for start signal (remote command)

struct state_wait_data
{
    const struct state_common_data *common;
};

void state_wait_enter(struct state_wait_data *data, state_e from, state_event_e event);

#endif // STATE_WAIT_H
