#include "filtering.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app.h"

#define FILTER_SIZE_COOLANT_TEMPERATURE 20
#define FILTER_SIZE_BATTERY_VOLTAGE     20
#define FILTER_SIZE_FUEL_LEVEL          300
#define FILTER_SIZE_MOTOR_RPM           2

#define DEBOUNCE_SIZE_TURN_SIGNAL          2
#define DEBOUNCE_SIZE_HIGH_BEAM            2
#define DEBOUNCE_SIZE_OIL_PRESSURE_0_3_BAR 2
#define DEBOUNCE_SIZE_OIL_PRESSURE_1_8_BAR 2

static void    ringbuf_update(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value);
static int16_t ringbuf_mean(int16_t ringbuf[], uint16_t size);
static bool    ringbuf_is_consistent(int16_t ringbuf[], uint16_t size);

int16_t filtering_filter_coolant_temperature(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_COOLANT_TEMPERATURE] = {0};
    static uint16_t index                                    = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    return ringbuf_mean(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]));
}

int16_t filtering_filter_battery_voltage(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_BATTERY_VOLTAGE] = {0};
    static uint16_t index                                = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    return ringbuf_mean(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]));
}

int16_t filtering_filter_fuel_level(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_FUEL_LEVEL] = {0};
    static uint16_t index                           = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    return ringbuf_mean(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]));
}

int16_t filtering_filter_motor_rpm(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_MOTOR_RPM] = {0};
    static uint16_t index                          = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    return ringbuf_mean(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]));
}

int16_t filtering_debounce_turn_signal(int16_t value)
{
    static int16_t  ringbuf[DEBOUNCE_SIZE_TURN_SIGNAL] = {0};
    static uint16_t index                              = 0;
    static int16_t  previous_value                     = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    if (ringbuf_is_consistent(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]))) {
        previous_value = value;
    }

    return previous_value;
}

int16_t filtering_debounce_high_beam(int16_t value)
{
    static int16_t  ringbuf[DEBOUNCE_SIZE_HIGH_BEAM] = {0};
    static uint16_t index                            = 0;
    static int16_t  previous_value                   = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    if (ringbuf_is_consistent(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]))) {
        previous_value = value;
    }

    return previous_value;
}

int16_t filtering_debounce_oil_pressure_0_3_bar(int16_t value)
{
    static int16_t  ringbuf[DEBOUNCE_SIZE_OIL_PRESSURE_0_3_BAR] = {0};
    static uint16_t index                                       = 0;
    static int16_t  previous_value                              = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    if (ringbuf_is_consistent(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]))) {
        previous_value = value;
    }

    return previous_value;
}

int16_t filtering_debounce_oil_pressure_1_8_bar(int16_t value)
{
    static int16_t  ringbuf[DEBOUNCE_SIZE_OIL_PRESSURE_1_8_BAR] = {0};
    static uint16_t index                                       = 0;
    static int16_t  previous_value                              = 0;

    ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);

    if (ringbuf_is_consistent(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]))) {
        previous_value = value;
    }

    return previous_value;
}

/**
 * @brief Update ring buffer with a new value
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 * @param index   -- Pointer to the current index in the ring buffer
 * @param value   -- New value to add to the ring buffer
 */
static void ringbuf_update(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value)
{
    if (size == 0) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_update: Size cannot be zero\n", strlen("ringbuf_update: Size cannot be zero\n"), HAL_MAX_DELAY);
        return; // Invalid size
    }

    if (*index >= size) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_update: Invalid index\n", strlen("ringbuf_update: Invalid index\n"), HAL_MAX_DELAY);
        *index = 0;
        return; // Invalid index
    }

    ringbuf[*index] = value;
    *index          = (*index + 1) % size;
}

/**
 * @brief Calculate the mean of a ring buffer
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 *
 * @return int16_t -- Mean value
 */
static int16_t ringbuf_mean(int16_t ringbuf[], uint16_t size)
{
    int32_t sum = 0;

    if (size == 0) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_mean: Size cannot be zero\n", strlen("ringbuf_mean: Size cannot be zero\n"), HAL_MAX_DELAY);
        return 0; // Invalid size
    }

    for (uint16_t i = 0; i < size; i++) {
        sum += ringbuf[i];
    }

    return (int16_t)(sum / size);
}

/**
 * @brief Check if all values in the ring buffer are the same
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 *
 * @return true -- All elements in the ring buffer have the same value
 * @return false -- Not all elements in the ring buffer have the same value
 */
static bool ringbuf_is_consistent(int16_t ringbuf[], uint16_t size)
{
    if (size == 0) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_is_consistent: Size cannot be zero\n", strlen("ringbuf_is_consistent: Size cannot be zero\n"), HAL_MAX_DELAY);
        return false; // Invalid size
    }

    int16_t value = ringbuf[0];

    for (uint16_t i = 1; i < size; i++) {
        if (ringbuf[i] != value) {
            return false;
        }
    }

    return true;
}
