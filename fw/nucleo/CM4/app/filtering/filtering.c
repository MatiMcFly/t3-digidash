#include "filtering.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "queue.h"
#include "shared.h"

#define FILTERING_SIZE 5

static int16_t filter_water_temperature(int16_t value);
static int16_t filter_battery_voltage(int16_t value);
static int16_t mean(int16_t values[], uint8_t size);

void filtering_task(void* params)
{
    sensor_data_t data;

    while (true) {
        if (xQueueReceive(queue_data_converted, &data, portMAX_DELAY) == pdTRUE) {
            switch (data.id) {
                case SENSOR_ID_WATER_TEMPERATURE:
                    data.value = filter_water_temperature(data.value);
                    break;

                case SENSOR_ID_BATTERY_VOLTAGE:
                    data.value = filter_battery_voltage(data.value);
                    break;

                default:
                    HAL_UART_Transmit(&huart3, (uint8_t*)"filtering: Unknown sensor id\n", strlen("filtering: Unknown sensor id\n"), HAL_MAX_DELAY);
                    continue;
            }

            if (xQueueSend(queue_data_filtered, &data, pdMS_TO_TICKS(20)) != pdTRUE) {
                HAL_UART_Transmit(&huart3, (uint8_t*)"filtering: xQueueSend error\n", strlen("filtering: xQueueSend error\n"), HAL_MAX_DELAY);
            }
        }
    }
}

static int16_t filter_water_temperature(int16_t value)
{
    static int16_t ringbuf[FILTERING_SIZE] = {0};
    static uint8_t index                   = 0;

    ringbuf[index] = value;
    index          = (index + 1) % FILTERING_SIZE;

    return mean(ringbuf, FILTERING_SIZE);
}

static int16_t filter_battery_voltage(int16_t value)
{
    static int16_t ringbuf[FILTERING_SIZE] = {0};
    static uint8_t index                   = 0;

    ringbuf[index] = value;
    index          = (index + 1) % FILTERING_SIZE;

    return mean(ringbuf, FILTERING_SIZE);
}

static int16_t mean(int16_t values[], uint8_t size)
{
    int32_t sum = 0;

    for (uint8_t i = 0; i < size; i++) {
        sum += values[i];
    }

    return (int16_t)(sum / size);
}
