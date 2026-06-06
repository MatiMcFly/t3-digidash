#include "tasks.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "acquisition.h"
#include "app.h"
#include "conversion.h"
#include "filtering.h"
#include "publication.h"
#include "shared.h"

/**
 * @brief Acquisition task for sensor data
 *
 * @param params -- Unused
 */
void acquisition_task(void* params)
{
    TickType_t last_wakeup = xTaskGetTickCount();

    (void)params; // Unused

    while (true) {

        // Start all analog sensor acquisitions
        if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc1_buffer, ADC1_BUFFER_SIZE) != HAL_OK) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: HAL_ADC_Start_DMA error\n", strlen("acquisition: HAL_ADC_Start_DMA error\n"), HAL_MAX_DELAY);
        }

        // Read all binary sensors
        acquisition_binary_sensors();

        // Start all pulse sensor acquisitions
        acquisition_pulse_sensors();

        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(MEASUREMENT_PERIOD_MS));
    }
}

/**
 * @brief Conversion task for sensor data
 *
 * @param params -- Unused
 */
void conversion_task(void* params)
{
    sensor_data_t data;

    (void)params; // Unused

    while (true) {
        if (xQueueReceive(queue_data_raw, &data, portMAX_DELAY) != pdPASS) {
            continue;
        }

        switch (data.id) {
            case SENSOR_ID_COOLANT_TEMPERATURE:
                data.value = conversion_coolant_temperature(data.value);
                break;

            case SENSOR_ID_BATTERY_VOLTAGE:
                data.value = conversion_battery_voltage(data.value);
                break;

            case SENSOR_ID_FUEL_LEVEL:
                data.value = conversion_fuel_level(data.value);
                break;

            case SENSOR_ID_TURN_SIGNAL:
            case SENSOR_ID_HIGH_BEAM:
            case SENSOR_ID_OIL_PRESSURE_0_3_BAR:
            case SENSOR_ID_OIL_PRESSURE_1_8_BAR:
                // No conversion needed for binary signals
                break;

            case SENSOR_ID_MOTOR_RPM:
                data.value = conversion_motor_rpm(data.value);
                break;

            default:
                HAL_UART_Transmit(&huart3, (uint8_t*)"conversion: Unknown sensor id\n", strlen("conversion: Unknown sensor id\n"), HAL_MAX_DELAY);
                continue;
        }

        if (xQueueSend(queue_data_converted, &data, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"conversion: xQueueSend error\n", strlen("conversion: xQueueSend error\n"), HAL_MAX_DELAY);
        }
    }
}

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
        if (xQueueReceive(queue_data_converted, &data, portMAX_DELAY) != pdPASS) {
            continue;
        }

        switch (data.id) {
            case SENSOR_ID_COOLANT_TEMPERATURE:
                data.value = filtering_filter_coolant_temperature(data.value);
                break;

            case SENSOR_ID_BATTERY_VOLTAGE:
                data.value = filtering_filter_battery_voltage(data.value);
                break;

            case SENSOR_ID_FUEL_LEVEL:
                data.value = filtering_filter_fuel_level(data.value);
                break;

            case SENSOR_ID_TURN_SIGNAL:
                data.value = filtering_debounce_turn_signal(data.value);
                break;

            case SENSOR_ID_HIGH_BEAM:
                data.value = filtering_debounce_high_beam(data.value);
                break;

            case SENSOR_ID_OIL_PRESSURE_0_3_BAR:
                data.value = filtering_debounce_oil_pressure_0_3_bar(data.value);
                break;

            case SENSOR_ID_OIL_PRESSURE_1_8_BAR:
                data.value = filtering_debounce_oil_pressure_1_8_bar(data.value);
                break;

            case SENSOR_ID_MOTOR_RPM:
                data.value = filtering_filter_motor_rpm(data.value);
                break;

            default:
                HAL_UART_Transmit(&huart3, (uint8_t*)"filtering: Unknown sensor id\n", strlen("filtering: Unknown sensor id\n"), HAL_MAX_DELAY);
                continue;
        }

        if (xQueueSend(queue_data_filtered, &data, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"filtering: xQueueSend error\n", strlen("filtering: xQueueSend error\n"), HAL_MAX_DELAY);
        }
    }
}

/**
 * @brief Publication task
 *
 * @param params -- Unused
 */
void publication_task(void* params)
{
    sensor_data_t data;
    char          string[50] = "";

    (void)params; // Unused

    while (true) {
        if (xQueueReceive(queue_data_filtered, &data, portMAX_DELAY) != pdPASS) {
            continue;
        }

        switch (data.id) {
            case SENSOR_ID_COOLANT_TEMPERATURE:
                snprintf(string, sizeof(string), "Coolant Temperature:  %d dC\n", data.value);
                break;

            case SENSOR_ID_BATTERY_VOLTAGE:
                snprintf(string, sizeof(string), "Battery Voltage:      %d cV\n", data.value);
                break;

            case SENSOR_ID_FUEL_LEVEL:
                snprintf(string, sizeof(string), "Fuel Level:           %d dl\n", data.value);
                break;

            case SENSOR_ID_TURN_SIGNAL:
                snprintf(string, sizeof(string), "Turn Signal:          %d\n", data.value);
                break;

            case SENSOR_ID_HIGH_BEAM:
                snprintf(string, sizeof(string), "High Beam:            %d\n", data.value);
                break;

            case SENSOR_ID_OIL_PRESSURE_0_3_BAR:
                snprintf(string, sizeof(string), "Oil Pressure 0.3 bar: %d\n", data.value);
                break;

            case SENSOR_ID_OIL_PRESSURE_1_8_BAR:
                snprintf(string, sizeof(string), "Oil Pressure 1.8 bar: %d\n", data.value);
                break;

            case SENSOR_ID_MOTOR_RPM:
                snprintf(string, sizeof(string), "Motor RPM:            %d rpm\n", data.value);
                break;

            default:
                HAL_UART_Transmit(&huart3, (uint8_t*)"publication: Unknown sensor id\n", strlen("publication: Unknown sensor id\n"), HAL_MAX_DELAY);
                continue;
        }

        HAL_UART_Transmit(&huart3, (uint8_t*)string, strlen(string), HAL_MAX_DELAY);

        if (xMessageBufferSend(ipc_message_buffer, &data, sizeof(data), 0) != sizeof(data)) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"publication: xMessageBufferSend error\n", strlen("publication: xMessageBufferSend error\n"), HAL_MAX_DELAY);
            continue;
        }
    }
}
