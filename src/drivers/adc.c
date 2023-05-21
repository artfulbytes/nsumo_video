#include "drivers/adc.h"
#include "drivers/io.h"
#include "common/defines.h"
#include "common/assert_handler.h"
#include <msp430.h>
#include <stdbool.h>

/* Strategy:
 * Setup ADC to sample a sequence of channels including the channels
 * of interest. Interrupt after the sequence has been sampled and cache
 * the sampled values and start a new round. Let the caller retrieve
 * the latest values from the cache. Use DMA (DTC) and slow clock (ACLK)
 * to reduce CPU involvement. */
static volatile adc_channel_values_t adc_dtc_block;
static volatile adc_channel_values_t adc_dtc_block_cache;
static const io_e *adc_pins;
static uint8_t adc_pin_cnt;
static uint8_t dtc_channel_cnt;

static inline void adc_enable_and_start_conversion(void)
{
    ADC10CTL0 |= ENC + ADC10SC;
}

static bool initialized = false;
void adc_init(void)
{
    ASSERT(!initialized);
    adc_pins = io_adc_pins(&adc_pin_cnt);

    uint8_t adc10ae0 = 0;
    uint8_t last_idx = 0;
    for (uint8_t i = 0; i < adc_pin_cnt; i++) {
        const uint8_t pin_idx = io_to_adc_idx(adc_pins[i]);
        const uint8_t pin_bit = 1 << pin_idx;
        adc10ae0 += pin_bit;
        if (pin_idx > last_idx) {
            last_idx = pin_idx;
        }
    }
    const uint16_t inch = last_idx << 12;

    /* inch: Select channels (last channel when CONSEQ_1)
     * ADC10DIV_7: Clock division (higher means slower)
     * CONSEQ_1: Sequence of channels
     * SHS_0: ADC10SC bit starts conversion
     * ADC10SSEL_1: ACLK as clock source (Slow) */
    ADC10CTL1 = inch + ADC10DIV_7 + CONSEQ_1 + SHS_0 + ADC10SSEL_1;

    /* ADC10ON: Enable
     * SREF_0: Voltage reference (VCC and VSS)
     * ADC10SHT_3: 64 * ADC10CLK sample and hold time (better readings?)
     * MSC: Multiple sample conversion
     * ADC10IE: Enable interrupt */
    ADC10CTL0 = ADC10ON + SREF_0 + ADC10SHT_3 + MSC + ADC10IE;

    // Enable ADC pins
    ADC10AE0 = adc10ae0;

    /* Use data transfer controller (DTC) to transfer data DMA-style from
     * the sampled channels and interrupt afterwards. Note, CONSEQ_1 iterates
     * the channels contiguously from last idx to 0. */
    dtc_channel_cnt = last_idx + 1;
    ADC10DTC0 = ADC10CT;
    ADC10DTC1 = dtc_channel_cnt;
    ADC10SA = (uint16_t)adc_dtc_block;

    adc_enable_and_start_conversion();

    initialized = true;
}

INTERRUPT_FUNCTION(ADC10_VECTOR) isr_adc10(void)
{
    for (uint8_t i = 0; i < dtc_channel_cnt; i++) {
        // DTC writes the channel samples in opposite order
        adc_dtc_block_cache[i] = adc_dtc_block[dtc_channel_cnt - 1 - i];
    }
    adc_enable_and_start_conversion();
}

void adc_get_channel_values(adc_channel_values_t values)
{
    /* For reason unclear to me, it's not enough to disable local ADC interrupt
     * here as then ADC stops working after a while, so as a workaround disable
     * interrupts globally. */
    _disable_interrupts();
    for (uint8_t i = 0; i < adc_pin_cnt; i++) {
        const uint8_t channel_idx = io_to_adc_idx(adc_pins[i]);
        values[channel_idx] = adc_dtc_block_cache[channel_idx];
    }
    _enable_interrupts();
}
