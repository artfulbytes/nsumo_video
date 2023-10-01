#include "app/state_manual.h"
#include "app/drive.h"
#include "common/defines.h"

// No blocking code (e.g. busy wait) allowed in this function
void state_manual_enter(struct state_manual_data *data, state_e from, state_event_e event)
{
    UNUSED(from);
    if (event != STATE_EVENT_COMMAND) {
        return;
    }

    switch (data->common->cmd) {
    case IR_CMD_UP:
        drive_set(DRIVE_DIR_FORWARD, DRIVE_SPEED_MAX);
        break;
    case IR_CMD_DOWN:
        drive_set(DRIVE_DIR_REVERSE, DRIVE_SPEED_MAX);
        break;
    case IR_CMD_LEFT:
        drive_set(DRIVE_DIR_ROTATE_LEFT, DRIVE_SPEED_MAX);
        break;
    case IR_CMD_RIGHT:
        drive_set(DRIVE_DIR_ROTATE_RIGHT, DRIVE_SPEED_MAX);
        break;
    case IR_CMD_0:
    case IR_CMD_1:
    case IR_CMD_2:
    case IR_CMD_3:
    case IR_CMD_4:
    case IR_CMD_5:
    case IR_CMD_6:
    case IR_CMD_7:
    case IR_CMD_8:
    case IR_CMD_9:
    case IR_CMD_STAR:
    case IR_CMD_OK:
    case IR_CMD_HASH:
        drive_stop();
        break;
    case IR_CMD_NONE:
        break;
    }
}
