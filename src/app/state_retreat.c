#include "app/state_retreat.h"
#include "app/drive.h"
#include "app/timer.h"
#include "common/assert_handler.h"
#include "common/enum_to_string.h"
#include <stdbool.h>

#define MOVE_MAX_CNT (3u)

struct move
{
    drive_dir_e dir;
    drive_speed_e speed;
    uint16_t duration;
};

struct retreat_state
{
    struct move moves[MOVE_MAX_CNT];
    uint8_t move_cnt;
};

static const struct retreat_state retreat_states[] =
{
    [RETREAT_STATE_REVERSE] =
    {
        .move_cnt = 1,
        .moves = { { DRIVE_DIR_REVERSE, DRIVE_SPEED_MAX, 300 } },
    },
    // TODO: Add the rest
};

static const struct move *current_move(const struct state_retreat_data *data)
{
    return &retreat_states[data->state].moves[data->move_idx];
}

static retreat_state_e next_retreat_state(const struct state_retreat_data *data)
{
    switch (data->common->line) {
    case LINE_FRONT_LEFT:
        // TODO: Set retreat state based on input
        break;
    case LINE_FRONT_RIGHT:
        // TODO: Set retreat state based on input
        break;
    case LINE_BACK_LEFT:
        current_move(data); // TODO: Use this properly
        // TODO: Set retreat state based on input
        break;
    case LINE_BACK_RIGHT:
        // TODO: Set retreat state based on input
        break;
    case LINE_FRONT:
        // TODO: Set retreat state based on input
        break;
    case LINE_BACK:
        return RETREAT_STATE_FORWARD;
    case LINE_LEFT:
        return RETREAT_STATE_ARCTURN_RIGHT;
    case LINE_RIGHT:
        return RETREAT_STATE_ARCTURN_LEFT;
    case LINE_DIAGONAL_LEFT:
    case LINE_DIAGONAL_RIGHT:
        /* It's ambigiuous if the dohyo is in front or back. Could perhaps use detect history
         * for best guess. */
        ASSERT(0);
        break;
    case LINE_NONE:
        ASSERT(0);
        break;
    }
    return RETREAT_STATE_REVERSE;
}

static void start_retreat_move(const struct state_retreat_data *data)
{
    ASSERT(data->move_idx < retreat_states[data->state].move_cnt);
    const struct move move = retreat_states[data->state].moves[data->move_idx];
    timer_start(data->common->timer, move.duration);
    drive_set(move.dir, move.speed);
}

static bool retreat_state_done(const struct state_retreat_data *data)
{
    return data->move_idx == retreat_states[data->state].move_cnt;
}

static void state_retreat_run(struct state_retreat_data *data)
{
    data->move_idx = 0;
    data->state = next_retreat_state(data);
    start_retreat_move(data);
}

// No blocking code (e.g. busy wait) allowed in this function
void state_retreat_enter(struct state_retreat_data *data, state_e from, state_event_e event)
{
    switch (from) {
    case STATE_SEARCH:
        // fall-through
    case STATE_ATTACK:
        switch (event) {
        case STATE_EVENT_LINE:
            state_retreat_run(data);
            break;
        case STATE_EVENT_FINISHED:
        case STATE_EVENT_TIMEOUT:
        case STATE_EVENT_ENEMY:
        case STATE_EVENT_COMMAND:
        case STATE_EVENT_NONE:
            ASSERT(0);
            break;
        }
        break;
    case STATE_RETREAT:
        switch (event) {
        case STATE_EVENT_LINE:
            state_retreat_run(data);
            break;
        case STATE_EVENT_TIMEOUT:
            data->move_idx++;
            if (retreat_state_done(data)) {
                state_machine_post_internal_event(data->common->state_machine_data,
                                                  STATE_EVENT_FINISHED);
            } else {
                start_retreat_move(data);
            }
        case STATE_EVENT_ENEMY:
            // Ignore enemy when retreating
        case STATE_EVENT_NONE:
            break;
        case STATE_EVENT_FINISHED:
        case STATE_EVENT_COMMAND:
            ASSERT(0);
            break;
        }
        break;
    case STATE_WAIT:
    case STATE_MANUAL:
        ASSERT(0);
        break;
    }
}

void state_retreat_init(struct state_retreat_data *data)
{
    data->state = RETREAT_STATE_REVERSE;
    data->move_idx = 0;
}
