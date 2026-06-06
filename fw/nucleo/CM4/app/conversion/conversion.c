#include "conversion.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static float adc_to_voltage(uint16_t raw_value);

static const float VREF_V = 3.3f;

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
int16_t conversion_coolant_temperature(uint16_t raw_value)
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
int16_t conversion_battery_voltage(uint16_t raw_value)
{
    const float R4_OHM = 10000.0f;
    const float R5_OHM = 2200.0f;

    float vadc_v = adc_to_voltage(raw_value);

    float vbat_v = vadc_v * (R4_OHM + R5_OHM) / R5_OHM;

    return (int16_t)(vbat_v * 100.0f);
}

/**
 * @brief Convert raw ADC value to fuel level in deci-l
 *
 * @param raw_value -- Raw ADC value (0 to 65535)
 *
 * @return int16_t -- Fuel level in deci-l (e.g., 123 means 12.3 l)
 */
int16_t conversion_fuel_level(uint16_t raw_value)
{
    const float R1_OHM   = 330.0f;
    const float VOL0_L   = 80.8f;
    const float OHM_TO_L = -0.269f;

    float vadc_v = adc_to_voltage(raw_value);

    float rv1_ohm = 0.0f;
    if ((VREF_V - vadc_v) != 0.0f) {
        rv1_ohm = R1_OHM * vadc_v / (VREF_V - vadc_v);
    }

    float vol_l = OHM_TO_L * rv1_ohm + VOL0_L;

    return (int16_t)(vol_l * 10.0f);
}

/**
 * @brief Convert pulses per minute to rotation speed in RPM
 *
 * @param pulses_per_min -- Pulses per minute (e.g., 4000 means 4000 pulses per minute)
 *
 * @return int16_t -- Rotation speed in RPM (e.g., 1000 means 1000 RPM)
 */
int16_t conversion_motor_rpm(uint16_t pulses_per_min)
{
    // 1 motor rotation generates 4 pulses
    // ==> pulses_per_min / 4 --> RPM
    const int16_t PULSES_PER_ROTATION = 4;

    return pulses_per_min / PULSES_PER_ROTATION;
}
