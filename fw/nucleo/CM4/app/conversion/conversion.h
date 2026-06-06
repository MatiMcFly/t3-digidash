#ifndef CONVERSION_H
#define CONVERSION_H

#include <stdint.h>

int16_t convert_coolant_temperature(uint16_t raw_value);
int16_t convert_battery_voltage(uint16_t raw_value);
int16_t convert_fuel_level(uint16_t raw_value);
int16_t convert_motor_rpm(uint16_t pulses_per_min);

#endif /* CONVERSION_H */
