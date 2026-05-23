#include "conversion.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "queue.h"
#include "shared.h"

static int16_t convert_water_temperature(int16_t raw_value);
static int16_t convert_battery_voltage(int16_t raw_value);

void conversion_task(void* params)
{
    sensor_data_t data;

    while (true) {
        if (xQueueReceive(queue_data_raw, &data, portMAX_DELAY) == pdTRUE) {
            switch (data.id) {
                case SENSOR_ID_WATER_TEMPERATURE:
                    data.value = convert_water_temperature(data.value);
                    break;

                case SENSOR_ID_BATTERY_VOLTAGE:
                    data.value = convert_battery_voltage(data.value);
                    break;

                default:
                    HAL_UART_Transmit(&huart3, (uint8_t*)"conversion: Unknown sensor id\n", strlen("conversion: Unknown sensor id\n"), HAL_MAX_DELAY);
                    continue;
            }

            if (xQueueSend(queue_data_converted, &data, pdMS_TO_TICKS(20)) != pdTRUE) {
                HAL_UART_Transmit(&huart3, (uint8_t*)"conversion: xQueueSend error\n", strlen("conversion: xQueueSend error\n"), HAL_MAX_DELAY);
            }
        }
    }
}

static int16_t convert_water_temperature(int16_t raw_value)
{
    // TODO: apply water temperature conversion
    return raw_value;
}

static int16_t convert_battery_voltage(int16_t raw_value)
{
    // TODO: apply battery voltage conversion
    return raw_value;
}
