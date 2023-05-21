#include "drivers/qre1113.h"
#include "drivers/adc.h"
#include "drivers/io.h"
#include "common/assert_handler.h"
#include <stdbool.h>

static bool initialized = false;
void qre1113_init(void)
{
    ASSERT(!initialized);
    adc_init();
    initialized = true;
}

void qre1113_get_voltages(struct qre1113_voltages *voltages)
{
    adc_channel_values_t values;
    adc_get_channel_values(values);
    voltages->front_left = values[io_to_adc_idx(IO_LINE_DETECT_FRONT_LEFT)];
#if defined(NSUMO)
    voltages->front_right = values[io_to_adc_idx(IO_LINE_DETECT_FRONT_RIGHT)];
    voltages->back_left = values[io_to_adc_idx(IO_LINE_DETECT_BACK_LEFT)];
    voltages->back_right = values[io_to_adc_idx(IO_LINE_DETECT_BACK_RIGHT)];
#endif
}
