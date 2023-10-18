#include "app/state_machine.h"
#include "app/state_common.h"
#include "app/state_wait.h"
#include "app/state_search.h"
#include "app/state_attack.h"
#include "app/state_retreat.h"
#include "app/state_manual.h"
#include "app/timer.h"
#include "app/input_history.h"
#include "common/trace.h"
#include "common/defines.h"
#include "common/assert_handler.h"
#include "common/enum_to_string.h"
#include "common/ring_buffer.h"
#include "common/sleep.h"

/* A state machine implemented as a set of enums and functions. The states are linked through
 * transitions, which are triggered by events.
 *
 * Flow:
 *    1. Process input
 *        - Check input (e.g. sensors, timer, internal event...)
 *        - Return event
 *    2. Process event
 *        - State/Change state
 *        - Run state function
 *    3. Repeat
 *
 * The flow is continuous (never blocks), which avoids the need for event synchronization
 * mechanisms, since the input can be processed repeatedly at the beginning of each iteration
 * instead. No input is still treated as an event (STATE_EVENT_NONE), but treated as a NOOP
 * when processed. Of course, this means that the code inside the state machine can't block.
 */

struct state_transition
{
    state_e from;
    state_event_e event;
    state_e to;
};

// See docs/state_machine.png (docs/state_machine.uml)
static const struct state_transition state_transitions[] = {
    { STATE_WAIT, STATE_EVENT_LINE, STATE_WAIT },
    { STATE_WAIT, STATE_EVENT_ENEMY, STATE_WAIT },
    { STATE_WAIT, STATE_EVENT_COMMAND, STATE_SEARCH },
    { STATE_SEARCH, STATE_EVENT_NONE, STATE_SEARCH },
    { STATE_SEARCH, STATE_EVENT_TIMEOUT, STATE_SEARCH },
    { STATE_SEARCH, STATE_EVENT_ENEMY, STATE_ATTACK },
    { STATE_SEARCH, STATE_EVENT_LINE, STATE_RETREAT },
    { STATE_SEARCH, STATE_EVENT_COMMAND, STATE_MANUAL },
    { STATE_ATTACK, STATE_EVENT_ENEMY, STATE_ATTACK },
    { STATE_ATTACK, STATE_EVENT_LINE, STATE_RETREAT },
    { STATE_ATTACK, STATE_EVENT_NONE, STATE_SEARCH }, // Enemy lost
    { STATE_ATTACK, STATE_EVENT_COMMAND, STATE_MANUAL },
    { STATE_ATTACK, STATE_EVENT_TIMEOUT, STATE_ATTACK },
    { STATE_RETREAT, STATE_EVENT_LINE, STATE_RETREAT },
    { STATE_RETREAT, STATE_EVENT_FINISHED, STATE_SEARCH },
    { STATE_RETREAT, STATE_EVENT_TIMEOUT, STATE_RETREAT },
    { STATE_RETREAT, STATE_EVENT_ENEMY, STATE_RETREAT },
    { STATE_RETREAT, STATE_EVENT_NONE, STATE_RETREAT },
    { STATE_RETREAT, STATE_EVENT_COMMAND, STATE_MANUAL },
    { STATE_MANUAL, STATE_EVENT_COMMAND, STATE_MANUAL },
    { STATE_MANUAL, STATE_EVENT_NONE, STATE_MANUAL },
    { STATE_MANUAL, STATE_EVENT_LINE, STATE_MANUAL },
    { STATE_MANUAL, STATE_EVENT_ENEMY, STATE_MANUAL },
};

struct state_machine_data
{
    state_e state;
    struct state_common_data common;
    struct state_wait_data wait;
    struct state_search_data search;
    struct state_attack_data attack;
    struct state_retreat_data retreat;
    struct state_manual_data manual;
    state_event_e internal_event;
    timer_t timer;
    struct ring_buffer input_history;
};

static inline bool has_internal_event(const struct state_machine_data *data)
{
    return data->internal_event != STATE_EVENT_NONE;
}

static inline state_event_e take_internal_event(struct state_machine_data *data)
{
    ASSERT(has_internal_event(data));
    const state_event_e event = data->internal_event;
    data->internal_event = STATE_EVENT_NONE;
    return event;
}

void state_machine_post_internal_event(struct state_machine_data *data, state_event_e event)
{
    ASSERT(!has_internal_event(data));
    data->internal_event = event;
}

static void state_enter(struct state_machine_data *data, state_e from, state_event_e event,
                        state_e to)
{
    if (from != to) {
        timer_clear(&data->timer);
        data->state = to;
        TRACE("%s to %s (%s)", state_to_string(from), state_to_string(event),
              state_event_to_string(to));
    }
    switch (to) {
    case STATE_WAIT:
        state_wait_enter(&(data->wait), from, event);
        break;
    case STATE_SEARCH:
        state_search_enter(&(data->search), from, event);
        break;
    case STATE_ATTACK:
        state_attack_enter(&(data->attack), from, event);
        break;
    case STATE_RETREAT:
        state_retreat_enter(&(data->retreat), from, event);
        break;
    case STATE_MANUAL:
        state_manual_enter(&(data->manual), from, event);
        break;
    }
}

static inline void process_event(struct state_machine_data *data, state_event_e next_event)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(state_transitions); i++) {
        if (data->state == state_transitions[i].from && next_event == state_transitions[i].event) {
            state_enter(data, state_transitions[i].from, next_event, state_transitions[i].to);
            return;
        }
    }
    ASSERT(0);
}

static inline state_event_e process_input(struct state_machine_data *data)
{
    data->common.enemy = enemy_get();
    data->common.line = line_get();
    data->common.cmd = ir_remote_get_cmd();
    const struct input input = { .enemy = data->common.enemy, .line = data->common.line };
    input_history_save(&data->input_history, &input);

    if (data->common.cmd != IR_CMD_NONE) {
        return STATE_EVENT_COMMAND;
    } else if (has_internal_event(data)) {
        return take_internal_event(data);
    } else if (timer_timeout(&data->timer)) {
        timer_clear(&data->timer);
        return STATE_EVENT_TIMEOUT;
    } else if (data->common.line != LINE_NONE) {
        return STATE_EVENT_LINE;
    } else if (enemy_detected(&data->common.enemy)) {
        return STATE_EVENT_ENEMY;
    }
    return STATE_EVENT_NONE;
}

static inline void state_machine_init(struct state_machine_data *data)
{
    data->state = STATE_WAIT;
    data->common.state_machine_data = data;
    data->common.enemy.position = ENEMY_POS_NONE;
    data->common.enemy.range = ENEMY_RANGE_NONE;
    data->common.line = LINE_NONE;
    data->common.cmd = IR_CMD_NONE;
    data->common.timer = &data->timer;
    timer_clear(&data->timer);
    data->internal_event = STATE_EVENT_NONE;
    data->wait.common = &data->common;
    data->search.common = &data->common;
    data->attack.common = &data->common;
    data->retreat.common = &data->common;
    data->manual.common = &data->common;
    state_search_init(&data->search);
    state_attack_init(&data->attack);
    state_retreat_init(&data->retreat);
}

#define INPUT_HISTORY_BUFFER_SIZE (6u)
void state_machine_run(void)
{
    struct state_machine_data data;

    // Allocate input history here so the internal buffer remains allocated
    LOCAL_RING_BUFFER(input_history, INPUT_HISTORY_BUFFER_SIZE, struct input);
    data.input_history = input_history;
    data.common.input_history = &data.input_history;

    state_machine_init(&data);

    while (1) {
        const state_event_e next_event = process_input(&data);
        process_event(&data, next_event);
        sleep_ms(1);
    }
}
