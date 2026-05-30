#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void remotexy_init(void);
void remotexy_set_connected(bool connected);
void remotexy_set_notify_enabled(bool enabled);
void remotexy_set_outputs(uint8_t turn_signal_w, uint8_t high_beam_w, int16_t rpm_w,
                          float batt_volt_w, uint8_t oil_03_w, uint8_t oil_18_w,
                          int8_t water_temp_w, int8_t tank_level_w);
void remotexy_handle_rx(const uint8_t *data, size_t len);
