#include "conversion.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "queue.h"
#include "shared.h"

static float   adc_to_voltage(uint16_t raw_value);
static int16_t convert_coolant_temperature(uint16_t raw_value);
static int16_t convert_battery_voltage(uint16_t raw_value);

static const float VREF_V = 3.3f;

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
        if (xQueueReceive(queue_data_raw, &data, portMAX_DELAY) == pdTRUE) {
            switch (data.id) {
                case SENSOR_ID_COOLANT_TEMPERATURE:
                    data.value = convert_coolant_temperature(data.value);
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

/**
 * @brief Convert raw ADC value to voltage in V
 *
 * @param raw_value -- Raw ADC value (0 to 65535)
 *
 * @return float -- Voltage in V
 */
static float adc_to_voltage(uint16_t raw_value)
{
    const float ADC_MAX = 65536.0f;

    return ((float)raw_value / ADC_MAX) * VREF_V;
}

/**
 * @brief Convert raw ADC value to coolant temperature in deci-°C
 *
 * @param raw_value -- Raw ADC value (0 to 65535)
 *
 * @return int16_t -- Temperature in deci-°C (e.g., 234 means 23.4°C)
 */
static int16_t convert_coolant_temperature(uint16_t raw_value)
{
    const float R2_OHM = 470.0f;
    const float R0_OHM = 1100.0f;
    const float T0_K   = 293.15f;
    const float B      = 3572.0f;
    const float K_TO_C = 273.15f;

    float vadc_v = adc_to_voltage(raw_value);

    float rv2_ohm = INFINITY;
    if ((VREF_V - vadc_v) != 0.0f) {
        rv2_ohm = (vadc_v * R2_OHM) / (VREF_V - vadc_v);
    }

    float t_k = 0.0f;
    if ((rv2_ohm / R0_OHM) > 0.0f) {
        t_k = 1 / ((1 / T0_K) + (1 / B) * log(rv2_ohm / R0_OHM));
    }

    float t_c = t_k - K_TO_C;

    return (int16_t)(t_c * 10.0f);
}

/**
 * @brief Convert raw ADC value to battery voltage in centi-V
 *
 * @param raw_value -- Raw ADC value (0 to 65535)
 *
 * @return int16_t -- Battery voltage in centi-V (e.g., 1234 means 12.34V)
 */
static int16_t convert_battery_voltage(uint16_t raw_value)
{
    const float R4_OHM = 10000.0f;
    const float R5_OHM = 2200.0f;

    float vadc_v = adc_to_voltage(raw_value);

    float vbat_v = vadc_v * (R4_OHM + R5_OHM) / R5_OHM;

    return (int16_t)(vbat_v * 100.0f);
}
