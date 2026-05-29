#include "acquisition.h"

#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "main.h"
#include "queue.h"
#include "shared.h"
#include "task.h"

extern ADC_HandleTypeDef hadc1;

#define MEASUREMENT_PERIOD_MS 100

#define ADC1_BUFFER_SIZE 3
static volatile uint16_t adc1_buffer[ADC1_BUFFER_SIZE] = {0};

static void acquire_binary_sensors(void);

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
        acquire_binary_sensors();

        // Start all pulse sensor acquisitions (TODO)

        vTaskDelayUntil(&last_wakeup, pdMS_TO_TICKS(MEASUREMENT_PERIOD_MS));
    }
}

static void acquire_binary_sensors(void)
{
    sensor_data_t turn_signal          = {.id = SENSOR_ID_TURN_SIGNAL, .value = 0};
    sensor_data_t high_beam            = {.id = SENSOR_ID_HIGH_BEAM, .value = 0};
    sensor_data_t oil_pressure_0_3_bar = {.id = SENSOR_ID_OIL_PRESSURE_0_3_BAR, .value = 0};
    sensor_data_t oil_pressure_1_8_bar = {.id = SENSOR_ID_OIL_PRESSURE_1_8_BAR, .value = 0};

    turn_signal.value          = (int16_t)HAL_GPIO_ReadPin(SENSOR_05_TURN_SIGNAL_GPIO_Port, SENSOR_05_TURN_SIGNAL_Pin);
    high_beam.value            = (int16_t)HAL_GPIO_ReadPin(SENSOR_06_HIGH_BEAM_GPIO_Port, SENSOR_06_HIGH_BEAM_Pin);
    oil_pressure_0_3_bar.value = (int16_t)HAL_GPIO_ReadPin(SENSOR_07_OIL_PRESSURE_0_3_BAR_GPIO_Port, SENSOR_07_OIL_PRESSURE_0_3_BAR_Pin);
    oil_pressure_1_8_bar.value = (int16_t)HAL_GPIO_ReadPin(SENSOR_08_OIL_PRESSURE_1_8_BAR_GPIO_Port, SENSOR_08_OIL_PRESSURE_1_8_BAR_Pin);

    if (xQueueSend(queue_data_raw, &turn_signal, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSend error\n", strlen("acquisition: xQueueSend error\n"), HAL_MAX_DELAY);
    }

    if (xQueueSend(queue_data_raw, &high_beam, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSend error\n", strlen("acquisition: xQueueSend error\n"), HAL_MAX_DELAY);
    }

    if (xQueueSend(queue_data_raw, &oil_pressure_0_3_bar, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSend error\n", strlen("acquisition: xQueueSend error\n"), HAL_MAX_DELAY);
    }

    if (xQueueSend(queue_data_raw, &oil_pressure_1_8_bar, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSend error\n", strlen("acquisition: xQueueSend error\n"), HAL_MAX_DELAY);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    if (hadc->Instance == ADC1) {
        sensor_data_t coolant_temperature = {.id = SENSOR_ID_COOLANT_TEMPERATURE, .value = 0};
        sensor_data_t battery_voltage     = {.id = SENSOR_ID_BATTERY_VOLTAGE, .value = 0};
        sensor_data_t fuel_level          = {.id = SENSOR_ID_FUEL_LEVEL, .value = 0};

        coolant_temperature.value = adc1_buffer[0];
        battery_voltage.value     = adc1_buffer[1];
        fuel_level.value          = adc1_buffer[2];

        if (xQueueSendFromISR(queue_data_raw, &coolant_temperature, &higher_priority_task_woken) != pdPASS) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSendFromISR error\n", strlen("acquisition: xQueueSendFromISR error\n"), HAL_MAX_DELAY);
        }

        if (xQueueSendFromISR(queue_data_raw, &battery_voltage, &higher_priority_task_woken) != pdPASS) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSendFromISR error\n", strlen("acquisition: xQueueSendFromISR error\n"), HAL_MAX_DELAY);
        }

        if (xQueueSendFromISR(queue_data_raw, &fuel_level, &higher_priority_task_woken) != pdPASS) {
            HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSendFromISR error\n", strlen("acquisition: xQueueSendFromISR error\n"), HAL_MAX_DELAY);
        }
    } else {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: Unknown ADC instance\n", strlen("acquisition: Unknown ADC instance\n"), HAL_MAX_DELAY);
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}
