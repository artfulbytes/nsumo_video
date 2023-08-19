#ifndef ENUM_TO_STRING_H
#define ENUM_TO_STRING_H

#include "drivers/ir_remote.h"
#include "app/line.h"
#include "app/enemy.h"
#include "app/drive.h"

/* Functions to convert enum to readable string. String consume flash space, and
 * keeping these in one place makes it easy to compile them out. */

#ifdef DISABLE_ENUM_STRINGS
#include <stdint.h>
#include "common/defines.h"
static inline const char *empty_func(uint8_t val)
{
    UNUSED(val);
    return "";
}
#define ir_remote_cmd_to_string(cmd) empty_func(cmd)
#define drive_dir_to_string(dir) empty_func(dir)
#define drive_speed_to_string(speed) empty_func(speed)
#define enemy_pos_to_string(pos) empty_func(pos)
#define enemy_range_to_string(range) empty_func(range)
#define line_to_string(line) empty_func(line)
#else
const char *ir_remote_cmd_to_string(ir_cmd_e cmd);
const char *drive_dir_to_string(drive_dir_e dir);
const char *drive_speed_to_string(drive_speed_e speed);
const char *enemy_pos_to_string(enemy_pos_e pos);
const char *enemy_range_to_string(enemy_range_e range);
const char *line_to_string(line_e line);
#endif

#endif // ENUM_TO_STRING_H
