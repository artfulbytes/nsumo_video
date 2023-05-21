#include "app/line.h"
#include "drivers/qre1113.h"
#include "common/assert_handler.h"
#include <stdbool.h>

// Based on readings from the sensors when they are above the white line
#define LINE_DETECTED_VOLTAGE_THRESHOLD (700u)

static bool initialized = false;
void line_init(void)
{
    ASSERT(!initialized);
    qre1113_init();
    initialized = true;
}

line_e line_get(void)
{
    struct qre1113_voltages voltages;
    qre1113_get_voltages(&voltages);
    const bool front_left = voltages.front_left < LINE_DETECTED_VOLTAGE_THRESHOLD;
    const bool front_right = voltages.front_right < LINE_DETECTED_VOLTAGE_THRESHOLD;
    const bool back_left = voltages.back_left < LINE_DETECTED_VOLTAGE_THRESHOLD;
    const bool back_right = voltages.back_right < LINE_DETECTED_VOLTAGE_THRESHOLD;

    if (front_left) {
        if (front_right) {
            return LINE_FRONT;
        } else if (back_left) {
            return LINE_LEFT;
        } else if (back_right) {
            return LINE_DIAGONAL_LEFT;
        } else {
            return LINE_FRONT_LEFT;
        }
    } else if (front_right) {
        if (back_right) {
            return LINE_RIGHT;
        } else if (back_left) {
            return LINE_DIAGONAL_RIGHT;
        } else {
            return LINE_FRONT_RIGHT;
        }
    } else if (back_left) {
        if (back_right) {
            return LINE_BACK;
        } else {
            return LINE_BACK_LEFT;
        }
    } else if (back_right) {
        return LINE_BACK_RIGHT;
    }
    return LINE_NONE;
}
