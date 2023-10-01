#include "app/state_attack.h"
#include "app/drive.h"
#include "app/timer.h"
#include "common/assert_handler.h"

#define ATTACK_STATE_TIMEOUT (5000u)

static void state_attack_run(const struct state_attack_data *data)
{
    switch (data->state) {
    case ATTACK_STATE_FORWARD:
        drive_set(DRIVE_DIR_FORWARD, DRIVE_SPEED_FAST);
        break;
    case ATTACK_STATE_LEFT:
        drive_set(DRIVE_DIR_ARCTURN_WIDE_LEFT, DRIVE_SPEED_FAST);
        break;
    case ATTACK_STATE_RIGHT:
        drive_set(DRIVE_DIR_ARCTURN_WIDE_RIGHT, DRIVE_SPEED_FAST);
        break;
    }
    timer_start(data->common->timer, ATTACK_STATE_TIMEOUT);
}

static attack_state_e next_attack_state(const struct enemy *enemy)
{
    if (enemy_at_front(enemy)) {
        return ATTACK_STATE_FORWARD;
    } else if (enemy_at_left(enemy)) {
        return ATTACK_STATE_LEFT;
    } else if (enemy_at_right(enemy)) {
        return ATTACK_STATE_RIGHT;
    } else {
        ASSERT(0);
    }
    return ATTACK_STATE_FORWARD;
}

// No blocking code (e.g. busy wait) allowed in this function
void state_attack_enter(struct state_attack_data *data, state_e from, state_event_e event)
{
    const attack_state_e prev_attack_state = data->state;
    data->state = next_attack_state(&data->common->enemy);

    switch (from) {
    case STATE_SEARCH:
        switch (event) {
        case STATE_EVENT_ENEMY:
            state_attack_run(data);
            break;
        case STATE_EVENT_TIMEOUT:
        case STATE_EVENT_LINE:
        case STATE_EVENT_FINISHED:
        case STATE_EVENT_COMMAND:
        case STATE_EVENT_NONE:
            ASSERT(0);
            break;
        }
        break;
    case STATE_ATTACK:
        switch (event) {
        case STATE_EVENT_ENEMY:
            if (prev_attack_state != data->state) {
                state_attack_run(data);
            }
            break;
        case STATE_EVENT_TIMEOUT:
            // TODO: Consider adding a breakout strategy instead of asserting
            ASSERT(0);
            break;
        case STATE_EVENT_LINE:
        case STATE_EVENT_FINISHED:
        case STATE_EVENT_COMMAND:
        case STATE_EVENT_NONE:
            ASSERT(0);
            break;
        }
        break;
    case STATE_RETREAT:
        // Should always go via search state
        ASSERT(0);
        break;
    case STATE_WAIT:
    case STATE_MANUAL:
        ASSERT(0);
        break;
    }
}

void state_attack_init(struct state_attack_data *data)
{
    data->state = ATTACK_STATE_FORWARD;
}
