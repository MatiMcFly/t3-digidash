#pragma once

#include <stdbool.h>
#include <stdint.h>

void car_signals_on_disconnect(void);
void car_signals_on_subscribe(uint16_t attr_handle, bool notify_enabled);

void car_signals_set_water_temp_c(float temp_c);
void car_signals_set_outside_temp_c(float temp_c);
void car_signals_set_batt_v(float volts);
void car_signals_set_rpm(uint16_t rpm);
void car_signals_set_turn_signal(uint8_t value);
void car_signals_set_high_beam(uint8_t value);
void car_signals_set_preheat(uint8_t value);
void car_signals_set_tank_level(uint8_t percent);

int16_t car_signals_get_water_temp_cC(void);
int16_t car_signals_get_outside_temp_cC(void);
uint16_t car_signals_get_batt_mv(void);
uint8_t car_signals_get_batt_level_percent(void);
uint16_t car_signals_get_rpm(void);
uint8_t car_signals_get_turn_signal(void);
uint8_t car_signals_get_high_beam(void);
uint8_t car_signals_get_preheat(void);
uint8_t car_signals_get_tank_level(void);