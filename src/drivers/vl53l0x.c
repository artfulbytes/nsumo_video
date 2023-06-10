#include "drivers/vl53l0x.h"
#include "drivers/i2c.h"
#include "drivers/io.h"
#include "common/defines.h"
#include "common/assert_handler.h"

#define REG_IDENTIFICATION_MODEL_ID (0xC0)
#define REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV (0x89)
#define REG_MSRC_CONFIG_CONTROL (0x60)
#define REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT (0x44)
#define REG_SYSTEM_SEQUENCE_CONFIG (0x01)
#define REG_DYNAMIC_SPAD_REF_EN_START_OFFSET (0x4F)
#define REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD (0x4E)
#define REG_GLOBAL_CONFIG_REF_EN_START_SELECT (0xB6)
#define REG_SYSTEM_INTERRUPT_CONFIG_GPIO (0x0A)
#define REG_GPIO_HV_MUX_ACTIVE_HIGH (0x84)
#define REG_SYSTEM_INTERRUPT_CLEAR (0x0B)
#define REG_RESULT_INTERRUPT_STATUS (0x13)
#define REG_SYSRANGE_START (0x00)
#define REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 (0xB0)
#define REG_RESULT_RANGE_STATUS (0x14)
#define REG_SLAVE_DEVICE_ADDRESS (0x8A)

#define RANGE_SEQUENCE_STEP_TCC (0x10) // Target CentreCheck
#define RANGE_SEQUENCE_STEP_MSRC (0x04) // Minimum Signal Rate Check
#define RANGE_SEQUENCE_STEP_DSS (0x28) // Dynamic SPAD selection
#define RANGE_SEQUENCE_STEP_PRE_RANGE (0x40)
#define RANGE_SEQUENCE_STEP_FINAL_RANGE (0x80)

#define VL53L0X_EXPECTED_DEVICE_ID (0xEE)
#define VL53L0X_DEFAULT_ADDRESS (0x29)

/* There are two types of SPAD: aperture and non-aperture. My understanding
 * is that aperture ones let it less light (they have a smaller opening), similar
 * to how you can change the aperture on a digital camera. Only 1/4 th of the
 * SPADs are of type non-aperture. */
#define SPAD_TYPE_APERTURE (0x01)
/* The total SPAD array is 16x16, but we can only activate a quadrant spanning 44 SPADs at
 * a time. In the ST api code they have (for some reason) selected 0xB4 (180) as a starting
 * point (lies in the middle and spans non-aperture (3rd) quadrant and aperture (4th) quadrant). */
#define SPAD_START_SELECT (0xB4)
/* The total SPAD map is 16x16, but we should only activate an area of 44 SPADs at a time. */
#define SPAD_MAX_COUNT (44)
/* The 44 SPADs are represented as 6 bytes where each bit represents a single SPAD.
 * 6x8 = 48, so the last four bits are unused. */
#define SPAD_MAP_ROW_COUNT (6)
#define SPAD_ROW_SIZE (8)
/* Since we start at 0xB4 (180), there are four quadrants (three aperture, one aperture),
 * and each quadrant contains 256 / 4 = 64 SPADs, and the third quadrant is non-aperture, the
 * offset to the aperture quadrant is (256 - 64 - 180) = 12 */
#define SPAD_APERTURE_START_INDEX (12)

typedef enum
{
    VL53L0X_CALIBRATION_TYPE_VHV,
    VL53L0X_CALIBRATION_TYPE_PHASE
} vl53l0x_calibration_type_e;

typedef enum
{
    STATUS_MULTIPLE_NOT_STARTED,
    STATUS_MULTIPLE_MEASURING,
    STATUS_MULTIPLE_DONE
} status_multiple_e;

struct vl53l0x_cfg
{
    uint8_t addr;
    io_e xshut_io;
};

struct i2c_8reg
{
    uint8_t addr;
    uint8_t data;
};

static const struct vl53l0x_cfg vl53l0x_cfgs[] = {
    [VL53L0X_IDX_FRONT] = { .addr = 0x30, .xshut_io = IO_XSHUT_FRONT },
#if defined(NSUMO)
    [VL53L0X_IDX_LEFT] = { .addr = 0x31, .xshut_io = IO_XSHUT_LEFT },
    [VL53L0X_IDX_RIGHT] = { .addr = 0x32, .xshut_io = IO_XSHUT_RIGHT },
    [VL53L0X_IDX_FRONT_RIGHT] = { .addr = 0x33, .xshut_io = IO_XSHUT_FRONT_RIGHT },
    [VL53L0X_IDX_FRONT_LEFT] = { .addr = 0x34, .xshut_io = IO_XSHUT_FRONT_LEFT },
#endif
};

static uint8_t stop_variable = 0;
// Reads/Writes to this can be considered atomic on MSP430
static volatile status_multiple_e status_multiple = STATUS_MULTIPLE_NOT_STARTED;
static bool initialized = false;

/* We can read the model id to confirm that the device is booted.
 * (There is no fresh_out_of_reset as on the vl6180x) */
static vl53l0x_result_e device_is_booted(void)
{
    uint8_t device_id = 0;
    if (i2c_read_addr8_data8(REG_IDENTIFICATION_MODEL_ID, &device_id)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return (device_id == VL53L0X_EXPECTED_DEVICE_ID) ? VL53L0X_RESULT_OK
                                                     : VL53L0X_RESULT_ERROR_BOOT;
}

static vl53l0x_result_e vl53l0x_write_8regs(const struct i2c_8reg *regs, uint8_t cnt)
{
    for (uint8_t i = 0; i < cnt; i++) {
        if (i2c_write_addr8_data8(regs[i].addr, regs[i].data)) {
            return VL53L0X_RESULT_ERROR_I2C;
        }
    }
    return VL53L0X_RESULT_OK;
}

// One time device initialization
static vl53l0x_result_e vl53l0x_data_init(void)
{
    // Set 2v8 mode
    uint8_t vhv_config_scl_sda = 0;
    if (i2c_read_addr8_data8(REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, &vhv_config_scl_sda)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    vhv_config_scl_sda |= 0x01;
    if (i2c_write_addr8_data8(REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV, vhv_config_scl_sda)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    // Set I2C standard mode
    if (i2c_write_addr8_data8(0x88, 0x00)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    // Set various registers (same as ST reference code)
    const struct i2c_8reg data_regs1[] = { { 0x80, 0x01 }, { 0xFF, 0x01 }, { 0x00, 0x00 } };
    vl53l0x_result_e result = vl53l0x_write_8regs(data_regs1, ARRAY_SIZE(data_regs1));
    if (result) {
        return result;
    }

    // TODO: It may be unnecessary to retrieve the stop variable for each sensor
    if (i2c_read_addr8_data8(0x91, &stop_variable)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    // Set various registers (same as ST reference code)
    const struct i2c_8reg data_regs2[] = { { 0x00, 0x01 }, { 0xFF, 0x00 }, { 0x80, 0x00 } };
    result = vl53l0x_write_8regs(data_regs2, ARRAY_SIZE(data_regs2));

    return result;
}

/* Wait for strobe value to be set. This is used when we read values
 * from NVM (non volatile memory). */
static vl53l0x_result_e vl53l0x_read_strobe(void)
{
    uint8_t strobe = 0;
    if (i2c_write_addr8_data8(0x83, 0x00)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    i2c_result_e result = I2C_RESULT_OK;
    do {
        result = i2c_read_addr8_data8(0x83, &strobe);
    } while (result == I2C_RESULT_OK && (strobe == 0));
    if (result) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    if (i2c_write_addr8_data8(0x83, 0x01)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return VL53L0X_RESULT_OK;
}

/**
 * Gets the spad count, spad type och "good" spad map stored by ST in NVM at
 * their production line.
 * .
 * According to the datasheet, ST runs a calibration (without cover glass) and
 * saves a "good" SPAD map to NVM (non volatile memory). The SPAD array has two
 * types of SPADs: aperture and non-aperture. By default, all of the
 * good SPADs are enabled, but we should only enable a subset of them to get
 * an optimized signal rate. We should also only enable either only aperture
 * or only non-aperture SPADs. The number of SPADs to enable and which type
 * are also saved during the calibration step at ST factory and can be retrieved
 * from NVM.
 */
static vl53l0x_result_e vl53l0x_get_spad_info_from_nvm(uint8_t *spad_count, uint8_t *spad_type,
                                                       uint8_t good_spad_map[6])
{
    uint8_t tmp_data8 = 0;
    uint32_t tmp_data32 = 0;

    const struct i2c_8reg nvm_setup_regs[] = { { 0x80, 0x01 }, { 0xFF, 0x01 }, { 0x00, 0x00 },
                                               { 0xFF, 0x06 }, { 0xFF, 0x07 }, { 0x81, 0x01 },
                                               { 0x80, 0x01 } };

    // Setup to read from NVM
    vl53l0x_result_e result = vl53l0x_write_8regs(nvm_setup_regs, 4);
    if (result) {
        return result;
    }
    if (i2c_read_addr8_data8(0x83, &tmp_data8)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    if (i2c_write_addr8_data8(0x83, tmp_data8 | 0x04)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    result = vl53l0x_write_8regs(nvm_setup_regs + 4, ARRAY_SIZE(nvm_setup_regs) - 4);
    if (result) {
        return result;
    }

    // Get the SPAD count and type
    if (i2c_write_addr8_data8(0x94, 0x6b)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    result = vl53l0x_read_strobe();
    if (result) {
        return result;
    }
    if (i2c_read_addr8_data32(0x90, &tmp_data32)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    *spad_count = (tmp_data32 >> 8) & 0x7f;
    *spad_type = (tmp_data32 >> 15) & 0x01;

    /* Since the good SPAD map is already stored in REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0
     * we can simply read that register instead of doing the below */
#if 0
    /* Get the first part of the SPAD map */
    if (!i2c_write_addr8_data8(0x94, 0x24)) {
        return false;
    }
    if (!read_strobe()) {
        return false;
    }
    if (!i2c_read_addr8_data32(0x90, &tmp_data32)) {
      return false;
    }
    good_spad_map[0] = (uint8_t)((tmp_data32 >> 24) & 0xFF);
    good_spad_map[1] = (uint8_t)((tmp_data32 >> 16) & 0xFF);
    good_spad_map[2] = (uint8_t)((tmp_data32 >> 8) & 0xFF);
    good_spad_map[3] = (uint8_t)(tmp_data32 & 0xFF);

    /* Get the second part of the SPAD map */
    if (!i2c_write_addr8_data8(0x94, 0x25)) {
        return false;
    }
    if (!read_strobe()) {
        return false;
    }
    if (!i2c_read_addr8_data32(0x90, &tmp_data32)) {
        return false;
    }
    good_spad_map[4] = (uint8_t)((tmp_data32 >> 24) & 0xFF);
    good_spad_map[5] = (uint8_t)((tmp_data32 >> 16) & 0xFF);

#endif
    const struct i2c_8reg nvm_restore_regs[] = { { 0x81, 0x00 }, { 0xFF, 0x06 }, { 0xFF, 0x01 },
                                                 { 0x00, 0x01 }, { 0xFF, 0x00 }, { 0x80, 0x00 } };

    // Restore after reading from NVM
    result = vl53l0x_write_8regs(nvm_restore_regs, 2);
    if (result) {
        return result;
    }
    if (i2c_read_addr8_data8(0x83, &tmp_data8)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    if (i2c_write_addr8_data8(0x83, tmp_data8 & 0xfb)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    result = vl53l0x_write_8regs(nvm_restore_regs + 2, ARRAY_SIZE(nvm_restore_regs) - 2);
    if (result) {
        return result;
    }

    /* When we haven't configured the SPAD map yet, the SPAD map register actually
     * contains the good SPAD map, so we can retrieve it straight from this register
     * instead of reading it from the NVM. */
    const uint8_t reg_global_cfg_addr = REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0;
    result = i2c_read(&reg_global_cfg_addr, 1, good_spad_map, 6);
    return result;
}

/**
 * Sets the SPADs according to the value saved to NVM by ST during production. Assuming
 * similar conditions (e.g. no cover glass), this should give reasonable readings and we
 * can avoid running ref spad management (tedious code).
 */
static vl53l0x_result_e vl53l0x_set_spads_from_nvm(void)
{
    uint8_t spad_map[SPAD_MAP_ROW_COUNT] = { 0 };
    uint8_t good_spad_map[SPAD_MAP_ROW_COUNT] = { 0 };
    uint8_t spads_enabled_count = 0;
    uint8_t spads_to_enable_count = 0;
    uint8_t spad_type = 0;

    vl53l0x_result_e result =
        vl53l0x_get_spad_info_from_nvm(&spads_to_enable_count, &spad_type, good_spad_map);
    if (result) {
        return result;
    }

    const struct i2c_8reg spad_regs[] = { { 0xFF, 0x01 },
                                          { REG_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00 },
                                          { REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C },
                                          { 0xFF, 0x00 },
                                          { REG_GLOBAL_CONFIG_REF_EN_START_SELECT,
                                            SPAD_START_SELECT } };
    result = vl53l0x_write_8regs(spad_regs, ARRAY_SIZE(spad_regs));
    if (result) {
        return result;
    }

    uint8_t offset = (spad_type == SPAD_TYPE_APERTURE) ? SPAD_APERTURE_START_INDEX : 0;

    /* Create a new SPAD array by selecting a subset of the SPADs suggested by the good SPAD map.
     * The subset should only have the number of type enabled as suggested by the reading from
     * the NVM (spads_to_enable_count and spad_type). */
    for (int row = 0; row < SPAD_MAP_ROW_COUNT; row++) {
        for (int column = 0; column < SPAD_ROW_SIZE; column++) {
            int index = (row * SPAD_ROW_SIZE) + column;
            if (index >= SPAD_MAX_COUNT) {
                return VL53L0X_RESULT_ERROR_SPAD;
            }
            if (spads_enabled_count == spads_to_enable_count) {
                // We are done
                break;
            }
            if (index < offset) {
                continue;
            }
            if ((good_spad_map[row] >> column) & 0x1) {
                spad_map[row] |= (1 << column);
                spads_enabled_count++;
            }
        }
        if (spads_enabled_count == spads_to_enable_count) {
            // To avoid looping unnecessarily when we are already done.
            break;
        }
    }

    if (spads_enabled_count != spads_to_enable_count) {
        return VL53L0X_RESULT_ERROR_SPAD;
    }

    // Write the new SPAD configuration
    const uint8_t reg_global_cfg_addr = REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0;
    if (i2c_write(&reg_global_cfg_addr, 1, spad_map, SPAD_MAP_ROW_COUNT)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    return VL53L0X_RESULT_OK;
}

// Load tuning settings (same as default tuning settings provided by ST api code)
static vl53l0x_result_e vl53l0x_load_default_tuning_settings(void)
{
    const struct i2c_8reg default_tuning_regs[] = {
        { 0xFF, 0x01 }, { 0x00, 0x00 }, { 0xFF, 0x00 }, { 0x09, 0x00 }, { 0x10, 0x00 },
        { 0x11, 0x00 }, { 0x24, 0x01 }, { 0x25, 0xFF }, { 0x75, 0x00 }, { 0xFF, 0x01 },
        { 0x4E, 0x2C }, { 0x48, 0x00 }, { 0x30, 0x20 }, { 0xFF, 0x00 }, { 0x30, 0x09 },
        { 0x54, 0x00 }, { 0x31, 0x04 }, { 0x32, 0x03 }, { 0x40, 0x83 }, { 0x46, 0x25 },
        { 0x60, 0x00 }, { 0x27, 0x00 }, { 0x50, 0x06 }, { 0x51, 0x00 }, { 0x52, 0x96 },
        { 0x56, 0x08 }, { 0x57, 0x30 }, { 0x61, 0x00 }, { 0x62, 0x00 }, { 0x64, 0x00 },
        { 0x65, 0x00 }, { 0x66, 0xA0 }, { 0xFF, 0x01 }, { 0x22, 0x32 }, { 0x47, 0x14 },
        { 0x49, 0xFF }, { 0x4A, 0x00 }, { 0xFF, 0x00 }, { 0x7A, 0x0A }, { 0x7B, 0x00 },
        { 0x78, 0x21 }, { 0xFF, 0x01 }, { 0x23, 0x34 }, { 0x42, 0x00 }, { 0x44, 0xFF },
        { 0x45, 0x26 }, { 0x46, 0x05 }, { 0x40, 0x40 }, { 0x0E, 0x06 }, { 0x20, 0x1A },
        { 0x43, 0x40 }, { 0xFF, 0x00 }, { 0x34, 0x03 }, { 0x35, 0x44 }, { 0xFF, 0x01 },
        { 0x31, 0x04 }, { 0x4B, 0x09 }, { 0x4C, 0x05 }, { 0x4D, 0x04 }, { 0xFF, 0x00 },
        { 0x44, 0x00 }, { 0x45, 0x20 }, { 0x47, 0x08 }, { 0x48, 0x28 }, { 0x67, 0x00 },
        { 0x70, 0x04 }, { 0x71, 0x01 }, { 0x72, 0xFE }, { 0x76, 0x00 }, { 0x77, 0x00 },
        { 0xFF, 0x01 }, { 0x0D, 0x01 }, { 0xFF, 0x00 }, { 0x80, 0x01 }, { 0x01, 0xF8 },
        { 0xFF, 0x01 }, { 0x8E, 0x01 }, { 0x00, 0x01 }, { 0xFF, 0x00 }, { 0x80, 0x00 }
    };
    return vl53l0x_write_8regs(default_tuning_regs, ARRAY_SIZE(default_tuning_regs));
}

static vl53l0x_result_e vl53l0x_configure_interrupt(void)
{
    // Interrupt on new sample ready
    if (i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    /* Configure active low since the pin is pulled-up on most breakout boards */
    uint8_t gpio_hv_mux_active_high = 0;
    if (i2c_read_addr8_data8(REG_GPIO_HV_MUX_ACTIVE_HIGH, &gpio_hv_mux_active_high)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    gpio_hv_mux_active_high &= ~0x10;
    if (i2c_write_addr8_data8(REG_GPIO_HV_MUX_ACTIVE_HIGH, gpio_hv_mux_active_high)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    if (i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return VL53L0X_RESULT_OK;
}

static void front_measurement_done_isr()
{
    status_multiple = STATUS_MULTIPLE_DONE;
}

static void vl53l0x_configure_front_sensor_interrupt(void)
{
    static const struct io_config front_interrupt_config = {
        .select = IO_SELECT_GPIO,
        .resistor = IO_RESISTOR_DISABLED,
        .dir = IO_DIR_INPUT,
        .out = IO_OUT_LOW,
    };
    struct io_config current_config;
    io_get_current_config(IO_RANGE_SENSOR_FRONT_INT, &current_config);
    ASSERT(io_config_compare(&front_interrupt_config, &current_config));

    io_configure_interrupt(IO_RANGE_SENSOR_FRONT_INT, IO_TRIGGER_FALLING,
                           front_measurement_done_isr);
    io_enable_interrupt(IO_RANGE_SENSOR_FRONT_INT);
}

// Enable (or disable) specific steps in the sequence
static vl53l0x_result_e vl53l0x_set_sequence_steps_enabled(uint8_t sequence_step)
{
    if (i2c_write_addr8_data8(REG_SYSTEM_SEQUENCE_CONFIG, sequence_step)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return VL53L0X_RESULT_OK;
}

// Basic device initialization
static vl53l0x_result_e vl53l0x_static_init(void)
{
    vl53l0x_result_e result = vl53l0x_set_spads_from_nvm();
    if (result) {
        return result;
    }

    result = vl53l0x_load_default_tuning_settings();
    if (result) {
        return result;
    }

    result = vl53l0x_configure_interrupt();
    if (result) {
        return result;
    }

    result = vl53l0x_set_sequence_steps_enabled(
        RANGE_SEQUENCE_STEP_DSS + RANGE_SEQUENCE_STEP_PRE_RANGE + RANGE_SEQUENCE_STEP_FINAL_RANGE);
    return result;
}

static vl53l0x_result_e
vl53l0x_perform_single_ref_calibration(vl53l0x_calibration_type_e calib_type)
{
    uint8_t sysrange_start = 0;
    uint8_t sequence_config = 0;
    switch (calib_type) {
    case VL53L0X_CALIBRATION_TYPE_VHV:
        sequence_config = 0x01;
        sysrange_start = 0x01 | 0x40;
        break;
    case VL53L0X_CALIBRATION_TYPE_PHASE:
        sequence_config = 0x02;
        sysrange_start = 0x01 | 0x00;
        break;
    }
    if (i2c_write_addr8_data8(REG_SYSTEM_SEQUENCE_CONFIG, sequence_config)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    if (i2c_write_addr8_data8(REG_SYSRANGE_START, sysrange_start)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    // Wait for interrupt
    uint8_t interrupt_status = 0;
    i2c_result_e i2c_result = I2C_RESULT_OK;
    do {
        i2c_result = i2c_read_addr8_data8(REG_RESULT_INTERRUPT_STATUS, &interrupt_status);
    } while (i2c_result == I2C_RESULT_OK && ((interrupt_status & 0x07) == 0));
    if (i2c_result) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    if (i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    if (i2c_write_addr8_data8(REG_SYSRANGE_START, 0x00)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return VL53L0X_RESULT_OK;
}

/* Temperature calibration needs to be run again if the temperature changes by
 * more than 8 degrees according to the datasheet. */
static vl53l0x_result_e vl53l0x_perform_ref_calibration(void)
{
    vl53l0x_result_e result = vl53l0x_perform_single_ref_calibration(VL53L0X_CALIBRATION_TYPE_VHV);
    if (result) {
        return result;
    }
    result = vl53l0x_perform_single_ref_calibration(VL53L0X_CALIBRATION_TYPE_PHASE);
    if (result) {
        return result;
    }
    // Restore sequence steps enabled
    result = vl53l0x_set_sequence_steps_enabled(
        RANGE_SEQUENCE_STEP_DSS + RANGE_SEQUENCE_STEP_PRE_RANGE + RANGE_SEQUENCE_STEP_FINAL_RANGE);
    return result;
}

static vl53l0x_result_e vl53l0x_configure_address(uint8_t addr)
{
    // 7-bit address
    if (i2c_write_addr8_data8(REG_SLAVE_DEVICE_ADDRESS, addr & 0x7F)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return VL53L0X_RESULT_OK;
}

// Sets the sensor in hardware standby by flipping the XSHUT pin.
static void vl53l0x_set_hardware_standby(vl53l0x_idx_e idx, bool enable)
{
    io_set_out(vl53l0x_cfgs[idx].xshut_io, enable ? IO_OUT_LOW : IO_OUT_HIGH);
}

/* Configures the GPIOs used for the XSHUT pin.
 * Output low by default means the sensors will be in
 * hardware standby after this function is called. */
static void vl53l0x_assert_xshut_pins(void)
{
    static const struct io_config xshut_config = {
        .select = IO_SELECT_GPIO,
        .resistor = IO_RESISTOR_DISABLED,
        .dir = IO_DIR_OUTPUT,
        .out = IO_OUT_LOW,
    };
    struct io_config current_config;
    io_get_current_config(IO_XSHUT_FRONT, &current_config);
    ASSERT(io_config_compare(&xshut_config, &current_config));
#if defined(NSUMO)
    io_get_current_config(IO_XSHUT_FRONT_LEFT, &current_config);
    ASSERT(io_config_compare(&xshut_config, &current_config));
    io_get_current_config(IO_XSHUT_FRONT_RIGHT, &current_config);
    ASSERT(io_config_compare(&xshut_config, &current_config));
    io_get_current_config(IO_XSHUT_LEFT, &current_config);
    ASSERT(io_config_compare(&xshut_config, &current_config));
    io_get_current_config(IO_XSHUT_RIGHT, &current_config);
    ASSERT(io_config_compare(&xshut_config, &current_config));
#endif
}

/* Sets the address of a single VL53L0X sensor.
 * This functions assumes that all non-configured VL53L0X are still
 * in hardware standby. */
static vl53l0x_result_e vl53l0x_init_address(vl53l0x_idx_e idx)
{
    vl53l0x_set_hardware_standby(idx, false);
    i2c_set_slave_address(VL53L0X_DEFAULT_ADDRESS);

    /* The datasheet doesn't say how long we must wait to leave hw standby,
     * but using the same delay as vl6180x seems to work fine. */
    /* TODO: Tune this delay */
    __delay_cycles(10000);

    vl53l0x_result_e result = device_is_booted();
    if (result) {
        return result;
    }
    result = vl53l0x_configure_address(vl53l0x_cfgs[idx].addr);
    return result;
}

/* Initializes the sensors by putting them in hw standby and then
 * waking them up one-by-one as described in AN4846. */
static vl53l0x_result_e vl53l0x_init_addresses(void)
{
    // Default IO config should put all sensors in hardware standby
    vl53l0x_assert_xshut_pins();

    // Wake each sensor up one by one and set a unique address for each one
    vl53l0x_result_e result = vl53l0x_init_address(VL53L0X_IDX_FRONT);
    if (result) {
        return result;
    }
#if defined(NSUMO)
    result = vl53l0x_init_address(VL53L0X_IDX_LEFT);
    if (result) {
        return result;
    }
    result = vl53l0x_init_address(VL53L0X_IDX_RIGHT);
    if (result) {
        return result;
    }
    result = vl53l0x_init_address(VL53L0X_IDX_FRONT_LEFT);
    if (result) {
        return result;
    }
    result = vl53l0x_init_address(VL53L0X_IDX_FRONT_RIGHT);
    if (result) {
        return result;
    }
#endif
    return VL53L0X_RESULT_OK;
}

static vl53l0x_result_e vl53l0x_init_config(vl53l0x_idx_e idx)
{
    i2c_set_slave_address(vl53l0x_cfgs[idx].addr);
    vl53l0x_result_e result = vl53l0x_data_init();
    if (result) {
        return result;
    }
    result = vl53l0x_static_init();
    if (result) {
        return result;
    }
    result = vl53l0x_perform_ref_calibration();
    if (result) {
        return result;
    }

    if (idx == VL53L0X_IDX_FRONT) {
        vl53l0x_configure_front_sensor_interrupt();
    }
    return VL53L0X_RESULT_OK;
}

static vl53l0x_result_e vl53l0x_start_sysrange(vl53l0x_idx_e idx)
{
    i2c_set_slave_address(vl53l0x_cfgs[idx].addr);
    const struct i2c_8reg sysrange_regs[] = { { 0x80, 0x01 }, { 0xFF, 0x01 }, { 0x00, 0x00 },
                                              { 0x00, 0x01 }, { 0xFF, 0x00 }, { 0x80, 0x00 } };
    vl53l0x_result_e result = vl53l0x_write_8regs(sysrange_regs, 3);
    if (result) {
        return result;
    }
    if (i2c_write_addr8_data8(0x91, stop_variable)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    result = vl53l0x_write_8regs(sysrange_regs + 3, ARRAY_SIZE(sysrange_regs) - 3);
    if (result) {
        return result;
    }

    if (i2c_write_addr8_data8(REG_SYSRANGE_START, 0x01)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    uint8_t sysrange_start = 0;
    i2c_result_e i2c_result = I2C_RESULT_OK;
    do {
        i2c_result = i2c_read_addr8_data8(REG_SYSRANGE_START, &sysrange_start);
    } while (i2c_result == I2C_RESULT_OK && (sysrange_start & 0x01));
    return i2c_result == I2C_RESULT_OK ? VL53L0X_RESULT_OK : VL53L0X_RESULT_ERROR_I2C;
}

// Assumes I2C address is set already
static vl53l0x_result_e vl53l0x_clear_sysrange_interrupt(void)
{
    if (i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }
    return VL53L0X_RESULT_OK;
}

// Assumes I2C address is set already
static vl53l0x_result_e vl53l0x_pollwait_sysrange(void)
{
    i2c_result_e i2c_result = I2C_RESULT_OK;
    uint8_t interrupt_status = 0;
    do {
        i2c_result = i2c_read_addr8_data8(REG_RESULT_INTERRUPT_STATUS, &interrupt_status);
    } while (i2c_result == I2C_RESULT_OK && ((interrupt_status & 0x07) == 0));
    return i2c_result == I2C_RESULT_OK ? VL53L0X_RESULT_OK : VL53L0X_RESULT_ERROR_I2C;
}

static bool vl53l0x_is_sysrange_done(vl53l0x_idx_e idx)
{
#if defined(LAUNCHPAD)
    if (idx != VL53L0X_IDX_FRONT) {
        return true;
    }
#endif
    i2c_set_slave_address(vl53l0x_cfgs[idx].addr);
    uint8_t interrupt_status = 0;
    const i2c_result_e i2c_result =
        i2c_read_addr8_data8(REG_RESULT_INTERRUPT_STATUS, &interrupt_status);
    return i2c_result == I2C_RESULT_OK && (interrupt_status & 0x07);
}

static vl53l0x_result_e vl53l0x_read_range(vl53l0x_idx_e idx, uint16_t *range)
{
    i2c_set_slave_address(vl53l0x_cfgs[idx].addr);

    vl53l0x_result_e result = vl53l0x_pollwait_sysrange();
    if (result) {
        return result;
    }

    if (i2c_read_addr8_data16(REG_RESULT_RANGE_STATUS + 10, range)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    if (i2c_write_addr8_data8(REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return VL53L0X_RESULT_ERROR_I2C;
    }

    // 8190 or 8191 may be returned when obstacle is out of range.
    if (*range == 8190 || *range == 8191) {
        *range = VL53L0X_OUT_OF_RANGE;
    }

    result = vl53l0x_clear_sysrange_interrupt();
    return result;
}

vl53l0x_result_e vl53l0x_read_range_single(vl53l0x_idx_e idx, uint16_t *range)
{
    ASSERT(initialized);
    vl53l0x_result_e result = vl53l0x_start_sysrange(idx);
    if (result) {
        return result;
    }
    result = vl53l0x_read_range(idx, range);
    return result;
}

// TODO: Verify this works after bring up real robot
vl53l0x_result_e vl53l0x_start_measuring_multiple(void)
{
    ASSERT(initialized);
    if (status_multiple == STATUS_MULTIPLE_MEASURING) {
        return VL53L0X_RESULT_ERROR_MEASURE_ONGOING;
    }
    status_multiple = STATUS_MULTIPLE_MEASURING;
    vl53l0x_result_e result = vl53l0x_start_sysrange(VL53L0X_IDX_FRONT);
    if (result) {
        return result;
    }
#if defined(NSUMO)
    result = vl53l0x_start_sysrange(VL53L0X_IDX_FRONT_LEFT);
    if (result) {
        return result;
    }
    result = vl53l0x_start_sysrange(VL53L0X_IDX_FRONT_RIGHT);
    if (result) {
        return result;
    }
#endif
#if 0 // Skip left and right, since they are are mounted badly
    result = vl53l0x_start_sysrange(VL53L0X_IDX_LEFT);
    if (result) {
        return result;
    }
    result = vl53l0x_start_sysrange(VL53L0X_IDX_RIGHT);
    if (result) {
        return result;
    }
#endif
    return VL53L0X_RESULT_OK;
}

static vl53l0x_ranges_t latest_ranges = { VL53L0X_OUT_OF_RANGE, VL53L0X_OUT_OF_RANGE,
                                          VL53L0X_OUT_OF_RANGE, VL53L0X_OUT_OF_RANGE,
                                          VL53L0X_OUT_OF_RANGE };

/*
 * The approach is as follow:
 * For multiple sensors and single interrupt line:
 * 1. Start measure on all sensors
 * 2. If measurement ready
 *    - Read measurement of all sensors
 * 3. else
 *    - Return old values
 * 4. Update measurement ready on interrupt
 */
// TODO: Verify this works after bring up real robot
vl53l0x_result_e vl53l0x_read_range_multiple(vl53l0x_ranges_t ranges, bool *fresh_values)
{
    ASSERT(initialized);
    vl53l0x_result_e result = VL53L0X_RESULT_OK;
    if (status_multiple == STATUS_MULTIPLE_NOT_STARTED) {
        result = vl53l0x_start_measuring_multiple();
        if (result) {
            return result;
        }
        // Block here the first time
        while (status_multiple != STATUS_MULTIPLE_DONE) { }
    }

    if (status_multiple == STATUS_MULTIPLE_DONE
        // We already know front is done because of its interrupt
        //&& vl53l0x_is_sysrange_done(VL53L0X_IDX_FRONT)
        && vl53l0x_is_sysrange_done(VL53L0X_IDX_LEFT) && vl53l0x_is_sysrange_done(VL53L0X_IDX_RIGHT)
        && vl53l0x_is_sysrange_done(VL53L0X_IDX_FRONT_LEFT)
        && vl53l0x_is_sysrange_done(VL53L0X_IDX_FRONT_RIGHT)) {

        if (!vl53l0x_is_sysrange_done(VL53L0X_IDX_FRONT)) {
            // Sanity check!
            ASSERT(false);
        }

        result = vl53l0x_read_range(VL53L0X_IDX_FRONT, &latest_ranges[VL53L0X_IDX_FRONT]);
        if (result) {
            return result;
        }
#if defined(NSUMO)
        result = vl53l0x_read_range(VL53L0X_IDX_FRONT_LEFT, &latest_ranges[VL53L0X_IDX_FRONT_LEFT]);
        if (result) {
            return result;
        }
        result =
            vl53l0x_read_range(VL53L0X_IDX_FRONT_RIGHT, &latest_ranges[VL53L0X_IDX_FRONT_RIGHT]);
        if (result) {
            return result;
        }
#endif
#if 0 // Skip left and right, since they are are mounted badly
        result = vl53l0x_read_range(VL53L0X_IDX_LEFT, &latest_ranges[VL53L0X_IDX_LEFT]);
        if (result) {
            return result;
        }
        result = vl53l0x_read_range(VL53L0X_IDX_RIGHT, &latest_ranges[VL53L0X_IDX_RIGHT]);
        if (result) {
            return result;
        }
#endif
        result = vl53l0x_start_measuring_multiple();
        if (result) {
            return result;
        }
        *fresh_values = true;
    } else {
        *fresh_values = false;
    }
    for (int i = 0; i < VL53L0X_IDX_COUNT; i++) {
        ranges[i] = latest_ranges[i];
    }
    return result;
}

vl53l0x_result_e vl53l0x_init(void)
{
    ASSERT(!initialized);

    // TODO: Tune this delay (required to make the VL53L0X communication work)
    __delay_cycles(5000);

    i2c_init();

    vl53l0x_result_e result = vl53l0x_init_addresses();
    if (result) {
        return result;
    }
    result = vl53l0x_init_config(VL53L0X_IDX_FRONT);
    if (result) {
        return result;
    }
#if defined(NSUMO)
    result = vl53l0x_init_config(VL53L0X_IDX_LEFT);
    if (result) {
        return result;
    }
    result = vl53l0x_init_config(VL53L0X_IDX_RIGHT);
    if (result) {
        return result;
    }
    result = vl53l0x_init_config(VL53L0X_IDX_FRONT_LEFT);
    if (result) {
        return result;
    }
    result = vl53l0x_init_config(VL53L0X_IDX_FRONT_RIGHT);
    if (result) {
        return result;
    }
#endif
    initialized = true;
    return VL53L0X_RESULT_OK;
}
