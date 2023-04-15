#ifndef DRIVE_H
#define DRIVE_H

// A coarser drive interface for controlling the motors from the application code

typedef enum
{
    DRIVE_DIR_FORWARD,
    DRIVE_DIR_REVERSE,
    DRIVE_DIR_ROTATE_LEFT,
    DRIVE_DIR_ROTATE_RIGHT,
    DRIVE_DIR_ARCTURN_SHARP_LEFT,
    DRIVE_DIR_ARCTURN_SHARP_RIGHT,
    DRIVE_DIR_ARCTURN_MID_LEFT,
    DRIVE_DIR_ARCTURN_MID_RIGHT,
    DRIVE_DIR_ARCTURN_WIDE_LEFT,
    DRIVE_DIR_ARCTURN_WIDE_RIGHT,
} drive_dir_e;

typedef enum
{
    DRIVE_SPEED_SLOW,
    DRIVE_SPEED_MEDIUM,
    DRIVE_SPEED_FAST,
    DRIVE_SPEED_MAX
} drive_speed_e;

void drive_init(void);
void drive_stop(void);
void drive_set(drive_dir_e direction, drive_speed_e speed);

#endif
