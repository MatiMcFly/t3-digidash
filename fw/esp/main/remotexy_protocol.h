#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void remotexy_init(void);
void remotexy_set_connected(bool connected);
void remotexy_set_notify_enabled(bool enabled);
void remotexy_set_instrument(float value);
void remotexy_set_outputs(uint8_t led_01, uint8_t led_02, float instrument_01,
						  float onlineGraph_01_var1, uint8_t led_03, uint8_t led_04,
						  float linearbar_01, int16_t linearbar_02);
void remotexy_handle_rx(const uint8_t *data, size_t len);
