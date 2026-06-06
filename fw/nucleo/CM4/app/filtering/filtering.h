#ifndef FILTERING_H
#define FILTERING_H

#include <stdint.h>

int16_t filter_coolant_temperature(int16_t value);
int16_t filter_battery_voltage(int16_t value);
int16_t filter_fuel_level(int16_t value);
int16_t filter_motor_rpm(int16_t value);
int16_t debounce_turn_signal(int16_t value);
int16_t debounce_high_beam(int16_t value);
int16_t debounce_oil_pressure_0_3_bar(int16_t value);
int16_t debounce_oil_pressure_1_8_bar(int16_t value);

#endif /* FILTERING_H */
