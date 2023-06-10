#include "drivers/i2c.h"
#include "drivers/io.h"
#include "common/assert_handler.h"
#include "common/defines.h"
#include <msp430.h>
#include <stdbool.h>

#define DEFAULT_SLAVE_ADDRESS (0x29)
#define RETRY_COUNT (UINT16_MAX)

static inline void i2c_set_tx_byte(uint8_t byte)
{
    UCB0TXBUF = byte;
}

static i2c_result_e i2c_wait_tx_byte(void)
{
    uint16_t retries = RETRY_COUNT;
    while (!(IFG2 & UCB0TXIFG) && retries--) { }
    if (retries == 0) {
        return I2C_RESULT_ERROR_TIMEOUT;
    }
    return (UCB0STAT & UCNACKIFG) ? I2C_RESULT_ERROR_TX : I2C_RESULT_OK;
}

static i2c_result_e i2c_wait_rx_byte(void)
{
    uint16_t retries = RETRY_COUNT;
    while (!(IFG2 & UCB0RXIFG) && retries--) { }
    if (retries == 0) {
        return I2C_RESULT_ERROR_TIMEOUT;
    }
    return (UCB0STAT & UCNACKIFG) ? I2C_RESULT_ERROR_RX : I2C_RESULT_OK;
}

static inline uint8_t i2c_get_rx_byte(void)
{
    return UCB0RXBUF;
}

static inline void i2c_send_start_condition(void)
{
    // Send start condition (and slave addr)
    UCB0CTL1 |= UCTXSTT;
}

static i2c_result_e i2c_wait_start_condition(void)
{
    uint16_t retries = RETRY_COUNT;
    while ((UCB0CTL1 & UCTXSTT) && --retries) { }
    if (retries == 0) {
        return I2C_RESULT_ERROR_TIMEOUT;
    }
    return (UCB0STAT & UCNACKIFG) ? I2C_RESULT_ERROR_START : I2C_RESULT_OK;
}

static i2c_result_e i2c_send_addr(const uint8_t *addr, uint8_t addr_size)
{
    // Configure as sender
    UCB0CTL1 |= UCTR;
    i2c_send_start_condition();

    // Note, when master is TX, we must write to TXBUF before waiting for UCTXSTT
    i2c_set_tx_byte(addr[0]);
    i2c_result_e result = i2c_wait_start_condition();
    if (result) {
        return result;
    }
    result = i2c_wait_tx_byte();
    if (result) {
        return result;
    }

    // Send from most to least significant byte
    for (uint8_t i = 1; i < addr_size; i++) {
        i2c_set_tx_byte(addr[i]);
        result = i2c_wait_tx_byte();
        if (result) {
            return result;
        }
    }

    return result;
}

static i2c_result_e i2c_start_tx_transfer(const uint8_t *addr, uint8_t addr_size)
{
    return i2c_send_addr(addr, addr_size);
}

static i2c_result_e i2c_start_rx_transfer(const uint8_t *addr, uint8_t addr_size)
{
    i2c_result_e result = i2c_send_addr(addr, addr_size);
    if (result) {
        return result;
    }

    // Configure receiver
    UCB0CTL1 &= ~UCTR;
    i2c_send_start_condition();
    result = i2c_wait_start_condition();
    return result;
}

i2c_result_e i2c_stop_transfer(void)
{
    // Send stop condition
    UCB0CTL1 |= UCTXSTP;
    uint16_t retries = RETRY_COUNT;
    while ((UCB0CTL1 & UCTXSTP) && --retries) { }
    if (retries == 0) {
        return I2C_RESULT_ERROR_TIMEOUT;
    }
    return (UCB0STAT & UCNACKIFG) ? I2C_RESULT_ERROR_STOP : I2C_RESULT_OK;
}

i2c_result_e i2c_write(const uint8_t *addr, uint8_t addr_size, const uint8_t *data,
                       uint8_t data_size)
{
    ASSERT(addr);
    ASSERT(addr_size > 0);
    ASSERT(data);
    ASSERT(data_size > 0);

    i2c_result_e result = i2c_start_tx_transfer(addr, addr_size);
    if (result) {
        return result;
    }

    // Send from most to least significant byte
    for (uint16_t i = 0; i < data_size; i++) {
        i2c_set_tx_byte(data[i]);
        result = i2c_wait_tx_byte();
        if (result) {
            return result;
        }
    }

    result = i2c_stop_transfer();
    return result;
}

i2c_result_e i2c_read(const uint8_t *addr, uint8_t addr_size, uint8_t *data, uint8_t data_size)
{
    ASSERT(addr);
    ASSERT(addr_size > 0);
    ASSERT(data);
    ASSERT(data_size > 0);

    i2c_result_e result = i2c_start_rx_transfer(addr, addr_size);
    if (result) {
        return result;
    }

    // Read bytes from most to least significant byte
    for (uint16_t i = data_size - 1; 1 <= i; i--) {
        result = i2c_wait_rx_byte();
        if (result) {
            return result;
        }
        data[i] = i2c_get_rx_byte();
    }

    // Must stop before last byte
    result = i2c_stop_transfer();
    if (result) {
        return result;
    }
    result = i2c_wait_rx_byte();
    if (result) {
        return result;
    }
    data[0] = i2c_get_rx_byte();

    return I2C_RESULT_OK;
}

i2c_result_e i2c_read_addr8_data8(uint8_t addr, uint8_t *data)
{
    return i2c_read(&addr, 1, data, 1);
}

i2c_result_e i2c_read_addr8_data16(uint8_t addr, uint16_t *data)
{
    return i2c_read(&addr, 1, (uint8_t *)data, 2);
}

i2c_result_e i2c_read_addr8_data32(uint8_t addr, uint32_t *data)
{
    return i2c_read(&addr, 1, (uint8_t *)data, 4);
}

i2c_result_e i2c_write_addr8_data8(uint8_t addr, uint8_t data)
{
    return i2c_write(&addr, 1, &data, 1);
}

void i2c_set_slave_address(uint8_t addr)
{
    UCB0I2CSA = addr;
}

static bool initialized = false;
void i2c_init(void)
{
    ASSERT(!initialized);
    static const struct io_config i2c_config = {
        .select = IO_SELECT_ALT3,
        .resistor = IO_RESISTOR_DISABLED,
        .dir = IO_DIR_OUTPUT,
        .out = IO_OUT_LOW,
    };
    struct io_config current_config;
    io_get_current_config(IO_I2C_SCL, &current_config);
    ASSERT(io_config_compare(&i2c_config, &current_config));
    io_get_current_config(IO_I2C_SDA, &current_config);
    ASSERT(io_config_compare(&i2c_config, &current_config));

    // Must set reset while configuring
    UCB0CTL1 |= UCSWRST;
    // Single master, synchronous mode, I2C mode
    UCB0CTL0 = UCMST + UCSYNC + UCMODE_3;
    // SMCLK
    UCB0CTL1 |= UCSSEL_2;
    // SMCLK/160 ~= 100 kHz
    UCB0BR0 = 160;
    UCB0BR1 = 0;
    // Clear reset
    UCB0CTL1 &= ~UCSWRST;
    i2c_set_slave_address(DEFAULT_SLAVE_ADDRESS);

    initialized = true;
}
