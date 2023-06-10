#ifndef I2C_H
#define I2C_H

#include <stdint.h>

// Polling-based I2C master driver

typedef enum
{
    I2C_RESULT_OK,
    I2C_RESULT_ERROR_START,
    I2C_RESULT_ERROR_TX,
    I2C_RESULT_ERROR_RX,
    I2C_RESULT_ERROR_STOP,
    I2C_RESULT_ERROR_TIMEOUT,
} i2c_result_e;

void i2c_init(void);
void i2c_set_slave_address(uint8_t addr);

// These functions send data in order from most to least significant byte
i2c_result_e i2c_write(const uint8_t *addr, uint8_t addr_size, const uint8_t *data,
                       uint8_t data_size);
i2c_result_e i2c_read(const uint8_t *addr, uint8_t addr_size, uint8_t *data, uint8_t data_size);

// Convenient wrapper functions
i2c_result_e i2c_read_addr8_data8(uint8_t addr, uint8_t *data);
i2c_result_e i2c_read_addr8_data16(uint8_t addr, uint16_t *data);
i2c_result_e i2c_read_addr8_data32(uint8_t addr, uint32_t *data);
i2c_result_e i2c_write_addr8_data8(uint8_t addr, uint8_t data);

#endif // I2C_H
