#ifndef FILTERING_H
#define FILTERING_H

#include <stdint.h>

int16_t filtering_filter_coolant_temperature(int16_t value);
int16_t filtering_filter_battery_voltage(int16_t value);
int16_t filtering_filter_fuel_level(int16_t value);
int16_t filtering_filter_motor_rpm(int16_t value);
int16_t filtering_debounce_turn_signal(int16_t value);
int16_t filtering_debounce_high_beam(int16_t value);
int16_t filtering_debounce_oil_pressure_0_3_bar(int16_t value);
int16_t filtering_debounce_oil_pressure_1_8_bar(int16_t value);

#endif /* FILTERING_H */
