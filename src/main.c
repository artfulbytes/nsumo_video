#include "common/assert_handler.h"
#include "common/trace.h"
#include "drivers/mcu_init.h"
#include "drivers/ir_remote.h"
#include "app/drive.h"
#include "app/enemy.h"
#include "app/line.h"
#include "app/state_machine.h"

int main(void)
{
    mcu_init();
    trace_init();
    drive_init();
    enemy_init();
    line_init();
    ir_remote_init();

    state_machine_run();

    ASSERT(0);
    return 0;
}
