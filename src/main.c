#include "drivers/uart.h"
#include "drivers/i2c.h"
#include "app/drive.h"
#include "app/enemy.h"

int main(void)
{
    // TODO: There are just for testing the build, should be replaced with real implementation.
    uart_init();
    i2c_init();
    drive_init();
    enemy_init();
    return 0;
}
