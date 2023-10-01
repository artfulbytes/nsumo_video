#ifndef STATE_SEARCH_H
#define STATE_SEARCH_H

#include "app/state_common.h"

// Drive around until enemy is found (or line is detected)

typedef enum
{
    SEARCH_STATE_ROTATE,
    SEARCH_STATE_FORWARD
} search_state_e;

struct state_search_data
{
    const struct state_common_data *common;
    search_state_e state;
};

void state_search_init(struct state_search_data *data);
void state_search_enter(struct state_search_data *data, state_e from, state_event_e event);

#endif // STATE_SEARCH_H
