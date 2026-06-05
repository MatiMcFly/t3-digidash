#include "acquisition.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "main.h"
#include "queue.h"
#include "shared.h"
#include "task.h"

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;

#define MEASUREMENT_PERIOD_MS 100

#define ADC1_BUFFER_SIZE 3
static volatile uint16_t adc1_buffer[ADC1_BUFFER_SIZE] = {0};

static volatile bool tim2_is_first_capture = true;

static void    acquire_binary_sensors(void);
static void    start_pulse_sensors(void);
static int16_t pulse_period_to_pulses_per_minute(uint32_t pulse_period_us);

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

        // Start all pulse sensor acquisitions
        start_pulse_sensors();

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

static void start_pulse_sensors(void)
{
    if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) == HAL_OK) { // SENSOR_ID_ROTATION_SPEE
        return;
    }

    // Timer could not be started, because it is still running --> timeout after 0.1 s
    // Therefore, reset the timer and send 0 pulses per minute
    HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
    tim2_is_first_capture = true;
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);

    sensor_data_t rotation_speed = {.id = SENSOR_ID_ROTATION_SPEED, .value = 0};

    if (xQueueSend(queue_data_raw, &rotation_speed, pdMS_TO_TICKS(QUEUE_TIMEOUT_MS)) != pdPASS) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSend error\n", strlen("acquisition: xQueueSend error\n"), HAL_MAX_DELAY);
    }
}

static int16_t pulse_period_to_pulses_per_minute(uint32_t pulse_period_us)
{
    if (pulse_period_us == 0) {
        return 0;
    }

    // 1 us --> 1'000'000 pulses per second
    // 1 pulse per second  --> 60 pulses per minute
    // ==> 60 * 1'000'000 / capture_value_us --> pulses_per_min
    const uint32_t US_PER_S  = 1000000;
    const uint32_t S_PER_MIN = 60;

    // Need to ensure that pulses_per_min will not exceed int16_t
    const uint32_t MIN_PULSE_PERIOD_US = S_PER_MIN * US_PER_S / 0x7FFF;

    if (pulse_period_us < MIN_PULSE_PERIOD_US) {
        pulse_period_us = MIN_PULSE_PERIOD_US;
    }

    int16_t pulses_per_min = (int16_t)(S_PER_MIN * US_PER_S / pulse_period_us);

    return pulses_per_min;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    BaseType_t    higher_priority_task_woken = pdFALSE;
    sensor_data_t coolant_temperature        = {.id = SENSOR_ID_COOLANT_TEMPERATURE, .value = 0};
    sensor_data_t battery_voltage            = {.id = SENSOR_ID_BATTERY_VOLTAGE, .value = 0};
    sensor_data_t fuel_level                 = {.id = SENSOR_ID_FUEL_LEVEL, .value = 0};

    if (hadc->Instance != ADC1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: Unknown ADC instance\n", strlen("acquisition: Unknown ADC instance\n"), HAL_MAX_DELAY);
        return;
    }

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

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim)
{
    static uint32_t capture_value_prev_us      = 0;
    BaseType_t      higher_priority_task_woken = pdFALSE;
    sensor_data_t   rotation_speed             = {.id = SENSOR_ID_ROTATION_SPEED, .value = 0};

    if (htim->Instance != TIM2 || htim->Channel != HAL_TIM_ACTIVE_CHANNEL_1) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: Unknown timer instance or channel\n", strlen("acquisition: Unknown timer instance or channel\n"), HAL_MAX_DELAY);
        return;
    }

    uint32_t capture_value_us = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

    if (tim2_is_first_capture) {
        capture_value_prev_us = capture_value_us;
        tim2_is_first_capture = false;
        return;
    }

    uint32_t capture_diff_us = capture_value_us - capture_value_prev_us; // unsinged subtraction handles timer overflow

    rotation_speed.value = pulse_period_to_pulses_per_minute(capture_diff_us);

    capture_value_prev_us = capture_value_us;

    if (xQueueSendFromISR(queue_data_raw, &rotation_speed, &higher_priority_task_woken) != pdPASS) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"acquisition: xQueueSendFromISR error\n", strlen("acquisition: xQueueSendFromISR error\n"), HAL_MAX_DELAY);
    }

    // Reset and get ready for next measurement period
    HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);
    tim2_is_first_capture = true;

    portYIELD_FROM_ISR(higher_priority_task_woken);
}
