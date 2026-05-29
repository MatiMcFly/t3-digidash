#include "filtering.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "queue.h"
#include "shared.h"

#define FILTER_SIZE_COOLANT_TEMPERATURE 20
#define FILTER_SIZE_BATTERY_VOLTAGE     20
#define FILTER_SIZE_FUEL_LEVEL          300

static int16_t filter_coolant_temperature(int16_t value);
static int16_t filter_battery_voltage(int16_t value);
static int16_t filter_fuel_level(int16_t value);
static int16_t ringbuf_update(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value);
static int16_t mean(int16_t values[], uint16_t size);

/**
 * @brief Filtering task for sensor data
 *
 * @param params -- Unused
 */
void filtering_task(void* params)
{
    sensor_data_t data;

    (void)params; // Unused

    while (true) {
        if (xQueueReceive(queue_data_converted, &data, portMAX_DELAY) == pdPASS) {
            switch (data.id) {
                case SENSOR_ID_COOLANT_TEMPERATURE:
                    data.value = filter_coolant_temperature(data.value);
                    break;

                case SENSOR_ID_BATTERY_VOLTAGE:
                    data.value = filter_battery_voltage(data.value);
                    break;

                case SENSOR_ID_FUEL_LEVEL:
                    data.value = filter_fuel_level(data.value);
                    break;

                default:
                    HAL_UART_Transmit(&huart3, (uint8_t*)"filtering: Unknown sensor id\n", strlen("filtering: Unknown sensor id\n"), HAL_MAX_DELAY);
                    continue;
            }

            if (xQueueSend(queue_data_filtered, &data, pdMS_TO_TICKS(20)) != pdPASS) {
                HAL_UART_Transmit(&huart3, (uint8_t*)"filtering: xQueueSend error\n", strlen("filtering: xQueueSend error\n"), HAL_MAX_DELAY);
            }
        }
    }
}

static int16_t filter_coolant_temperature(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_COOLANT_TEMPERATURE] = {0};
    static uint16_t index                                    = 0;

    return ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);
}

static int16_t filter_battery_voltage(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_BATTERY_VOLTAGE] = {0};
    static uint16_t index                                = 0;

    return ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);
}

static int16_t filter_fuel_level(int16_t value)
{
    static int16_t  ringbuf[FILTER_SIZE_FUEL_LEVEL] = {0};
    static uint16_t index                           = 0;

    return ringbuf_update(ringbuf, sizeof(ringbuf) / sizeof(ringbuf[0]), &index, value);
}

/**
 * @brief Update ring buffer with a new value and recalculate the mean
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 * @param index   -- Pointer to the current index in the ring buffer
 * @param value   -- New value to add to the ring buffer

 * @return int16_t -- New mean after adding the new value
 */
static int16_t ringbuf_update(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value)
{
    if (*index >= size) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_update: Invalid index\n", strlen("ringbuf_update: Invalid index\n"), HAL_MAX_DELAY);
        *index = 0;
        return 0; // Invalid index
    }

    if (size == 0) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_update: Size cannot be zero\n", strlen("ringbuf_update: Size cannot be zero\n"), HAL_MAX_DELAY);
        return 0; // Invalid size
    }

    ringbuf[*index] = value;
    *index          = (*index + 1) % size;

    return mean(ringbuf, size);
}

/**
 * @brief Calculate the mean of an array
 *
 * @param values -- Array of values
 * @param size   -- Number of elements in the array
 *
 * @return int16_t -- Mean value
 */
static int16_t mean(int16_t values[], uint16_t size)
{
    int32_t sum = 0;

    if (size == 0) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"mean: Size cannot be zero\n", strlen("mean: Size cannot be zero\n"), HAL_MAX_DELAY);
        return 0; // Invalid size
    }

    for (uint16_t i = 0; i < size; i++) {
        sum += values[i];
    }

    return (int16_t)(sum / size);
}
