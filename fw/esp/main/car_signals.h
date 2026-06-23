#pragma once

#include <stdbool.h>
#include <stdint.h>

void car_signals_on_disconnect(void);
void car_signals_on_subscribe(uint16_t attr_handle, bool notify_enabled);

void car_signals_set_water_temp_c(int16_t temp_c);
void car_signals_set_batt_v(uint16_t volts);
void car_signals_set_rpm(uint16_t rpm);
void car_signals_set_turn_signal(bool value);
void car_signals_set_high_beam(bool value);
void car_signals_set_tank_level(uint16_t value);
void car_signals_oil_pressure_3b(bool value);
void car_signals_oil_pressure_18b(bool value);

int16_t car_signals_get_water_temp_cC(void);
uint16_t car_signals_get_batt_mv(void);
uint16_t car_signals_get_rpm(void);
bool car_signals_get_turn_signal(void);
bool car_signals_get_high_beam(void);
bool car_signals_get_oil_pressure_3b(void);
bool car_signals_get_oil_pressure_18b(void);
uint16_t car_signals_get_tank_level(void);
const char *car_signals_get_tank_level_str(void);