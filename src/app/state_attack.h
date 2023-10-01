#ifndef STATE_ATTACK_H
#define STATE_ATTACK_H

#include "app/state_common.h"

// Drive towards detected enemy

typedef enum
{
    ATTACK_STATE_FORWARD,
    ATTACK_STATE_LEFT,
    ATTACK_STATE_RIGHT
} attack_state_e;

struct state_attack_data
{
    const struct state_common_data *common;
    attack_state_e state;
};

void state_attack_init(struct state_attack_data *data);
void state_attack_enter(struct state_attack_data *data, state_e from, state_event_e event);

#endif // STATE_ATTACK_H
