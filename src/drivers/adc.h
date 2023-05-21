#ifndef ADC_H
#define ADC_H

// ADC driver sampling the values of the ADC assigned IO pins (see io.c)

#include <stdint.h>

#define ADC_CHANNEL_COUNT (8u)
typedef uint16_t adc_channel_values_t[ADC_CHANNEL_COUNT];

void adc_init(void);
void adc_get_channel_values(adc_channel_values_t values);

#endif // ADC_H
